#pragma once
#include "pal4/comparison_game_heap_trace.h"
#include <windows.h>

namespace pal4 {
inline void ProbeComparisonHeapBackendModules(ComparisonGameHeapTrace::BackendRecord& record) {
    // GetModuleHandle observes existing mappings without loading or addrefing DLLs.
    record.windows_ui=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(GetModuleHandleW(L"Windows.UI.dll")));
    record.core_messaging=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(GetModuleHandleW(L"CoreMessaging.dll")));
}
// Diagnostic OS queries only. No backend calls, symbol loading or heap allocation.
inline ComparisonGameHeapTrace::FailureRecord ProbeComparisonHeapFailure(void* top) {
    ComparisonGameHeapTrace::FailureRecord out;
    void* stack[16]{};
    out.stack_count=CaptureStackBackTrace(1,16,stack,nullptr);
    for(unsigned i=0;i<out.stack_count;++i)
        out.stack[i]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(stack[i]));
    const auto words=[](const MEMORY_BASIC_INFORMATION& m) {
        return std::array<std::uint32_t,7>{
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(m.BaseAddress)),
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(m.AllocationBase)),
            static_cast<std::uint32_t>(m.RegionSize),m.State,m.Protect,m.AllocationProtect,m.Type};
    };
    MEMORY_BASIC_INFORMATION first{};
    if(VirtualQuery(top,&first,sizeof(first))!=sizeof(first))return out;
    out.top_region=words(first);out.query_valid=1;
    auto current=first;
    for(unsigned i=0;i<64;++i) {
        const auto next=reinterpret_cast<std::uintptr_t>(current.BaseAddress)+current.RegionSize;
        if(next<=reinterpret_cast<std::uintptr_t>(current.BaseAddress))break;
        MEMORY_BASIC_INFORMATION region{};
        if(VirtualQuery(reinterpret_cast<void*>(next),&region,sizeof(region))!=sizeof(region))break;
        if(region.AllocationBase!=first.AllocationBase) {
            out.after_reservation=words(region);out.query_valid=2;break;
        }
        current=region;
    }
    return out;
}
}
