#pragma once
#include "pal4/native_combat_ai_actions.h"
#include "pal4/native_combat_monster_ready_events.h"
#include "pal4/native_combat_ai_decision.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_attack_actions.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_player_action_memory.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_physical_attack_domain.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include "pal4/native_script_event_router.h"
#include "native_combat_ai_candidates_probe.h"
#include <array>
#include <bit>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatAiActions(NativeCombatMonsterReadyEvents& ready,NativeCombatAiActions& ai,NativeCombatAiDecisionRuntime& decisions,NativeCombatEventHandlers& handlers,
    NativeCombatPlayerManager& players,NativeCombatMonsterManager& monsters,
    std::span<NativeCombatActorSkillRuntime* const> player_skills,NativeCombatAttackActions& attacks,
    NativeCombatTaskQueue& queue,NativeCombatPlayerActionMemory& memory,NativeCombatSystemImage& image,
    const std::vector<std::unique_ptr<NativeCombatPhysicalAttackDomain>>& domains,NativeScriptEventRouter& router,
    INativeSkillSystemServices& system,const std::function<void(float)>& advance,
    const NativeCombatActorSkillRuntime::ActionMotionRefresh& motion,
    const NativeCombatAiActions::MagicResolver& magic,
    const NativeCombatAiActions::MagicResolver& stunt){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    check(!queue.Count() && !memory.Count() && !attacks.Count(),"actual AI action owners initially idle");
    std::vector<NativeCombatActorSkillRuntime*> actors(player_skills.begin(),player_skills.end());
    for(std::size_t i=0;i<monsters.Count();++i)actors.push_back(&monsters.Skills(monsters.ActorAt(i)));
    struct Restore{
        NativeCombatTaskQueue& queue;NativeScriptEventRouter& router;NativeCombatSystemImage& image;
        RandomNumberGenerator& rng;RandomNumberGenerator saved;std::uint32_t state;
        std::vector<std::uint32_t> routes;std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;
        ~Restore(){for(auto h:routes)(void)router.Unsubscribe(h);try{queue.ResetForStart();}catch(...){}
            for(auto [a,v]:fields)*reinterpret_cast<std::uint32_t*>(a)=v;image.State()=state;rng=saved;}
    } restore{queue,router,image,system.Random(),system.Random(),image.State()};
    for(auto* actor:actors){const auto a=actor->ActorAddress();
        for(auto offset:{340u,344u,352u,356u,360u,364u,368u,376u,380u,384u,416u,440u,588u,600u,604u,720u,828u,832u})
            restore.fields.emplace_back(a+offset,word(a+offset));
        for(unsigned offset=544u;offset<584u;offset+=4u)restore.fields.emplace_back(a+offset,word(a+offset));
        for(unsigned offset=632u;offset<668u;offset+=4u)restore.fields.emplace_back(a+offset,word(a+offset));
        word(a+344u)=100000u;word(a+572u)=50000u;word(a+828u)=1u;word(a+832u)=1u;
        word(a+636u)=word(a+648u)=word(a+640u)=word(a+652u)=0u;
        word(a+364u)=word(a+368u)=word(a+560u)=0u;
        word(a+376u)=word(a+380u)=word(a+600u)=std::bit_cast<std::uint32_t>(-2.f);
        word(a+384u)=std::bit_cast<std::uint32_t>(2.f);
        word(a+356u)=200u;word(a+360u)=20u;word(a+340u)=word(a+440u)=word(a+416u)=0u;
        word(a+588u)=word(a+604u)=std::bit_cast<std::uint32_t>(1.f);
    }
    image.State()=3u;
    NativeCombatActorSkillRuntime* source{};std::uint32_t seed{},turn{};
    for(std::size_t i=0;i<monsters.Count() && !source;++i){auto& candidate=monsters.Skills(monsters.ActorAt(i));
        for(unsigned count=1;count<=8u && !source;++count){word(candidate.ActorAddress()+832u)=count;
            const auto d=decisions.Choose(candidate.ActorAddress());if(!d.property)continue;
            for(unsigned value=1;value<10000u;++value){auto rng=system.Random();rng.Seed(value);
                const auto roll=static_cast<float>(static_cast<double>(rng.Next()%1000u)*static_cast<double>(.001f));
                double cumulative{};std::uint32_t selected=0xffffffffu;
                for(const auto& b:AiActionTask::kBranches){cumulative+=std::bit_cast<float>(decisions.ReadPropertyWord(d.property+b.weight_offset));
                    if(roll<=cumulative){selected=b.virtual_slot;break;}}
                if(selected==12u){source=&candidate;seed=value;turn=count;break;}
            }
        }
    }
    check(source,"actual cave AI table has a naturally selectable normal attack");
    const auto a=source->ActorAddress();word(a+832u)=turn;
    const auto position=[&]{return *reinterpret_cast<const std::array<float,3>*>(word(a+808u)+172u);};const auto home=position();
    struct LogFailure:std::runtime_error{LogFailure():std::runtime_error("AI target log fixture") {}};
    bool fail_log=true,in_ready_event{};unsigned branch_logs{},target_logs{},hits{},ends{};
    restore.routes.push_back(router.Subscribe(router.Key("caae_message"),3u,[&](std::uint32_t p){
        const auto context=std::string_view(reinterpret_cast<const char*>(word(p)));
        if(context=="cbActorAi::takeAction"){++branch_logs;bool rejected{};
            if(in_ready_event){try{ready.Close();}catch(const std::logic_error&){rejected=true;}
                check(rejected,"Monster ready callback pins Close during nested AI logging");rejected=false;}
            try{ai.Close();}catch(const std::logic_error&){rejected=true;}check(rejected,"AI action log pins Close");
            rejected=false;try{(void)ai.Process(a);}catch(const std::logic_error&){rejected=true;}check(rejected,"AI action log pins Process reentry");}
        if(context=="cbActorAi::doNormalAttackAction"){++target_logs;if(fail_log)throw LogFailure();}
    }));
    restore.routes.push_back(router.Subscribe(router.Key("ae_physical_attack"),3u,[&](std::uint32_t p){if(word(p)==a+716u)++hits;}));
    restore.routes.push_back(router.Subscribe(router.Key("ae_end_action"),3u,[&](std::uint32_t p){if(word(p)==a+716u)++ends;}));
    system.Random().Seed(seed);bool failed{};try{(void)ai.Process(a);}catch(const LogFailure&){failed=true;}
    check(failed && !queue.Count() && !attacks.Count() && !ai.PendingPoolCount() && !decisions.ActiveIterators() &&
        !players.ActiveIterators() && !monsters.ActiveIterators(),"real AI target-log failure releases pools/cursors before creating a task");
    fail_log=false;
    struct PrepareFailure:std::runtime_error{PrepareFailure():std::runtime_error("AI StartAction failure fixture") {}};
    bool fail_prepare=true;
    restore.routes.push_back(router.Subscribe(router.Key("ae_start_action"),1u,[&](std::uint32_t p){
        if(fail_prepare && word(p)==a+716u)throw PrepareFailure();
    }));
    system.Random().Seed(seed);failed=false;
    try{(void)ai.Process(a);}catch(const PrepareFailure&){failed=true;}
    fail_prepare=false;
    check(failed && !queue.Count() && !attacks.Count() && !attacks.OwnedAllocations() && !ai.PendingPoolCount() &&
        !players.ActiveIterators() && !monsters.ActiveIterators(),"failed actual AI queue preparation cancels its unpublished attack");
    source->EndAction(motion);ends=0u;image.State()=3u;
    const auto facet_before=a+716u;
    for(bool health:{false,true}){
        word(a+828u)=health?1u:0u;word(a+648u)=health?3u:0u;source->SetActionProgress(.75f);
        ready.Handle(handlers.ExtendedAddress(),reinterpret_cast<std::uint32_t>(&facet_before));
        check(source->ActionProgress()==0.f && !queue.Count() && !attacks.Count(),"original Monster not-ready branch resets actual progress without AI action");
    }
    word(a+828u)=1u;word(a+648u)=0u;
    bool rejected{};try{ready.Handle(0u,reinterpret_cast<std::uint32_t>(&facet_before));}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"foreign Monster handler owner rejected");
    const auto player_facet=player_skills.front()->ActorAddress()+716u;rejected=false;
    try{ready.Handle(handlers.ExtendedAddress(),reinterpret_cast<std::uint32_t>(&player_facet));}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"Player facet rejected by actual Monster ready consumer");
    system.Random().Seed(seed);in_ready_event=true;

    source->SetActionProgress(.75f);
    const auto facet=a+716u;
    const auto ready_count=router.Dispatch("me_can_take_action",reinterpret_cast<std::uint32_t>(&facet));
    check(ready_count>=2u && source->ActionProgress()==0.f && ai.LastBranchSlot()==12u && queue.Count()==1u && attacks.Count()==1u && !memory.Count(),
        "actual AI probability/target/factory pipeline publishes one owned CheckProtect without Player replay");
    in_ready_event=false;
    bool player_target{};for(std::size_t i=0;i<players.Count();++i)player_target|=players.ActorAt(i)==ai.LastSelectedTarget();
    check(player_target,"normal Monster AI chooses actual Player target");
    bool physical{},moved{};unsigned frames{};
    for(;frames<2400u && queue.Count();++frames){for(const auto& domain:domains)physical|=domain->Count()!=0u;
        moved|=position()!=home;advance(1.f/60.f);}
    bool hurt{};for(auto* player:player_skills)hurt|=player->CurrentStat(7u)<50000;
    check(!queue.Count() && frames<2400u && physical && moved && hits && ends && hurt,
        "real Monster AI attack executes AMF/damage/Wounded/end and naturally drains");
    for(unsigned i=0;i<3u;++i)check(std::fabs(position()[i]-home[i])<.002f,"AI source returns to its actual formation slot");
    check(!attacks.Count() && !attacks.OwnedAllocations() && !memory.Count() && !ai.PendingPoolCount(),"AI queued ownership fully retires");
    for(const auto& domain:domains)check(!domain->Count() && !domain->OwnedAllocations() && !domain->Subscriptions(),"AI physical children/subscriptions retire");
    VerifyNativeCombatAiCandidates(ai,decisions,*source,queue,image,system,router,magic,stunt,motion);
    for(auto* actor:actors)motion(actor->ActorAddress(),1u);
    std::cout<<"Native AI action: actor="<<a<<" type="<<source->AiType()<<" turn="<<turn<<" seed="<<seed
        <<" frames="<<frames<<" branch_logs="<<branch_logs<<" target_logs="<<target_logs<<" hits="<<hits<<" ends="<<ends
        <<" real me_can_take_action/57C370/progress reset/probability/weighted target/CheckProtect/AMF/HP/end, callback failure recovery and ownership cleanup pass\n";
}
}
