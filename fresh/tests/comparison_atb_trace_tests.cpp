#include "pal4/comparison_atb_trace.h"
#include <cassert>
#include <map>
#include <sstream>
#include <stdexcept>
#include <iostream>

int main(){
    using namespace pal4;
    std::map<std::uint32_t,std::uint32_t> memory;
    constexpr std::uint32_t manager=0x10000,combat=0x20000,storage=0x30000,actor=0x40000;
    memory[combat+508]=2;memory[combat+516]=0;memory[manager+112]=storage;
    memory[manager+116]=storage+4;memory[storage]=actor+716;
    memory[actor+804]=1;memory[actor+1196]=1001;
    memory[actor+720]=0x427fffffu;memory[actor+832]=3;memory[actor+828]=1;
    auto read=[&](auto a){return memory[a];};
    const auto before=ReadComparisonAtbSample(manager,combat,624,read);
    assert(!before.error && before.count==1 && before.actors[0].id==1001 && before.actors[0].progress_bits==0x427fffffu);
    memory[actor+720]=0x42800000u;memory[actor+832]=4;
    const auto after=ReadComparisonAtbSample(manager,combat,4,read);
    ComparisonAtbTrace trace(true,2);
    auto* first=trace.Begin(manager,0x3c8b4396u,before);
    assert(first && !trace.Begin(manager,0,before)); // Nested observation cannot corrupt outer record.
    trace.End(first,after);
    auto* second=trace.Begin(manager,1,before);assert(second);trace.Abort(second);
    assert(!trace.Begin(manager,2,before));
    std::ostringstream out;trace.Write(out);
    const auto text=out.str();
    assert(text.find("\"dropped\":1")!=std::string::npos && text.find("\"aborted\":1")!=std::string::npos);
    assert(text.find("\"nested\":1")!=std::string::npos && text.find("\"pending\":false")!=std::string::npos);
    assert(first->before.actors[0].turn==3 && first->after.actors[0].turn==4 && first->before.random_index==624 && first->after.random_index==4);
    ComparisonAtbTrace disabled(false,100);assert(!disabled.Begin(manager,0,before));
    ComparisonAtbTrace preparation(true,1,true);
    auto* prepared=preparation.BeginPreparation(manager,0x12340002u,before);assert(prepared);
    preparation.EndPreparation(prepared,0u,after);
    assert(prepared->argument_before==0x12340002u && prepared->argument_after==0u &&
           prepared->before.actors[0].progress_bits==0x427fffffu && prepared->after.random_index==4u);
    std::ostringstream prepare_out;preparation.Write(prepare_out);
    assert(prepare_out.str().find("\"boundary\":\"combat_prepare\"")!=std::string::npos);
    assert(prepare_out.str().find("\"argument_before\":305397762")!=std::string::npos);
    assert(!preparation.BeginPreparation(manager,2u,before));
    // A separate boundary includes frozen ATB frames; it must preserve both
    //samples and make truncation explicit without merging with manager calls.
    ComparisonAtbTrace frame(true,2,false,"combat_update");
    auto paused=before;paused.active=1;paused.state=3;
    auto* held=frame.Begin(manager,0x3c8b4396u,paused);assert(held);frame.End(held,paused);
    auto resumed=paused;resumed.active=0;
    auto* released=frame.Begin(manager,0x3c8b4396u,paused);assert(released);frame.End(released,resumed);
    assert(!frame.Begin(manager,0u,resumed));
    std::ostringstream frame_out;frame.Write(frame_out);
    assert(frame_out.str().find("\"boundary\":\"combat_update\"")!=std::string::npos &&
           frame_out.str().find("\"dropped\":1")!=std::string::npos &&
           held->before.actors[0].progress_bits==held->after.actors[0].progress_bits &&
           released->before.active==1u && released->after.active==0u);
    for(unsigned state=0;state<=8u;++state)
        assert(ShouldTraceComparisonCombatFrame(state)==((state>=1u && state<=3u)||state==6u));
    memory[manager+116]=storage+68;unsigned reads{};
    const auto oversized=ReadComparisonAtbSample(manager,combat,0,[&](auto a){++reads;return memory[a];});
    assert(oversized.error==1 && oversized.count==0 && reads==6);
    memory[manager+116]=storage+4;memory[actor+804]=7;
    assert(ReadComparisonAtbSample(manager,combat,0,read).error==3);
    std::cout<<"PASS: raw actor boundary, before/after independence, prefix overflow, abort/nesting/disabled and bounded-invalid spans\n";
}
