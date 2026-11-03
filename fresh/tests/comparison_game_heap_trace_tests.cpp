#include "pal4/comparison_game_heap_trace.h"
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
        std::cout<<"PASS: bounded lifetime history, reuse, source scope and real outer allocator observations\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
}
