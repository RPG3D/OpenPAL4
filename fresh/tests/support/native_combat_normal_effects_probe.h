#pragma once
#include "pal4/native_combat_normal_attack_effects.h"
#include "pal4/native_combat_normal_attack_processor.h"
#include "pal4/native_combat_end_sound_events.h"
#include "pal4/native_combat_reaction_domain.h"
#include "pal4/native_combat_fend_off_actions.h"
#include "pal4/native_combat_miss_actions.h"
#include "pal4/native_combat_wounded_actions.h"
#include "pal4/native_combat_death_actions.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_damage_calculator.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_combat_actor_action_tasks.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_combat_fatal_visual_runtime.h"
#include "pal4/native_combat_final_hit_message.h"
#include "pal4/native_combat_fatal_children.h"
#include "pal4/native_combat_message_children.h"
#include "pal4/native_combat_physical_attack_actions.h"
#include "pal4/start_attack_task.h"
#include "pal4/physical_attack_task.h"
#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include "pal4/native_skill_numeric.h"
#include "pal4/random_number_generator.h"
#include <array>
#include <bit>
#include <vector>
#include <iostream>
namespace pal4::test {
inline void VerifyNativeCombatNormalEffects(NativeCombatActorModelOwner& model,NativeCombatActorModelOwner& source_model,
    NativeCombatActorSkillRuntime& source,NativeCombatActorSkillRuntime& target,
    NativeCombatSkillActorRegistry& registry,NativeCombatDamageCalculator& calculator,
    NativeCombatSystemImage& image,INativeSkillSystemServices& system,
    NativeEffectSceneSession& effects,NativeScriptEventRouter& router,float& scale,
    const std::function<void(float)>& advance,const NativeCombatReactionDomain::PrepareAction& prepare,
    const NativeCombatFormulaCatalog& formulas,NativeCombatEndSoundEvents& sounds,NativeCombatMonsterManager& monsters,
    bool& fail_sound){
    const auto check=[](bool v,const char* why){if(!v)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,"normal effects must reject");};
    const auto a=source.ActorAddress(),b=target.ActorAddress();
    const auto word=[](std::uint32_t p)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(p);};
    struct Restore {RandomNumberGenerator& rng;RandomNumberGenerator saved;std::uint8_t& mode;std::uint8_t old_mode;
        NativeScriptEventRouter& router;std::vector<std::uint32_t> tokens;std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;
        ~Restore(){for(auto token:tokens)(void)router.Unsubscribe(token);rng=saved;mode=old_mode;
            for(auto [p,v]:fields)*reinterpret_cast<std::uint32_t*>(p)=v;}
    } restore{system.Random(),system.Random(),image.Mode(),image.Mode(),router};
    for(auto actor:{a,b}){
        for(auto offset:{224u,344u,352u,800u,828u,1148u,1152u,364u,368u,376u,380u,384u})restore.fields.emplace_back(actor+offset,word(actor+offset));
        for(unsigned offset=544u;offset<584u;offset+=4u)restore.fields.emplace_back(actor+offset,word(actor+offset));
        for(unsigned offset=632u;offset<668u;offset+=4u)restore.fields.emplace_back(actor+offset,word(actor+offset));
        word(actor+344u)=100000u;word(actor+572u)=50000u;word(actor+828u)=1u;word(actor+648u)=0u;
        word(actor+640u)=0u;word(actor+652u)=0u;word(actor+1152u)=0u;
    }
    image.Mode()=0u;
    NativeCombatDeathActions deaths([&]{return image.Mode()!=0u;});
    NativeCombatReactionDomain reactions(model,target,registry,image,system,effects,router,scale,deaths,prepare);
    reactions.BindWounded(sounds);
    NativeCombatTaskQueue queue(deaths,system);
    const std::array<INativeCombatTaskDomain*,3> owners{&reactions.FendOff(),&reactions.Miss(),&reactions.Wounded()};queue.BindActions(owners);
    unsigned physical{},critical{};bool fail_notify{};
    std::array<std::uint32_t,19> task{};task[15]=a;task[16]=b;task[17]=std::bit_cast<std::uint32_t>(1.f);
    const auto address=reinterpret_cast<std::uint32_t>(task.data());
    NativeCombatNormalAttackEffects* active{};
    NativeCombatNormalAttackEffects runtime(registry,calculator,queue,[&](std::uint32_t t)->NativeCombatReactionDomain&{
        check(t==b,"normal reaction target");return reactions;
    });active=&runtime;
    NativeCombatNormalAttackProcessor processor(registry,runtime,formulas,sounds,monsters);
    for(bool magic:{false,true})restore.tokens.push_back(router.Subscribe(router.Key(magic?"ae_critical_attack":"ae_physical_attack"),1u,
        [&,magic](std::uint32_t payload){
            check(word(payload)==a+716u && word(payload+4u)==b,"actual attack notification participants");
            if(magic)++critical;else ++physical;
            reject([&]{(void)active->Apply(address,b,NormalAttackActorBranch::Attack);});
            if(fail_notify)throw std::runtime_error("intentional attack notification failure");
        }));
    const auto trunc=[](float v){return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(TruncateSkillFloat(v)));};
    const auto drain_wounded=[&]{for(unsigned n=0;n<600u && queue.Count();++n){advance(.016f);queue.Update(.016f);}
        check(!queue.Count() && !reactions.Count() && !reactions.OwnedAllocations() && !reactions.OwnedTexts(),"actual nonlethal Wounded naturally drains with zero ownership");};
    for(bool magic:{false,true}){
        word(a+572u)=50000u;word(b+572u)=50000u;
        system.Random().Seed(17u);const auto saved=system.Random();
        const auto expected=magic?calculator.CalculateExtended(a,b):calculator.Calculate(a,b);
        check(trunc(expected.main)<0 && trunc(expected.main)>-50000,"original damage fixture bounded");
        system.Random()=saved;
        (void)runtime.Apply(address,b,magic?NormalAttackActorBranch::Magic:NormalAttackActorBranch::Attack);
        check(target.CurrentStat(7u)==50000+trunc(expected.main) && source.CurrentStat(7u)==50000+trunc(expected.secondary),
              "combined native effects apply actual main/reflected damage exactly");
        check(queue.Count()==1u && reactions.Wounded().Count()==1u,"actual damage registers real Wounded parent");drain_wounded();
    }
    check(physical==1u && critical==1u,"actual physical and critical notification each run once");
    word(b+572u)=50000u;fail_notify=true;bool caught{};
    try{(void)runtime.Apply(address,b,NormalAttackActorBranch::Attack);}catch(const std::runtime_error&){caught=true;}
    fail_notify=false;check(caught && target.CurrentStat(7u)==50000 && !queue.Count(),"observer failure precedes HP mutation and Wounded creation");
    (void)runtime.Apply(address,b,NormalAttackActorBranch::Attack);
    check(queue.Count()==1u && reactions.Wounded().Count()==1u,"effect runtime recovers and registers real Wounded after callback exception");
    drain_wounded();
    // Actual enemy-target animation -> original sound/probability -> effects.
    // Reversible rate operands force hit0/float0/dodge1; RNG queries still execute.
    unsigned amf_attacks{};
    {
        word(a+572u)=50000u;word(b+572u)=50000u;word(b+828u)=1u;word(b+648u)=0u;
        for(auto actor:{a,b}){word(actor+364u)=0u;word(actor+368u)=0u;word(actor+560u)=0u;}
        word(a+376u)=std::bit_cast<std::uint32_t>(-2.f);word(a+384u)=std::bit_cast<std::uint32_t>(2.f);
        word(b+380u)=std::bit_cast<std::uint32_t>(-2.f);word(a+1148u)=0u;
        check(!source.SoundName(1u).empty(),"original attacker has actual attack sound text");
        NativeCombatActorActionTasks playback(source_model,system);
        NativeCombatMessageChildren messages(source,playback,
            [&]() -> NativeActorInstanceOwner& {return source_model.CurrentModel();},effects,router,scale);
        // A predecessor image fixture only; the successor factory and full physical
        // parent/292-byte child lifecycle below are the actual shared routines.
        std::array<std::uint32_t,14> predecessor{};predecessor[0]=StartAttackTask::kVtable;
        predecessor[11]=a;predecessor[12]=b;
        const auto start_address=reinterpret_cast<std::uint32_t>(predecessor.data());
        const auto refresh_source=[&](std::uint32_t actor,std::uint32_t flag){
            check(actor==a,"physical continuation refreshes actual source");source.UpdateMotion(source_model,flag,&monsters);
        };
        NativeCombatFatalChildren source_wait(playback,refresh_source);
        NativeCombatWoundedActions source_wounded(a,registry,image,source_wait,deaths,sounds);
        NativeCombatPhysicalAttackActions* active_physical{};bool fail_attack{};unsigned wounded_successors{};
        NativeCombatPhysicalAttackActions actions(a,registry,messages,[&](std::uint32_t embedded){
            reject([&]{active_physical->Close();});
            if(fail_attack)throw std::runtime_error("intentional parent attack processor failure");
            check(word(embedded+60u)==a && word(embedded+64u)==b,"actual AMF embedded attack has enemy operands");
            const auto before=target.CurrentStat(7u);const auto saved=system.Random();
            check(!calculator.CheckHit(a,b) && !calculator.CheckFloatProperty(a,b) && calculator.CheckDodge(a,b),
                  "actual original probability fixture selects physical branch");
            const auto expected=calculator.Calculate(a,b);system.Random()=saved;
            (void)processor.Process(embedded);
            check(word(embedded+68u)==std::bit_cast<std::uint32_t>(1.f),"original single-target process writes scale1");
            check(target.CurrentStat(7u)==before+trunc(expected.main),"actual AMF reaches exact native enemy HP delta");++amf_attacks;
        },[&](std::uint32_t victim,std::uint32_t cause){
            check(victim==a && cause==b,"physical state-three Wounded uses source as victim");
            ++wounded_successors;return source_wounded.Create(victim,cause);
        },refresh_source);active_physical=&actions;
        auto parent=actions.CreateSuccessor(start_address);
        check(actions.Name(parent)=="PhysicalAttack" && actions.Identity(parent)==a && actions.Processed(parent),
            "actual physical parent factory, identity and processed callback");
        reject([&]{(void)actions.ResultEvent(parent);});reject([&]{actions.Close();});
        (void)source.SetCurrentStat(3u,1);
        bool overlap{};
        for(unsigned attack_index=0u;attack_index<2u;++attack_index){
            actions.Prepare(parent);actions.BeginWaiting(parent);
            for(unsigned n=0;n<600u && !actions.Ready(parent);++n){
                source_model.Update(.016f,router);advance(.016f);(void)effects.UpdateCombatEffects(.016f);
                actions.Update(parent,.016f);if(!actions.Ready(parent) && queue.Count())overlap=true;
                queue.Update(.016f);
            }
            check(actions.Ready(parent) && amf_attacks==attack_index+1u && target.CurrentStat(7u)<50000,
                "actual physical parent naturally completes AMF enemy attack");
            const auto next=actions.ResultEvent(parent);reject([&]{(void)actions.ResultEvent(parent);});
            check(actions.Owns(next) && actions.Name(next)==(attack_index?"EndAttack":"PhysicalAttack") &&
                source.CurrentStat(3u)==0,"actual original counter3 repeat then EndAttack successor");
            actions.Cancel(parent);parent=next;
        }
        check(overlap,"real Wounded queue progresses while source physical parent is active");
        reject([&]{actions.Prepare(parent);}); // EndAttack recipe remains explicitly unbound.
        actions.Cancel(parent);
        drain_wounded();
        // Run the real parent, AMF damage and Wounded reaction in one shared
        // queue. The predecessor is still an explicit image fixture; this does
        // not claim production CheckProtect/StartAttack composition.
        queue.BindActions(actions);
        const auto attacks_before_queue=amf_attacks;
        parent=actions.CreateSuccessor(start_address);(void)queue.Register(parent);
        bool end_gate{};unsigned queued_frames{};
        try{
            for(;queued_frames<600u && queue.Count();++queued_frames){
                source_model.Update(.016f,router);advance(.016f);
                (void)effects.UpdateCombatEffects(.016f);queue.Update(.016f);
            }
        }catch(const std::logic_error& error){
            if(std::string_view(error.what())!="EndAttack execution services are not bound")throw;
            end_gate=true;
        }
        check(end_gate && queued_frames<600u && amf_attacks==attacks_before_queue+1u,
              "one shared queue executes actual physical AMF attack before explicit EndAttack gate");
        check(!queue.PendingSuccessorCount() && !actions.Count() && !actions.OwnedAllocations() &&
              !actions.OwnedTexts() && !messages.Count() && !messages.SubscriptionCount(),
              "failed EndAttack preparation retires physical predecessor, successor and AMF ownership");
        // The queue failed after real effects; cancel any remaining reactions,
        // without undoing HP/events or replaying the transferred result.
        queue.Clear();queue.DetachActions();queue.BindActions(deaths);queue.BindActions(owners);
        check(!reactions.Count() && !reactions.OwnedAllocations() && !reactions.OwnedTexts(),
              "failed shared attack queue retires remaining Wounded reactions");
        std::cout<<"Native queued physical attack: real AMF damage and Wounded, EndAttack failure with zero orphan parents/children, frames="
            <<queued_frames<<" pass\n";
        // A throwing AMF processor leaves the parent cancellable with no leaked children.
        fail_attack=true;parent=actions.CreateSuccessor(start_address);actions.Prepare(parent);actions.BeginWaiting(parent);
        bool failed{};
        try{for(unsigned n=0;n<600u && !actions.Ready(parent);++n){source_model.Update(.016f,router);
            advance(.016f);(void)effects.UpdateCombatEffects(.016f);actions.Update(parent,.016f);}}
        catch(const std::runtime_error&){failed=true;}
        check(failed,"actual parent reaches injected AMF processor failure");actions.Cancel(parent);fail_attack=false;
        // Branch after real attack completion: preserve original source-as-victim
        // Wounded factory selection; inspect and retire without claiming its death recipe.
        parent=actions.CreateSuccessor(start_address);actions.Prepare(parent);actions.BeginWaiting(parent);
        for(unsigned n=0;n<600u && !actions.Ready(parent);++n){source_model.Update(.016f,router);
            advance(.016f);(void)effects.UpdateCombatEffects(.016f);actions.Update(parent,.016f);queue.Update(.016f);}
        check(actions.Ready(parent),"physical parent completes before source-state-three branch");
        const auto source_state=word(a+648u);word(a+648u)=3u;
        const auto wound=actions.ResultEvent(parent);
        check(wounded_successors==1u && source_wounded.Owns(wound) && source_wounded.Identity(wound)==a,
            "actual physical result routes state-three source to its real Wounded factory");
        word(a+648u)=source_state;actions.Cancel(parent);source_wounded.Cancel(wound);
        source_wounded.Close();check(!source_wait.Count() && !source_wait.OwnedAllocations(),"source Wounded child ownership");
        check(!actions.Count() && !actions.OwnedAllocations() && !actions.OwnedTexts() &&
            !messages.Count() && !messages.OwnedAllocations() && !messages.OwnedTexts() && !messages.SubscriptionCount(),
            "physical parents, shared GOM and AMF children release all ownership");
        actions.Close();messages.Close();
        std::cout<<"Native physical parent: actual factory/564910/564A90, real AMF attacks, counter3 repeat -> EndAttack, source-state-three Wounded, "
            <<"unbound EndAttack gate, processor failure cancellation and zero ownership pass\n";
    }
    drain_wounded();
    word(a+1148u)=1u;
    (void)processor.Process(address);
    check(!processor.ActiveIterators() && !monsters.ActiveIterators() &&
          task[17]==std::bit_cast<std::uint32_t>(formulas.FloatAt(26)),
          "actual attack420 mode1 traverses stage roster, writes formula26 and releases iterator");
    word(a+1148u)=0u;
    // These branches now create and prepare real reaction parents in the same queue.
    word(b+572u)=50000u;word(b+828u)=1u;word(b+648u)=0u;
    task[18]=1u;(void)processor.Process(address);task[18]=0u;
    check(target.CurrentStat(7u)==49999 && queue.Count()==1u && reactions.FendOff().Count()==1u,
          "original Item branch applies minus-one then registers actual FendOff");
    for(unsigned n=0;n<600u && queue.Count();++n){advance(.016f);queue.Update(.016f);}
    check(!queue.Count() && !reactions.Count(),"native FendOff from normal effect drains");
    word(b+828u)=1u;word(b+648u)=0u;
    word(a+384u)=std::bit_cast<std::uint32_t>(-2.f);
    (void)processor.Process(address);
    check(queue.Count()==1u && reactions.Miss().Count()==1u,"original Support branch registers actual Miss");
    for(unsigned n=0;n<600u && queue.Count();++n){advance(.016f);queue.Update(.016f);}
    check(!queue.Count() && !reactions.Count() && !reactions.OwnedAllocations() && !reactions.OwnedTexts() && !reactions.Subscriptions(),
          "normal effect reaction domains drain with zero ownership");
    // Cover factory/queue semantics independently of whether this actor's
    // current attack data selects these branches during normal damage.
    image.Mode()=1u;const auto quick=reactions.Wounded().Create(b,a);
    check(reactions.Wounded().Name(quick)=="QuickWounded" && reactions.Wounded().Identity(quick)==b &&
          word(quick+44u)==a && word(quick+48u)==b && !reactions.Wounded().Processed(quick),
          "QuickWounded keeps reversed fields and victim identity");
    (void)queue.Register(quick);check(reactions.Wounded().Ready(quick),"QuickWounded original prepare is empty");queue.Update(0.f);drain_wounded();
    image.Mode()=0u;const auto failed=reactions.Wounded().Create(b,a);fail_sound=true;bool failed_sound{};
    try{(void)queue.Register(failed);}catch(const std::runtime_error&){failed_sound=true;}fail_sound=false;
    check(failed_sound && !queue.Count() && reactions.Wounded().Count()==1u && reactions.Wounded().OwnedTexts()==1u &&
          !sounds.TemporaryTexts(),"Wounded audio failure retires both temporary sound owners before child creation");
    reject([&]{reactions.Wounded().Prepare(failed);});reactions.Wounded().Cancel(failed);drain_wounded();
    const auto before_sound_failure=target.CurrentStat(7u);fail_sound=true;bool effect_sound_failed{};
    try{(void)runtime.Apply(address,b,NormalAttackActorBranch::Attack);}catch(const std::runtime_error&){effect_sound_failed=true;}
    fail_sound=false;
    check(effect_sound_failed && target.CurrentStat(7u)<before_sound_failure && !queue.Count() &&
          !reactions.Count() && !reactions.OwnedAllocations() && !reactions.OwnedTexts() && !sounds.TemporaryTexts(),
          "default Wounded registration failure cancels parent but preserves preceding actual HP write");
    (void)runtime.Apply(address,b,NormalAttackActorBranch::Attack);drain_wounded();
    word(b+828u)=4u;check(!reactions.Wounded().Create(b,a),"terminal facet gates Wounded factory before allocation");word(b+828u)=1u;
    const auto old_message=word(b+1056u);word(b+1056u)=1u;word(b+648u)=3u;
    const auto final_hit=reactions.Wounded().Create(b,a);
    check(reactions.Wounded().Name(final_hit)=="FinalHit" && reactions.Wounded().Processed(final_hit),"original factory retains true-processed FinalHit branch");
    reject([&]{reactions.Wounded().Prepare(final_hit);});
    check(word(b+828u)==1u,"unbound FinalHit rejects before recipe state changes");reactions.Wounded().Cancel(final_hit);
    {
        NativeCombatActorActionTasks playback(model,system);
        NativeCombatFatalVisualRuntime visuals(model,target,effects,reinterpret_cast<std::uint32_t>(&image));
        NativeCombatFinalHitMessage message(model,effects,router);
        NativeCombatFatalChildren children(playback,prepare);
        children.BindFinalHit({&playback,&visuals,&visuals},message);
        NativeCombatWoundedActions bound(b,registry,image,children,deaths,sounds);
        const auto old_text=word(b+1052u);const char effect_name[]="EFF6";
        word(b+1052u)=reinterpret_cast<std::uint32_t>(effect_name);
        const auto special=bound.Create(b,a);
        check(special && bound.Processed(special),"bound FinalHit uses original factory");
        bound.Prepare(special);
        check(word(b+828u)==2u && children.Count()==4u,
              "FinalHit starts original collapse, message, effect and fade recipe");
        bound.Cancel(special);
        check(!bound.Count() && !bound.OwnedAllocations() && !bound.OwnedTexts() &&
              !children.Count() && !children.OwnedAllocations() && !children.OwnedActionTexts() && !message.Count(),
              "prepared FinalHit cancellation releases all owned resources");
        bound.Close();word(b+1052u)=old_text;word(b+828u)=1u;
    }
    word(b+1056u)=old_message;word(b+648u)=0u;
    for(bool quick_kind:{false,true}){
        image.Mode()=quick_kind?1u:0u;const auto parent=reactions.Wounded().Create(b,a);
        reactions.Wounded().Prepare(parent);reactions.Wounded().BeginWaiting(parent);
        for(unsigned n=0;n<600u && !reactions.Wounded().Ready(parent);++n){advance(.016f);reactions.Wounded().Update(parent,.016f);}
        check(reactions.Wounded().Ready(parent),"actual Wounded children finish before result");
        word(b+648u)=3u;image.Mode()=quick_kind?0u:1u;
        const auto successor=reactions.Wounded().ResultEvent(parent);
        check(deaths.Owns(successor) && deaths.Name(successor)==(quick_kind?"QuickDead":"Dead") &&
              word(successor+44u)==b && word(successor+48u)==a,"Wounded result keeps fixed death type and correct victim/cause");
        reactions.Wounded().Cancel(parent);deaths.Destroy(successor);word(b+648u)=0u;drain_wounded();
    }
    word(b+828u)=4u;const auto hp=target.CurrentStat(7u);
    const auto prior_notifications=physical+critical;
    (void)runtime.Apply(address,b,NormalAttackActorBranch::Attack);
    check(target.CurrentStat(7u)==hp && !queue.Count() && physical+critical==prior_notifications,"unready target gates damage/notification/task creation");
    reject([&]{(void)runtime.Apply(address,b+4u,NormalAttackActorBranch::Attack);});
    queue.DetachActions();reactions.Close();
    std::cout<<"Native normal attack effects: Monster actual physical/extended damage, HP/reflection, notifications/260, AMF enemy attacks="<<amf_attacks
        <<", real Wounded/FendOff/Miss queues, QuickWounded/fixed death, audio failure, FinalHit gate and zero ownership pass\n";
}
}
