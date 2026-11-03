#include "pal4/comparison_party_recovery.h"
#include <iostream>
#include <iterator>
namespace {
void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
}
int main(){
    const auto root=std::filesystem::path("H:/PAL4/objs/tmp")/("party-recovery-"+std::to_string(GetCurrentProcessId()));
    Check(std::filesystem::create_directory(root),"isolated recovery receipts");
    _putenv_s("PAL4_COMPARE_STATE_DIR",root.string().c_str());
    pal4::ComparisonPartyRecovery owner("test");
    pal4::ComparisonFrameGate::Observation held{};held.phase=pal4::ComparisonFrameGate::Phase::held;
    unsigned calls{};const auto recover=[&]{++calls;};
    const auto receipt=[&](unsigned id){std::ifstream f(root/("compare-test-party-recovery-"+
        std::to_string(GetCurrentProcessId())+"-"+std::to_string(id)+".json"));return std::string(std::istreambuf_iterator<char>(f),{});};
    auto running=held;running.phase=pal4::ComparisonFrameGate::Phase::running;
    owner.Apply(1u,90500u,90500u,running,true,recover);
    owner.Apply(2u,90500u,90500u,held,false,recover);
    owner.Apply(3u,90500u,90600u,held,true,recover);
    auto nested=held;nested.depth=1u;owner.Apply(4u,90500u,90500u,nested,true,recover);
    Check(!calls && receipt(1).find("rejected")!=std::string::npos && receipt(3).find("90600")!=std::string::npos,
          "running/locked/wrong-story/nested recovery must reject without a game call");
    owner.Apply(5u,90500u,90500u,held,true,[&]{recover();owner.Apply(6u,90500u,90500u,held,true,recover);});
    Check(calls==1u && receipt(5).find("applied")!=std::string::npos && receipt(6).find("busy")!=std::string::npos,"once-only recovery and reentry");
    bool failed{};try{owner.Apply(7u,90500u,90500u,held,true,[]{throw std::runtime_error("intentional game failure");});}
    catch(const std::runtime_error&){failed=true;}
    Check(failed && receipt(7).find("failed")!=std::string::npos,"failed game call receipt");
    owner.Apply(8u,90500u,90500u,held,true,recover);
    for(auto id:{1u,5u,6u,7u})owner.Apply(id,90500u,90500u,held,true,recover);
    Check(calls==2u,"old rejected/busy/failed/applied ids must not replay after newer operations");
    std::cout<<"PASS: held Scene/story guards, actual callback once, reentry, failure consumption and receipts\n";
}
