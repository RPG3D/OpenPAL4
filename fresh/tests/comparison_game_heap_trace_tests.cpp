#include "pal4/comparison_game_heap_trace.h"
#include "pal4/comparison_game_heap_failure_probe_win32.h"
#include "pal4/native_game_allocator.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
void Check(bool condition,const char* message) {
    if(!condition)throw std::runtime_error(message);
}
std::string Json(const pal4::ComparisonGameHeapTrace& trace) {
    std::ostringstream output;trace.Write(output);return output.str();
}
std::size_t Count(const std::string& text,const std::string& needle) {
    std::size_t total{},start{};
    while((start=text.find(needle,start))!=std::string::npos){++total;start+=needle.size();}
    return total;
}
}
int main() {
    try {
        pal4::ComparisonGameHeapTrace disabled(false,2,2);
        disabled.Add({0,168,0,0,100,0,172});
        Check(Json(disabled).find("\"total\":0")!=std::string::npos,"disabled observer retains no events");
        pal4::ComparisonGameHeapTrace bounded(true,3,4);
        bounded.Add({0,168,0,0,100,0,172});
        bounded.Add({1,0,0,100,0,172,0});
        bounded.Add({0,168,0,0,100,0,172});
        bounded.Add({0,32,0,0,200,0,36});
        bounded.Add({1,0,0,200,0,36,0});
        const auto bounded_json=Json(bounded);
        Check(bounded_json.find("\"capacity\":3,\"count\":3,\"dropped\":2")!=std::string::npos,
              "recent ring exposes dropped history");
        Check(bounded_json.find("\"capacity\":4,\"count\":3,\"dropped\":0")!=std::string::npos,
              "candidate lifetime survives unrelated allocations");
        Check(bounded_json.find("\"sequence\":2")<bounded_json.find("\"sequence\":3") &&
              bounded_json.find("\"sequence\":3")<bounded_json.find("\"sequence\":4"),
              "wrapped recent ring exports chronological completion order");
        Check(Count(bounded_json,"\"before\":100")==1u && Count(bounded_json,"\"after\":100")==3u,
              "free and pointer reuse remain separate events");

        pal4::ComparisonGameHeapTrace backend(true,2,2,2);
        pal4::ComparisonGameHeapTrace::FailureRecord failure;
        failure.stack_count=2;failure.stack[0]=123;failure.stack[1]=456;
        backend.AddBackend({4,4096,0,0xffffffffu,0,1,10,20},&failure);
        backend.Add({0,168,0,0,1000,0,172});
        backend.AddBackend({4,0xffffffffu,0,0xffffffffu,1000,1,11,20});
        backend.AddBackend({5,1048576,0,2000,1000,1,12,20});
        const auto backend_json=Json(backend);
        Check(backend_json.find("\"backend\":{\"capacity\":2,\"count\":2,\"dropped\":1")!=std::string::npos,
              "backend overflow cannot silently claim complete chronology");
        Check(backend_json.find("\"size_bits\":4294967295")!=std::string::npos &&
              backend_json.find("\"result\":4294967295")!=std::string::npos,
              "signed trim request and failure sentinel retain exact bits");
        Check(Count(backend_json,"\"heap_completed\":1")==2u &&
              backend_json.find("\"kind\":4")<backend_json.find("\"kind\":5"),
              "failed morecore then fallback stay ordered and anchored to completed outer calls");
        Check(backend_json.find("\"backend_sequence\":0,\"heap_completed\":0")!=std::string::npos &&
              backend_json.find("\"stack\":[123,456]")!=std::string::npos,
              "failure evidence retains its own backend sequence across backend-ring overflow");
        auto* reservation=VirtualAlloc(nullptr,65536,MEM_RESERVE,PAGE_NOACCESS);
        Check(reservation && VirtualAlloc(reservation,4096,MEM_COMMIT,PAGE_READWRITE)==reservation,
              "real split committed/reserved OS fixture");
        const auto probe=pal4::ProbeComparisonHeapFailure(reservation);
        const auto address=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(reservation));
        Check(probe.query_valid==2 && probe.stack_count>0 && probe.top_region[0]==address &&
              probe.top_region[1]==address && probe.top_region[3]==MEM_COMMIT &&
              probe.after_reservation[0]==address+65536,
              "probe walks committed and reserved parts to the next allocation without committing pages");
        Check(VirtualFree(reservation,0,MEM_RELEASE)!=0,"release diagnostic OS fixture");
        pal4::ComparisonGameHeapTrace::BackendRecord modules;
        const auto windows_ui=GetModuleHandleW(L"Windows.UI.dll");
        const auto core_messaging=GetModuleHandleW(L"CoreMessaging.dll");
        pal4::ProbeComparisonHeapBackendModules(modules);
        Check(modules.windows_ui==reinterpret_cast<std::uint32_t>(windows_ui) &&
                  modules.core_messaging==reinterpret_cast<std::uint32_t>(core_messaging) &&
                  GetModuleHandleW(L"Windows.UI.dll")==windows_ui &&
                  GetModuleHandleW(L"CoreMessaging.dll")==core_messaging,
              "module probe reports existing bases without loading an absent DLL");

        _putenv_s("PAL4_COMPARE_SCRIPTED_INPUT","1");
        _putenv_s("PAL4_COMPARE_GAME_HEAP_TRACE","1");
        auto& live=pal4::GetComparisonGameHeapTrace();
        Check(live.Enabled(),"explicit trace input enabled");
        Check(!pal4::comparison_game_heap_origin.source,"origin starts empty");
        void* block{};
        {
            pal4::ComparisonGameHeapOriginScope scope(std::source_location::current());
            block=pal4_game_malloc(168u);
        }
        Check(block && !pal4::comparison_game_heap_origin.source,"game heap allocation and origin restoration");
        auto* zero=pal4_game_calloc(2u,84u);
        Check(zero!=nullptr,"actual calloc fixture");
        pal4_game_free(block);pal4_game_free(zero);
        const auto actual=Json(live);
        Check(actual.find("\"total\":4")!=std::string::npos && Count(actual,"\"kind\":3")==2u,
              "actual calloc emits one outer event in each exported ring");
        Check(actual.find("comparison_game_heap_trace_tests.cpp")!=std::string::npos &&
              actual.find("\"usable_before\":172")!=std::string::npos,
              "actual creating origin and before-free usable size retained");
        Check(actual.find("\"kind\":4")!=std::string::npos &&
              actual.find("\"heap_completed\":0")!=std::string::npos,
              "actual source backend reports initial morecore before first outer allocation completion");
        auto* mapped=pal4_game_malloc(0x80000u);
        Check(mapped && (reinterpret_cast<const std::uint32_t*>(mapped)[-1]&2u),
              "large fixture actually uses the source mmap path");
        Check(Json(live).find("\"kind\":5,\"size_bits\":528384")!=std::string::npos,
              "actual six-argument mmap forwarding reports its page-rounded request");
        pal4_game_free(mapped);
        std::cout<<"PASS: bounded lifetime history, reuse, source scope and real outer allocator observations\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
}
