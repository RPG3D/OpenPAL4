#pragma once
#include "pal4/native_combat_ai_actions.h"
#include "pal4/native_combat_ai_decision.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_skill_domain.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_script_event_router.h"
#include "pal4/random_number_generator.h"
#include "pal4/ai_action_task.h"
#include <bit>
#include <map>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
// Fixture selects the real 5703/5804 records on an existing scene Monster.
// Only the decision weights, skill slots and current SP/MP are fixture data;
// candidate construction, rejection, publication and cleanup use actual owners.
inline void VerifyNativeCombatAiCandidates(NativeCombatAiActions& ai,
    NativeCombatAiDecisionRuntime& decisions, NativeCombatActorSkillRuntime& source,
    NativeCombatTaskQueue& queue, NativeCombatSystemImage& image,
    INativeSkillSystemServices& system, NativeScriptEventRouter& router,
    const NativeCombatAiActions::MagicResolver& magic_factory,
    const NativeCombatAiActions::MagicResolver& stunt_factory,
    const NativeCombatActorSkillRuntime::ActionMotionRefresh& motion) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto actor=source.ActorAddress();
    auto& magic=magic_factory(actor);auto& stunt=stunt_factory(actor);
    struct Restore {
        NativeCombatTaskQueue& queue;NativeScriptEventRouter& router;NativeCombatSystemImage& image;
        RandomNumberGenerator& rng;RandomNumberGenerator saved;std::uint32_t state;
        std::map<std::uint32_t,std::uint32_t> fields;std::vector<std::uint32_t> routes;
        ~Restore(){for(auto route:routes)(void)router.Unsubscribe(route);try{queue.ResetForStart();}catch(...){}
            for(auto [a,v]:fields)*reinterpret_cast<std::uint32_t*>(a)=v;image.State()=state;rng=saved;}
    } restore{queue,router,image,system.Random(),system.Random(),image.State()};
    const auto write=[&](std::uint32_t a,std::uint32_t value){restore.fields.try_emplace(a,word(a));word(a)=value;};
    for(unsigned i=0;i<5u;++i)write(actor+896u+4u*i,i==0u?5703u:i==1u?5804u:0u);
    write(actor+576u,100000u);write(actor+580u,100000u);
    const auto decision=decisions.Choose(actor);
    check(decision.property!=0u,"dual AI fixture requires an actual decision row");
    for(const auto& branch:AiActionTask::kBranches)
        write(decision.property+branch.weight_offset,std::bit_cast<std::uint32_t>(branch.virtual_slot==4u?1.f:0.f));
    unsigned stunt_logs{},magic_logs{},final_logs{};
    restore.routes.push_back(router.Subscribe(router.Key("caae_message"),3u,[&](std::uint32_t p){
        const auto context=std::string_view(reinterpret_cast<const char*>(word(p)));
        const auto text=std::string_view(reinterpret_cast<const char*>(word(p+4u)));
        if(context!="cbActorAi::doMagicAttackAction")return;
        if(text.starts_with("selectStuntPipeline"))++stunt_logs;
        if(text.starts_with("selectMagicPipeline"))++magic_logs;
        if(text.starts_with("fint pipeline:")){
            ++final_logs;
            check(magic.Count()+stunt.Count()==1u,"rejected real candidate is destroyed before the final selection log");
        }
    }));
    struct Failure:std::runtime_error{Failure():std::runtime_error("dual AI publication failure fixture") {}};
    bool fail_publication{};
    restore.routes.push_back(router.Subscribe(router.Key("ae_start_action"),1u,[&](std::uint32_t p){
        if(fail_publication && word(p)==actor+716u)throw Failure();
    }));
    bool saw_magic{},saw_stunt{};
    for(unsigned seed=1;seed<=64u && (!saw_magic || !saw_stunt);++seed){
        system.Random().Seed(seed);image.State()=3u;
        const auto before_stunt=stunt_logs,before_magic=magic_logs,before_final=final_logs;
        check(ai.Process(actor) && queue.Count()==1u && ai.LastBranchSlot()==4u,
            "dual real AI candidates publish exactly one original MagicAttack action");
        check(stunt_logs==before_stunt+1u && magic_logs==before_magic+1u && final_logs==before_final+1u,
            "both actual skill candidates participate before selection");
        saw_magic|=magic.Count()!=0u;saw_stunt|=stunt.Count()!=0u;
        queue.ResetForStart();source.EndAction(motion);image.State()=3u;
        check(!magic.Count() && !stunt.Count() && !magic.OwnedAllocations() && !stunt.OwnedAllocations() &&
            !magic.OwnedTexts() && !stunt.OwnedTexts() && !magic.Subscriptions() && !stunt.Subscriptions(),
            "selected and rejected real skill resources retire without retained ownership");
    }
    check(saw_magic && saw_stunt,"original final random choice can select either real candidate");
    system.Random().Seed(1u);fail_publication=true;bool failed{};
    try{(void)ai.Process(actor);}catch(const Failure&){failed=true;}
    fail_publication=false;
    check(failed && !queue.Count() && !magic.Count() && !stunt.Count() &&
        !magic.OwnedAllocations() && !stunt.OwnedAllocations() && !ai.PendingPoolCount(),
        "publication failure cancels the selected action after retiring its rejected sibling");
    source.EndAction(motion);
    std::cout<<"Native AI candidates: real5703/5804, both selected by original RNG, rejected destruction before publication, queue-reset and publication-failure cleanup verified\n";
}
}
