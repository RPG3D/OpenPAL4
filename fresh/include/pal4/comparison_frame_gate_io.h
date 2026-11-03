#pragma once
#include "pal4/comparison_frame_gate.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <locale>
#include <string>
#include <string_view>
#include <windows.h>

namespace pal4 {
struct ComparisonCountdownAddresses {std::uint32_t owner{},queue{};};
inline ComparisonCountdownAddresses& GetComparisonCountdownAddresses(){
    static ComparisonCountdownAddresses value;return value;
}
inline void BindComparisonCountdown(std::uint32_t owner,std::uint32_t queue){
    GetComparisonCountdownAddresses()={owner,queue};
}
inline std::uint32_t ComparisonReadWord(std::uint32_t address){
    std::uint32_t value;std::memcpy(&value,reinterpret_cast<const void*>(address),4u);return value;
}
// Main-thread observer of the existing SceneTimer list; it invokes no callbacks.
inline void WriteComparisonCountdown(std::ostream& out,
    ComparisonCountdownAddresses addresses=GetComparisonCountdownAddresses()){
    if(!addresses.owner || !addresses.queue){out<<"null";return;}
    out<<"{\"remaining\":"<<ComparisonReadWord(addresses.owner+0xf0u)<<",\"timers\":[";
    const auto head=ComparisonReadWord(addresses.queue+4u);
    if(!head)throw std::runtime_error("frame gate timer list has no sentinel");
    auto node=ComparisonReadWord(head);unsigned visited{},matched{};
    while(node!=head){
        if(!node || ++visited>10000u)throw std::runtime_error("frame gate timer list is corrupt");
        const auto data=ComparisonReadWord(node+12u),length=ComparisonReadWord(node+16u);
        const auto callback=ComparisonReadWord(node+36u);
        if(length==10u && data && std::string_view(reinterpret_cast<const char*>(data),length)=="SceneTimer" &&
            callback && ComparisonReadWord(callback+4u)==addresses.owner){
            if(matched++)out<<',';
            out<<"{\"initial_bits\":"<<ComparisonReadWord(node+24u)
               <<",\"current_bits\":"<<ComparisonReadWord(node+28u)<<'}';
        }
        node=ComparisonReadWord(node);
    }
    out<<"]}";
}
inline void PublishComparisonFrameGate(std::string_view side,std::uint32_t request,bool accepted=true){
    const auto* root=std::getenv("PAL4_COMPARE_STATE_DIR");
    if(!root || !*root)throw std::runtime_error("frame gate output directory is absent");
    const auto directory=std::filesystem::path(root);
    if(!directory.is_absolute())throw std::runtime_error("frame gate output directory must be absolute");
    const auto path=directory/("compare-"+std::string(side)+"-framegate-"+
        std::to_string(GetCurrentProcessId())+"-"+std::to_string(request)+".json");
    const auto temporary=std::filesystem::path(path.string()+".tmp");
    const auto s=GetComparisonFrameGate().Observe();
    {
        std::ofstream out(temporary,std::ios::binary|std::ios::trunc);out.imbue(std::locale::classic());
        out<<"{\"pid\":"<<GetCurrentProcessId()<<",\"request\":"<<request
           <<",\"accepted\":"<<(accepted?"true":"false")<<",\"phase\":\""<<ComparisonFrameGate::PhaseName(s.phase)
           <<"\",\"depth\":"<<s.depth<<",\"remaining\":"<<s.remaining<<",\"active_request\":"<<s.request
           <<",\"arm_period\":"<<s.arm_period<<",\"arm_calls\":"<<s.arm_calls
           <<",\"arm_pending\":"<<(s.arm_pending?"true":"false")
           <<",\"checkpoint_allowed\":"<<(s.checkpoint_allowed?"true":"false")
           <<",\"arm_source\":\""<<ComparisonFrameGate::ArmSourceName(s.arm_source)<<'"'
           <<",\"completed\":"<<s.completed<<",\"granted\":"<<s.granted<<",\"nested\":"<<s.nested
           <<",\"clock_reads\":"<<s.clock_reads<<",\"callbacks\":"<<s.callbacks
           <<",\"clock_ms\":"<<GetComparisonFrameClock().Current()
           <<",\"step_ms\":"<<GetComparisonFrameClock().StepMilliseconds()<<",\"countdown\":";
        WriteComparisonCountdown(out);out<<"}\n";out.flush();
        if(!out)throw std::runtime_error("frame gate result write failed");
    }
    if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("frame gate result publication failed");
}
}
