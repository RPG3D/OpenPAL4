#include "pal4/native_combat_end_sound_events.h"
#include "pal4/native_script_event_router.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
void Check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void Reject(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}Check(caught,"expected rejection");}
}
int main(){try{
    std::array<std::uint32_t,130> image{};
    const auto combat=reinterpret_cast<std::uint32_t>(image.data());auto& slot=image[105];
    pal4::NativeScriptEventRouter router;
    using Owner=pal4::NativeCombatEndSoundEvents;
    Owner* active{};bool inactive=false,fail=false,acquire_fail=false;
    unsigned acquisitions{};std::vector<std::string> calls;
    pal4::NativeCombatEndSoundServices services;
    services.acquire_combat=[&]{++acquisitions;if(acquire_fail)throw std::logic_error("factory unavailable");return combat;};
    services.read_word=[&](auto a){
        if(a==combat+460u)return image[115];
        if(a==combat+128u)return 1234u;
        throw std::invalid_argument("foreign read");
    };
    services.inactive=[&](auto c){Check(c==combat,"actual system preserved");return inactive;};
    services.text=[&](auto t)->std::string{
        if(t==combat+452u)return "P01-1";
        if(t==1234u)return "M01";
        throw std::invalid_argument("foreign text");
    };
    services.special_music=[&](auto t,float dt,bool restore){
        Check(dt==0.0f,"original zero fade");
        Reject([&]{active->Close();});
        calls.push_back(std::string(t)+(restore?":restore":":special"));
        if(fail)throw std::runtime_error("music transport failure");
    };
    services.restore_music=[&](auto scene){calls.push_back("scene:"+std::string(scene));};
    services.play_sound=[&](auto path,auto mode){
        Check(mode==1u,"original sound repeat mode");calls.push_back(std::string(path));
        if(fail)throw std::runtime_error("sound transport failure");return 77u;
    };
    Reject([&]{Owner invalid(combat,image[104],router,services);});
    Reject([&]{Owner invalid(combat,slot,router,{});});
    const auto unrelated=router.Subscribe(router.Key("other"),1u,[](auto){});
    {
        Owner owner(combat,slot,router,services);active=&owner;
        Check(slot==owner.Address() && owner.SubscriptionCount()==2u,"member420 publishes two subscriptions");
        Reject([&]{Owner duplicate(combat,slot,router,services);});
        for(std::uint32_t cue=0;cue<4u;++cue){
            const auto before=acquisitions;router.Dispatch("combat_over",reinterpret_cast<std::uint32_t>(&cue));
            Check(acquisitions==before,"result cue does not query singleton");
        }
        Check(calls==std::vector<std::string>{"wa16.mp3:restore","wa15.mp3:restore","wa15.mp3:restore"},"all original result cue branches");
        calls.clear();router.Dispatch("combat_prepare",0u);
        Check(calls==std::vector<std::string>{"scene:M01","gamedata\\palSound\\WA14.wav"},"prepare restores scene then plays WA14");
        image[115]=1u;calls.clear();router.Dispatch("combat_prepare",0u);
        Check(calls==std::vector<std::string>{"P01-1:special","gamedata\\palSound\\WA14.wav"},"prepare explicit track then effect");
        inactive=true;calls.clear();router.Dispatch("combat_prepare",0u);
        Check(calls.empty() && !owner.TemporaryTexts(),"inactive skips audio and retires text");inactive=false;
        fail=true;Reject([&]{router.Dispatch("combat_prepare",0u);});
        Check(!owner.TemporaryTexts(),"music failure releases temporary text");
        image[115]=0u;Reject([&]{router.Dispatch("combat_prepare",0u);});
        Check(!owner.TemporaryTexts(),"sound failure releases filename and path");
        std::uint32_t cue=0u;Reject([&]{router.Dispatch("combat_over",reinterpret_cast<std::uint32_t>(&cue));});
        Check(!owner.TemporaryTexts(),"cue transport failure releases text");fail=false;
        acquire_fail=true;Reject([&]{router.Dispatch("combat_prepare",0u);});acquire_fail=false;
        Check(!owner.TemporaryTexts(),"acquisition failure has no text leak");
        Reject([&]{router.Dispatch("combat_over",0u);});
        const auto published=slot;slot=42u;Reject([&]{owner.Close();});slot=published;
        owner.Close();owner.Close();
        Check(!slot && !owner.SubscriptionCount() && router.Contains(unrelated),"retirement preserves other owners");
    }
    {Owner restart(combat,slot,router,services);}
    Check(!slot && router.SubscriptionCount()==1u,"destructor unsubscribes and clears member420");
    std::cout<<"PASS: Combat member420 original audio branches, service dispatch, failure cleanup and retirement\n";
    return EXIT_SUCCESS;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}}
