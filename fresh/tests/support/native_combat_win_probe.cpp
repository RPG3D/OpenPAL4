#include "native_combat_win_probe.h"
#include "pal4/native_combat_result_actions.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_reaction_domain.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_script_event_router.h"
#include "pal4/random_number_generator.h"
#include "pal4/win_action_task.h"
#include "pal4/level_up_action_task.h"
#include "pal4/win_action_child_task.h"
#include "pal4/check_protect_children.h"
#include <bit>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
void VerifyNativeCombatWin(NativeCombatResultActions& win,NativeCombatTaskQueue& queue,NativeCombatActorSkillRuntime& actor,
    NativeCombatReactionDomain& domain,INativeSkillSystemServices& system,NativeScriptEventRouter& router,
    const std::function<void(float)>& advance,const std::function<std::array<float,3>()>& formation,
    const std::function<std::string()>& animation,const std::function<void()>& restore_motion){
    const auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto a=actor.ActorAddress();check(!queue.Count() && !domain.Count() && !domain.Subscriptions(),"Win probe starts with idle actual actor domains");
    struct Restore{NativeCombatTaskQueue& queue;RandomNumberGenerator& rng;RandomNumberGenerator saved;
        std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;const std::function<void()>& motion;
        ~Restore(){try{queue.Clear();motion();}catch(...){}for(auto [p,v]:fields)*reinterpret_cast<std::uint32_t*>(p)=v;rng=saved;}
    } restore{queue,system.Random(),system.Random(),{},restore_motion};
    for(unsigned o=340u;o<676u;o+=4u)restore.fields.emplace_back(a+o,word(a+o));
    restore.fields.emplace_back(a+828u,word(a+828u));
    for(unsigned branch=0;branch<2u;++branch){
        std::uint32_t seed{},sample{};for(;seed<1000u;++seed){RandomNumberGenerator candidate;candidate.Seed(seed);sample=candidate.Next()%1000u;if((sample<500u)==(branch==0u))break;}
        check(seed<1000u,"original RNG supplies both victory choices");system.Random().Seed(seed);auto expected=system.Random();(void)expected.Next();
        const auto task=win.Create();check(task && word(task)==WinActionTask::kVtable && word(task+44u)==a && !win.Processed(task) && win.Identity(task)==a,"actual Win factory/vtable/actor/processed callbacks");
        check(queue.Register(task)==task && queue.Count()==1u,"actual production task queue owns Win parent");
        check(system.Random().Index()==expected.Index(),"Win consumes one original RNG sample");
        for(unsigned i=0;i<RandomNumberGenerator::kStateCount;++i)check(system.Random().StateAt(i)==expected.StateAt(i),"Win RNG state matches original transition");
        const auto sentinel=word(task+8u);std::vector<std::uint32_t> children;
        for(auto n=word(sentinel);n!=sentinel;n=word(n))children.push_back(word(n+8u));
        check(children.size()==3u && word(children[0])==CheckProtectPositionTask::kVtable && word(children[1])==WinActionSecondChildTask::kVtable && word(children[2])==WinActionThirdChildTask::kVtable,"Win owns exact three original mixed GOM children");
        const auto target=formation();check(*reinterpret_cast<const std::array<float,3>*>(children[0]+60u)==target && word(children[0]+72u)==a && word(children[1]+60u)==a,"Win uses actual per-slot formation, not fixed center descriptor");
        const std::string selected=branch==0u?"z14":"z15";
        check(std::string(reinterpret_cast<const char*>(word(children[2]+68u)))==selected,"native owned victory message matches original global selection");
        for(auto c:children)check(word(c+12u)==1u && word(c+56u)==task+4u,"Win original serial wait and container context");
        bool observed{},subscribed{};unsigned frames{};
        while(queue.Count() && frames<1200u){advance(1.f/60.f);++frames;observed|=_stricmp(animation().c_str(),selected.c_str())==0;subscribed|=domain.Subscriptions()!=0u;}
        check(!queue.Count() && !win.Count() && !domain.Count() && !domain.OwnedAllocations() && !domain.OwnedTexts() && !domain.Subscriptions() && observed && subscribed,
            "real formation/prepare/victory playback and AMF subscriptions complete with zero Win/mixed ownership");
        std::cout<<"Native Win action: actor="<<a<<" action="<<selected<<" seed="<<seed<<" frames="<<frames<<" exact original three children, real formation/model/AMF, RNG and zero ownership pass\n";
    }
    // Same native parent/child engine executes the original LevelUp recipe.
    const auto old_state=word(a+828u);word(a+828u)=1u;
    const auto before_level_rng=system.Random();const auto upgrade=win.CreateLevelUp();
    check(upgrade && word(upgrade)==LevelUpActionTask::kVtable,"real LevelUp factory vtable");
    (void)queue.Register(upgrade);check(word(a+828u)==7u && !win.CreateLevelUp(),"original state7 blocks duplicate LevelUp allocation");
    check(system.Random().Index()==before_level_rng.Index(),"LevelUp does not consume victory RNG");
    const auto level_head=word(upgrade+8u);const auto level_child=word(word(level_head)+8u);
    check(word(upgrade+12u)==1u && word(level_child)==WinActionThirdChildTask::kVtable &&
        std::string(reinterpret_cast<const char*>(word(level_child+68u)))=="z16","original single native message child and owned z16 action");
    bool level_seen{};unsigned level_frames{};while(queue.Count() && level_frames<1200u){advance(1.f/60.f);++level_frames;level_seen|=_stricmp(animation().c_str(),"z16")==0;}
    check(level_seen && !queue.Count() && !domain.Count() && !domain.OwnedAllocations() && !domain.OwnedTexts() && !domain.Subscriptions(),"actual z16/AMF LevelUp action fully retires");
    std::cout<<"Native LevelUp action: actor="<<a<<" state7/z16 frames="<<level_frames<<" original single message, no RNG, duplicate-state guard and zero ownership pass\n";
    word(a+828u)=old_state;
    const auto task=win.Create();(void)queue.Register(task);queue.ResetFlags();queue.Update(0.f);
    check(!queue.Count() && !domain.Count() && !domain.OwnedAllocations() && !domain.OwnedTexts() && !domain.Subscriptions(),"Win canceled before playback retires mixed children and position owner");
    const auto facet=word(a+828u),action=word(a+660u);unsigned starts{};
    const auto token=router.Subscribe(router.Key("ae_start_action"),3u,[&](std::uint32_t p){if(word(p)==a+716u)++starts;});
    const auto restored=win.Create();(void)queue.Register(restored);(void)router.Unsubscribe(token);
    check(!starts && word(a+828u)==facet && word(a+660u)==action,"Win virtual568 model restore preserves facet/action and emits no virtual476 start event");
    queue.ResetFlags();queue.Update(0.f);
    check(!domain.Count() && !domain.OwnedAllocations() && !domain.OwnedTexts() && !domain.Subscriptions(),"model-restore regression graph fully cancels");
}
}
