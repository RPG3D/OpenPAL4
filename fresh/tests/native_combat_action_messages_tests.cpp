#include "pal4/native_combat_action_messages.h"
#include "pal4/native_script_event_router.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F> void Reject(F f){bool rejected{};try{f();}catch(const std::logic_error&){rejected=true;}Check(rejected,"invalid subscription accepted");}
}
int main(){try{
    pal4::NativeScriptEventRouter router;
    // Explicit task/event fixtures exercise routing and cancellation, not damage.
    std::array<std::uint32_t,24> first{},second{};
    const auto a=reinterpret_cast<std::uint32_t>(first.data());
    const auto b=reinterpret_cast<std::uint32_t>(second.data());
    first[15]=0x1000u;second[15]=0x2000u;
    const auto ak=router.Key("ae_animation_message"),ek=router.Key("effect_message");
    std::array<std::uint32_t,2> animation{0x1000u+716u,0x1234u};
    std::array<std::uint32_t,6> effect{7u,8u,1u,0u,0u,0u};
    const auto ap=reinterpret_cast<std::uint32_t>(animation.data());
    const auto ep=reinterpret_cast<std::uint32_t>(effect.data());
    unsigned animations{},effects{};std::vector<unsigned> order;
    const auto before=router.Subscribe(ak,1u,[&](auto){order.push_back(1u);});
    const auto after=router.Subscribe(ak,3u,[&](auto){order.push_back(3u);});
    pal4::NativeCombatActionMessages messages(router);
    auto ac=[&](auto task,auto command){Check(task==a && command==animation[1],"animation payload/filter");++animations;order.push_back(2u);};
    auto ec=[&](auto task,auto x,auto y,auto header){Check(task==a && x==7u && y==8u && header==ep+8u,"effect handle/header view");++effects;};
    Reject([&]{messages.Register(0u,ac,ec);});Reject([&]{messages.Register(a,{},ec);});
    first[20]=12u;Reject([&]{messages.Register(a,ac,ec);});first[20]=0u;
    messages.Register(a,ac,ec);Reject([&]{messages.Register(a,ac,ec);});
    Check(messages.Count()==1u && router.SubscriptionCount()==4u && router.Contains(first[20]) && router.Contains(first[22]),"native tokens owned");
    Check(first[21]==ak && first[23]==ek,"original key slots");
    (void)router.Dispatch(ak,ap);Check(animations==1u && order==std::vector<unsigned>{1u,2u,3u},"original mode2 ordering");
    animation[0]=0x2000u+716u;(void)router.Dispatch(ak,ap);Check(animations==1u,"foreign actor filtered");
    animation[0]=0u;(void)router.Dispatch(ak,ap);Check(animations==1u,"null actor filtered");
    (void)router.Dispatch(ek,ep);Check(effects==1u,"effect callback dispatch");
    messages.Detach(a);messages.Detach(a);
    Check(!messages.Count() && router.SubscriptionCount()==2u && !first[20] && !first[21] && !first[22] && !first[23],"detach clears exact pairs and keeps other subscribers");
    animation[0]=0x1000u+716u;(void)router.Dispatch(ak,ap);(void)router.Dispatch(ek,ep);
    Check(animations==1u && effects==1u,"retired task receives no messages");
    messages.Register(a,[&](auto,auto){messages.Detach(a);(void)router.Dispatch(ek,ep);},ec);
    (void)router.Dispatch(ak,ap);Check(!messages.Count() && effects==1u,"self-cancel and nested dispatch do not retain callback");
    messages.Register(a,[&](auto,auto){throw std::runtime_error("intentional");},ec);
    bool failed{};try{(void)router.Dispatch(ak,ap);}catch(const std::runtime_error&){failed=true;}
    Check(failed && messages.Count()==1u,"callback failure propagates without retiring task");messages.Clear();
    unsigned hits{};
    messages.Register(a,ac,ec);
    messages.Register(b,[&](auto task,auto command){Check(task==b && command==animation[1],"second task");++hits;},
        [&](auto task,auto,auto,auto){Check(task==b,"second effect task");++hits;});
    animation[0]=0x2000u+716u;(void)router.Dispatch(ak,ap);Check(hits==1u,"independent tasks");
    messages.Clear();Check(!messages.Count() && router.SubscriptionCount()==2u && !second[20] && !second[22],"multi task clear");
    {pal4::NativeCombatActionMessages scoped(router);scoped.Register(a,ac,ec);}
    Check(router.SubscriptionCount()==2u && router.Contains(before) && router.Contains(after),"destructor retires only owned tokens");
    std::cout<<"Native combat action messages: shared filtering/payload, mode order, exact tokens, foreign/null gates, cancel/nesting, exceptions, multi-task and cleanup pass\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
