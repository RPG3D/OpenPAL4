#pragma once
#include "pal4/native_combat_actor_action_tasks.h"
#include "native_combat_action_lifecycle_probe.h"
#include "native_combat_first_roster_probe.h"
#include "native_combat_protection_children_probe.h"
#include "native_combat_action_wait_probe.h"
#include "native_combat_reaction_children_probe.h"
#include "native_combat_miss_actions_probe.h"
#include "native_combat_fend_off_impact_probe.h"
#include "native_combat_message_children_probe.h"
#include "native_combat_reaction_composition_probe.h"
#include "native_combat_damage_probe.h"
#include "native_combat_current_delta_probe.h"
#include "native_combat_attack_gates_probe.h"
#include "native_combat_normal_effects_probe.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_combat_fatal_visual_runtime.h"
#include "pal4/native_combat_final_hit_message.h"
#include "pal4/native_combat_fatal_children.h"
#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include "pal4/effect_handle.h"
#include "pal4/native_combat_death_actions.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_death_domain.h"
#include "pal4/native_combat_reaction_domain.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_actor_death_services.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_player_collection_values.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_script_event_router.h"
#include "pal4/random_number_generator.h"
#include "pal4/native_combat_actor_turn_services.h"
#include "pal4/native_combat_actor_turn_runtime.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_rw_geometry_processor.h"
#include "pal4/native_miles_digital_driver.h"
#include "pal4/native_miles_sound_2d_runtime.h"
#include "pal4/native_combat_end_sound_events.h"
#include "pal4/native_effect_creation.h"
#include "pal4/native_effect_handle_content_runtime.h"
#include <iostream>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <rpworld.h>
namespace pal4::test {
inline void VerifyNativeCombatDeath(NativeEffectSceneSession& effects,NativeScriptEventRouter& router,
    const std::filesystem::path& root,NativeCombatSystemImage& combat_image){
    const auto combat=reinterpret_cast<std::uint32_t>(&combat_image);
    const auto check=[](bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);};
    const auto word=[](unsigned a)->unsigned&{return *reinterpret_cast<unsigned*>(a);};
    NativeGameDatabase db;check(db.Load(root),"death original Monster database");
    NativeMonsterRecordCatalog records(db);NativeSkillRecordCatalog catalog(db);
    NativePlayerCollectionCatalog passives(db);NativePlayerCombatEvents events;
    RandomNumberGenerator random;NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");
    std::array<unsigned,128> game{};
    NativeActorInstancePool pool([]{return false;},[&](std::string_view name){
        auto m=std::make_unique<NativeActorInstanceOwner>();m->Construct(reinterpret_cast<unsigned>(game.data()));
        m->Load(root,name);return m;
    });
    NativeCombatSkillActorRegistry registry(router,catalog,system);
    NativeCombatFormulaCatalog formulas(db);NativeCombatDamageCalculator damage(registry,formulas);
    {
        NativeCombatMonsterManager replacement(pool,records,catalog,router,system,registry);
        const auto a=replacement.CreateMonster(2589,1u);
        auto& model=replacement.ModelOwner(a);auto& skills=replacement.Skills(a);
        skills.BindAllTargets([&](unsigned source){check(source==a,"replacement enumerates actual roster owner");return std::vector<unsigned>{a};});
        replacement.CleanupObjectLists(a,passives);(void)skills.RebuildStats();
        replacement.JoinStageAndScene(a);skills.BecomeAlive();
        (void)skills.SetCurrentStatViaActor(7u,1);
        (void)skills.SetCurrentStatViaActor(8u,40);
        (void)skills.SetCurrentStatViaActor(9u,9969);
        const std::array saved{skills.CurrentStat(7u),skills.CurrentStat(9u),skills.CurrentStat(8u)};
        const auto old_model=word(a+808u);
        const auto old_name=std::string(model.Model()->ModelName());
        std::array<unsigned,16> geometry{};std::vector<char> order;bool fail=true,missing=true;
        NativeCombatActorTurnRuntime turn(model,[&]{return reinterpret_cast<unsigned>(geometry.data());},
            [&](unsigned actor){
                if(missing)word(actor+1164u)=0x7fffffffu;
                replacement.ApplyPendingSceneRecord(actor,passives,effects);
                if(missing)check(word(actor+1164u)==0x7fffffffu && word(actor+960u)==2589u && word(actor+808u)==old_model,
                    "missing record returns without mutation or pending reset inside531F40");
            });
        const auto reject=[&](auto&& operation){bool caught=false;try{operation();}catch(const std::logic_error&){caught=true;}
            check(caught,"pending Monster record scope rejection");};
        reject([&]{replacement.ApplyPendingSceneRecord(a,passives,effects);});
        const auto leave=router.Subscribe(router.Key("me_leave_scene"),1u,[&](unsigned payload){
            if(word(payload)!=a+716u)return;
            check(word(a+960u)==2589u && word(a+808u)==old_model,"replacement leave precedes detach/copy");
            reject([&]{replacement.ApplyPendingSceneRecord(a,passives,effects);});
            reject([&]{replacement.Close();});reject([&]{skills.Close();});
            order.push_back('L');if(fail)throw std::runtime_error("replacement observer failure");
        });
        const auto join=router.Subscribe(router.Key("me_join_scene"),1u,[&](unsigned payload){
            if(word(payload)!=a+716u)return;
            check(word(a+960u)==2601u && word(a+808u) && word(a+1164u)==2601u,
                  "replacement joins new model before clearing pending ID");order.push_back('J');
        });
        word(a+832u)=2u;replacement.CanTakeAction(a,turn);
        check(order.empty() && !word(a+1164u),"outer turn clears missing pending ID without scene events");
        missing=false;word(a+832u)=2u;bool caught=false;
        try{replacement.CanTakeAction(a,turn);}catch(const std::runtime_error&){caught=true;}
        check(caught && word(a+960u)==2589u && word(a+1164u)==2601u && word(a+808u)==old_model,
              "real7752 pending selection survives failed leave observer with old model intact");
        reject([&]{replacement.ApplyPendingSceneRecord(a,passives,effects);});
        fail=false;order.clear();word(a+832u)=2u;
        auto reference=system.Random();(void)reference.Next();
        replacement.CanTakeAction(a,turn);
        check(order==std::vector<char>{'L','J'} && word(a+960u)==2601u && !word(a+1164u) &&
              model.Slot()==1u && replacement.StageCount()==1u && replacement.Count()==1u &&
              word(a+808u)==model.Model()->Address() && model.Model()->ModelName()!=old_name,
              "actual2589->2601 keeps actor/roster/slot and replaces owned model in original order");
        check(saved==std::array{skills.CurrentStat(7u),skills.CurrentStat(9u),skills.CurrentStat(8u)},
              "replacement restores live HP/MP/SP through real health setters");
        auto actual=system.Random();for(unsigned i=0;i<624u;++i)
            check(actual.Next()==reference.Next(),"only actual7752 consumes MT; replacement adds none");
        check(router.Unsubscribe(leave) && router.Unsubscribe(join),"replacement observers retired");
        replacement.ClearActors(effects);replacement.Close();
        check(!registry.Count(),"replacement record/model/skills retire through creating owners");
        std::cout<<"Native pending Monster record: actual2589->2601,7752, current stats, slot, scene order, model/skill lifetime, reentry, failure unwind and full MT pass\n";
    }
    NativeCombatMonsterManager manager(pool,records,catalog,router,system,registry);
    const auto actor=manager.CreateMonster(2579,0u);
    const auto cause=manager.CreateMonster(2580,1u);
    auto& model=manager.ModelOwner(actor);auto& skills=manager.Skills(actor);
    (void)skills.RebuildStats();model.SetSlot(0u);(void)model.RefreshResource();
    (void)manager.Skills(cause).RebuildStats();
    VerifyNativeCombatDamage(damage,skills,manager.Skills(cause),formulas,system,"Monster/Monster");
    VerifyNativeCombatCurrentDelta(damage,skills,cause,system,router,"Monster");
    VerifyNativeCombatAttackGates(damage,skills,manager.Skills(cause),formulas,system,"Monster");
    VerifyNativeCombatFirstRoster(manager,[&]{manager.ClearActors(effects);},"Monster");
    VerifyNativeCombatActionLifecycle(skills,router,[&](std::uint32_t a,std::uint32_t flag){manager.UpdateMonsterMotion(a,flag);},"Monster");
    VerifyNativeCombatProtectionChildren(skills,manager,router,"Monster");
    NativeCombatActorActionTasks animation(model,system);
    VerifyNativeCombatActionWait(animation,actor,[&](float delta){model.Update(delta,router);},"Monster",router);
    VerifyNativeCombatReactionChildren(animation,actor,[&](float delta){model.Update(delta,router);},
        [&](std::uint32_t a,std::uint32_t flag){manager.UpdateMonsterMotion(a,flag);},"Monster");
    VerifyNativeCombatMissActions(animation,manager.Skills(cause),skills,registry,combat_image,system,router,
        [&](float delta){model.Update(delta,router);},
        [&](std::uint32_t a,std::uint32_t flag){manager.UpdateMonsterMotion(a,flag);},"Monster",
        [&]{return HasNativeMissRecord(*model.Model()->Asset());});
    VerifyNativeCombatFendOffImpact(manager.Skills(cause),skills,registry,router,"Monster");
    float message_scale_fixture=1.f;
    VerifyNativeCombatMessageChildren(animation,skills,[&]() -> NativeActorInstanceOwner& {return model.CurrentModel();},
        effects,router,[&](float delta){model.Update(delta,router);},message_scale_fixture,"Monster");
    VerifyNativeCombatReactionComposition(animation,manager.Skills(cause),skills,registry,
        [&]() -> NativeActorInstanceOwner& {return model.CurrentModel();},effects,router,
        [&](float delta){model.Update(delta,router);},
        [&](std::uint32_t a,std::uint32_t flag){manager.UpdateMonsterMotion(a,flag);},message_scale_fixture,"Monster",combat_image,system);
    NativeCombatFatalVisualRuntime visuals(model,skills,effects,combat);
    manager.ModelOwner(cause).SetSlot(0u);(void)manager.ModelOwner(cause).RefreshResource();
    NativeMilesDigitalDriver driver(root.string());check(driver.Open(),"death EFF6 actual Miles driver");driver.SetMasterVolume(0);
    NativeMilesSound2dRuntime audio(root,driver);audio.SetVolume(0);effects.BindSound(&audio);
    {
        NativeScriptEventRouter message_router;
        NativeCombatFinalHitMessage message(model,effects,message_router);
        std::array<std::uint32_t,25> task{};
        const char name[]="EFF6";
        const auto address=reinterpret_cast<std::uint32_t>(task.data());
        task[16]=reinterpret_cast<std::uint32_t>(name);task[17]=4u;task[19]=actor;
        const auto subscriptions=message_router.SubscriptionCount();
        message.Start(address);
        check(message.Count()==1u && message_router.SubscriptionCount()==subscriptions+1u,
              "FinalHit message owns real effect-end subscription");
        EffectSystemHandleWords handle{task[20],task[21]};
        check(handle.system==effects.CombatEffectCreation().Group() &&
              EffectHandle::DialogNeedsUpdate(handle,effects.MutationHandles()),
              "FinalHit message creates real EFF6 in Combat group");
        std::array<std::uint32_t,6> unrelated{handle.id+1u,handle.system,1u,0u,0u,0u};
        (void)message_router.Dispatch("effect_end",reinterpret_cast<std::uint32_t>(unrelated.data()));
        check(!message.Complete(address),"unrelated effect does not finish FinalHit message");
        const std::array<std::uint32_t,6> finished{handle.id,handle.system,1u,0u,0u,0u};
        (void)message_router.Dispatch("effect_end",reinterpret_cast<std::uint32_t>(finished.data()));
        check(message.Complete(address),"matching effect completes FinalHit message");
        message.Stop(address);
        check(!message.Count() && message_router.SubscriptionCount()==subscriptions,
              "FinalHit message retires subscription and task handle");
        effects.ReleaseCombatEffect(handle);
    }
    {
        NativeScriptEventRouter message_router;
        NativeCombatFinalHitMessage message(model,effects,message_router);
        NativeCombatFatalChildren children(animation,[](std::uint32_t,std::uint32_t){});
        children.BindFinalHit({&animation,&visuals,&visuals},message);
        GomTaskBase base(children.BaseRuntime());
        GomTaskContainer queue(children.ContainerRuntime(),base);
        GomTaskContainerState state{};
        const auto owner=reinterpret_cast<std::uint32_t>(&state);
        (void)queue.Initialize(state,0u);
        const auto cold=children.Create(0x845df0u);
        children.AssignFinalMessage(cold,"EFF6");children.Destroy(cold);
        check(!children.Count() && !children.OwnedActionTexts(),"cold FinalHit cancellation releases message text");
        const auto child=children.Create(0x845df0u);
        children.AssignFinalMessage(child,"EFF6");
        word(child+76u)=actor;
        (void)queue.AddTaskAndInitialize(state,owner,children.ContainerRuntime().TaskAt(child),
                                         child,"final-message",{1u,0.f,0.f});
        (void)queue.BeginWaiting(state);(void)queue.Update(state,0.1f);
        check(message.Count()==1u && !state.ready,"FinalHit GOM starts real effect and waits");
        EffectSystemHandleWords handle{word(child+80u),word(child+84u)};
        const std::array<std::uint32_t,6> finished{handle.id,handle.system,1u,0u,0u,0u};
        (void)message_router.Dispatch("effect_end",reinterpret_cast<std::uint32_t>(finished.data()));
        for(unsigned i=0;i<4u && !state.ready;++i)(void)queue.Update(state,0.1f);
        check(state.ready && !children.Count() && !message.Count() && !message_router.SubscriptionCount(),
              "FinalHit GOM completes and retires message subscription");
        (void)queue.Destroy(state);effects.ReleaseCombatEffect(handle);
        check(!children.OwnedAllocations() && !children.OwnedActionTexts(),"FinalHit GOM releases allocations and text");
        (void)queue.Initialize(state,0u);
        const auto cancelled=children.Create(0x845df0u);children.AssignFinalMessage(cancelled,"EFF6");
        word(cancelled+76u)=actor;
        (void)queue.AddTaskAndInitialize(state,owner,children.ContainerRuntime().TaskAt(cancelled),
                                         cancelled,"cancel-final-message",{1u,0.f,0.f});
        (void)queue.BeginWaiting(state);(void)queue.Update(state,0.1f);
        EffectSystemHandleWords cancelled_handle{word(cancelled+80u),word(cancelled+84u)};
        (void)queue.Destroy(state);
        check(!children.Count() && !message.Count() && !message_router.SubscriptionCount() &&
              !children.OwnedAllocations() && !children.OwnedActionTexts(),
              "active FinalHit cancellation retires subscription and all GOM ownership");
        effects.ReleaseCombatEffect(cancelled_handle);
    }
    unsigned played{};bool fail_sample{};NativeCombatEndSoundServices sound_services;
    sound_services.acquire_combat=[&]{return combat;};
    sound_services.read_word=[&](unsigned a){return word(a);};
    sound_services.inactive=[&](unsigned c){check(c==combat,"normal attack actual Combat sound receiver");return combat_image.Mode()!=0u;};
    sound_services.text=[](unsigned)->std::string{throw std::logic_error("unexpected external sound text");};
    sound_services.special_music=[](std::string_view,float,bool){throw std::logic_error("unexpected battle-end music");};
    sound_services.restore_music=[](std::string_view){throw std::logic_error("unexpected battle-end restore");};
    sound_services.play_sound=[&](std::string_view path,unsigned mode){
        constexpr std::string_view prefix="gamedata\\palSound\\";
        check(path.starts_with(prefix) && mode==1u,"normal sound uses original path prefix and mode");
        if(fail_sample)throw std::runtime_error("intentional audio service failure");
        const auto handle=audio.PlayNamedSound(path.substr(prefix.size()),static_cast<std::int32_t>(mode));
        check(handle!=0u,"actual attack sound loads through Miles");++played;return handle;
    };
    NativeCombatEndSoundEvents sound_events(combat,combat_image.EndSoundSlot(),router,std::move(sound_services));
    const auto old_mode=combat_image.Mode();combat_image.Mode()=0u;fail_sample=true;bool failed{};
    try{(void)sound_events.PlayEffect(manager.Skills(cause).SoundName(1u));}catch(const std::runtime_error&){failed=true;}
    fail_sample=false;check(failed && !sound_events.TemporaryTexts(),"audio service error releases input and path text");
    combat_image.Mode()=1u;check(!sound_events.PlayEffect(manager.Skills(cause).SoundName(1u)) && !played &&
        !sound_events.TemporaryTexts(),"inactive Combat skips attack samples and releases input text");combat_image.Mode()=old_mode;
    VerifyNativeCombatNormalEffects(model,manager.ModelOwner(cause),manager.Skills(cause),skills,registry,damage,combat_image,system,
        effects,router,message_scale_fixture,[&](float delta){model.Update(delta,router);},
        [&](std::uint32_t a,std::uint32_t flag){manager.UpdateMonsterMotion(a,flag);},formulas,sound_events,manager,fail_sample);
    check(played && !sound_events.TemporaryTexts(),"normal attack plays real sound and retires temporary names");
    audio.StopAll();
    std::cout<<"Native normal attack sound: original actor456 text, shared579ED0, actual muted Miles samples="<<played<<" and zero temporary text pass\n";
    std::array<unsigned,4> geometry_queue{};bool world_bound=true;
    NativeCombatActorTurnRuntime turn(model,[&]{return world_bound?reinterpret_cast<unsigned>(geometry_queue.data()):0u;});
    const auto rejects=[&](auto&& operation,const char* message){
        bool rejected=false;try{operation();}catch(const std::logic_error&){rejected=true;}check(rejected,message);
    };
    rejects([&]{(void)turn.CreateDeathAction(actor);},"unbound turn death queue rejects");
    rejects([&]{turn.ApplyPendingSceneRecord(actor);},"unbound pending Monster replacement rejects");
    world_bound=false;rejects([&]{(void)turn.GeometryQueue();},"missing world queue rejects");world_bound=true;
    NativeCombatActorDeathServices services(skills,events,passives,&turn);
    unsigned deaths{};auto expected_cause=cause;
    const auto death_observer=router.Subscribe(router.Key("ae_become_dead"),1u,[&](unsigned p){
        if(word(p)!=actor+716u)return;
        check(word(p+4u)==expected_cause,"full Dead actual death event receiver/cause");++deaths;
    });
    NativeCombatDeathDomain domain(model,skills,turn,combat_image,system,passives,effects);
    auto& actions=domain.Actions();
    auto& second_model=manager.ModelOwner(cause);auto& second_skills=manager.Skills(cause);
    (void)second_skills.RebuildStats();second_model.SetSlot(0u);(void)second_model.RefreshResource();
    NativeCombatActorTurnRuntime second_turn(second_model,[&]{return reinterpret_cast<unsigned>(geometry_queue.data());});
    NativeCombatDeathDomain second_domain(second_model,second_skills,second_turn,combat_image,system,passives,effects);
    auto& second_actions=second_domain.Actions();
    NativeCombatTaskQueue queue(actions,system);
    {
        NativeCombatDeathActions other([]{return false;});
        NativeCombatTaskQueue other_queue(other,system);
        rejects([&]{turn.BindDeathQueue(actions,other_queue);},"turn rejects mismatched action/queue domain");
    }
    turn.BindDeathQueue(actions,queue);
    rejects([&]{turn.BindDeathQueue(actions,queue);},"turn rejects rebinding live dependencies");
    rejects([&]{(void)turn.CreateDeathAction(cause);},"turn rejects foreign actor before creating task");
    {
        const auto foreign=actions.Create(actor,cause);
        rejects([&]{turn.RegisterActorAction(foreign);},"turn rejects non-self death target");actions.Destroy(foreign);
    }
    struct Drain {
        NativeCombatTaskQueue& queue;NativeEffectSceneSession& effects;RpClump* clump;
        NativeScriptEventRouter& router;unsigned observer;
        ~Drain(){if(observer)(void)router.Unsubscribe(observer);try{queue.Clear();effects.ReleaseArenaEffects();effects.BindSound(nullptr);if(clump)NativeRwGeometryProcessor::RestoreClump(clump);}catch(...){}}
    } drain{queue,effects,model.Model()->Asset()->Main(),router,death_observer};
    // An incomplete active-mode binding must reject before any recipe effects.
    {
        NativeCombatDeathActions partial([]{return false;},services,{&animation,&visuals,nullptr});
        const auto p=partial.Create(actor,cause);const auto before=word(actor+828u);bool rejected=false;
        try{partial.Start(p);}catch(const std::logic_error&){rejected=true;}
        check(rejected && word(actor+828u)==before && !partial.OwnedChildren(),"missing fade binding rejects active Dead before mutation");
        partial.Destroy(p);
    }
    // Two actual Monsters share one queue. Reset preserves ownership until
    // Cleanup, then both parents and all ten prepared children retire.
    queue.BindActions(second_actions);second_turn.BindDeathQueue(second_actions,queue);
    rejects([&]{queue.BindActions(second_actions);},"duplicate task domain rejected");
    const auto first_reset=actions.Create(actor,cause),second_reset=second_actions.Create(cause,actor);
    (void)queue.Register(first_reset);(void)queue.Register(second_reset);
    NativeCombatDeathActions foreign_actions([]{return true;});
    rejects([&]{queue.BindActions(foreign_actions);},"nonempty queue rejects new domain");
    const auto foreign_task=foreign_actions.Create(actor,cause);
    bool foreign_rejected=false;try{(void)queue.Register(foreign_task);}catch(const std::invalid_argument&){foreign_rejected=true;}
    check(foreign_rejected && queue.Count()==2u,"unknown action rejected without queue mutation");
    foreign_actions.Destroy(foreign_task);
    rejects([&]{queue.DetachActions();},"nonempty queue retains borrowed owners");
    queue.ResetFlags();
    check(queue.Count()==2u && actions.OwnedChildren()==5u && second_actions.OwnedChildren()==5u,
          "shared reset flags preserves both actual prepared recipes");
    queue.ResetForStart();
    check(!queue.Count() && !actions.Count() && !second_actions.Count() &&
          !actions.OwnedAllocations() && !second_actions.OwnedAllocations() &&
          !actions.OwnedTexts() && !second_actions.OwnedTexts(),"shared Start reset balances both task domains");
    // Execute two original QuickDead chains through their separate real owners.
    unsigned second_deaths{};
    const auto second_observer=router.Subscribe(router.Key("ae_become_dead"),1u,[&](unsigned p){
        if(word(p)==cause+716u){check(word(p+4u)==actor,"second domain keeps its own death cause");++second_deaths;}
        rejects([&]{queue.ResetForStart();},"queue rejects callback reentry");
    });
    combat_image.Mode()=1u;
    (void)queue.Register(actions.Create(actor,cause));(void)queue.Register(second_actions.Create(cause,actor));
    for(unsigned tick=0;tick<30u && queue.Count();++tick)queue.Update(0.1f);
    check(!queue.Count() && deaths==1u && second_deaths==1u && !actions.Count() && !second_actions.Count(),
          "shared queue completes both real QuickDead domains");
    check(router.Unsubscribe(second_observer),"second task domain observer retired");
    combat_image.Mode()=0u;skills.BecomeAlive();second_skills.BecomeAlive();deaths=0u;
    queue.DetachActions();
    check(!queue.DomainCount() && !queue.UsesActions(actions) && !queue.UsesActions(second_actions),
          "drained queue detaches every borrowed domain before actor retirement");
    queue.BindActions(actions);queue.BindActions(second_actions);
    std::cout<<"Native shared death queue: two actual Monsters, reset/cancel, owner routing, completion and reentry pass\n";
    const auto cancelled=actions.Create(actor,cause);(void)queue.Register(cancelled);queue.Clear();
    check(!actions.Count() && !actions.OwnedTexts() && !actions.OwnedAllocations() && !deaths && !effects.CombatActiveCount(),
          "cancelled active Dead retires all five prepared children without executing death/effect");
    (void)skills.SetCurrentStat(7u,0);
    const auto action=actions.Create(actor,cause);
    check(actions.Name(action)=="Dead","active mode chooses real Dead parent");
    (void)queue.Register(action);
    check(actions.OwnedChildren()==5u && word(actor+804u)==2u,"original Monster Dead includes both final children");
    const std::array<unsigned,5> expected{0x845df4u,0x845d3cu,0x845e10u,0x845e2cu,0x845e48u};
    auto node=word(word(action+8u));
    for(auto vt:expected){
        const auto child=word(node+8u);check(word(child)==vt,"original five-child order");
        if(vt==0x845d3cu)check(std::string(reinterpret_cast<const char*>(word(child+68u)))=="z12","original global collapse name");
        if(vt==0x845e2cu){check(std::bit_cast<float>(word(child+16u))==1.5f &&
            std::bit_cast<float>(word(child+20u))==1.0f,"original fade wait1.5 duration1");}
        node=word(node);
    }
    unsigned materials{},faded{},minimum=255u;
    const auto clump=model.Model()->Asset()->Main();
    for(unsigned tick=0;tick<100u && queue.Count();++tick){
        queue.Update(0.1f);
        if(effects.CombatActiveCount()){
            check(tick>=2u,"death effect follows data/action child sequencing");
            // EFF6 remains owned by the actual Combat group after local reset.
        }
        struct Scan {unsigned& materials;unsigned& faded;unsigned& minimum;} scan{materials,faded,minimum};
        RpClumpForAllAtomics(clump,[](RpAtomic* a,void* p){
            auto& s=*static_cast<Scan*>(p);auto* geometry=RpAtomicGetGeometry(a);
            if(geometry)for(int i=0;i<RpGeometryGetNumMaterials(geometry);++i){
                const auto alpha=RpMaterialGetColor(RpGeometryGetMaterial(geometry,i))->alpha;
                ++s.materials;if(alpha<255u)++s.faded;s.minimum=std::min(s.minimum,unsigned(alpha));
            }return a;
        },&scan);
    }
    check(materials && faded && minimum==0u,"actual SDK B01 material alpha reaches zero through FadeOff ticks");
    check(!queue.Count() && deaths==1u && word(actor+828u)==4u && word(actor+648u)==3u,
          "actual five-child Dead queue reaches native Monster544 once");
    check(!actions.Count() && !actions.OwnedChildren() && !actions.OwnedAllocations() && !actions.OwnedTexts(),
          "full Dead parent/child/text/node/sentinel ownership balances");
    check(effects.CombatActiveCount()==1u,"original EFF6 lives in Combat group after temporary handle reset");
    const auto group=effects.CombatEffectCreation().Group();
    const auto effect_node=word(word(group+16u));
    const auto dialog=word(effect_node+16u);
    const auto f=[&](unsigned offset){return std::bit_cast<float>(word(dialog+offset));};
    const auto child=model.Model()->Address();
    const auto scale=std::bit_cast<float>(word(child+620u))*0.025f;
    check(f(176u)==-92.0f && f(180u)==0.0f && f(184u)==-102.0f &&
          std::fabs(f(188u)-scale)<0.0001f && std::fabs(f(192u)-scale)<0.0001f && std::fabs(f(196u)-scale)<0.0001f,
          "actual EFF6 Dialog retains actor position and original bounds-derived scale");
    effects.UpdateCombatEffects(0.04);
    effects.ClearCombatEffects();check(!effects.CombatActiveCount(),"actual death effect retires through group");
    // Exercise the production530F80 ->52B8D0 -> factory -> shared queue path.
    // No direct action creation/registration drives this second death.
    skills.BecomeAlive();expected_cause=actor;
    (void)skills.SetCurrentStat(7u,0);
    unsigned can_act{};
    const auto can_observer=router.Subscribe(router.Key("me_can_take_action"),1u,[&](unsigned){++can_act;});
    manager.CanTakeAction(actor,turn);
    check(queue.Count()==1u && actions.OwnedChildren()==5u && !can_act && deaths==1u,
          "real dead turn automatically prepares five children and suppresses living action notification");
    for(unsigned tick=0;tick<100u && queue.Count();++tick)queue.Update(0.1f);
    check(!queue.Count() && deaths==2u && word(actor+828u)==4u && !can_act &&
          !actions.Count() && !actions.OwnedChildren() && !actions.OwnedTexts() && !actions.OwnedAllocations(),
          "production turn reaches SDK death pipeline with self cause and retires all tasks");
    check(router.Unsubscribe(can_observer),"automatic turn observer retires");
    check(effects.CombatActiveCount()==1u,"automatic turn owns actual second EFF6 playback");
    effects.ClearCombatEffects();
    // Use actual FendOff/Wounded result dispatch to register and execute death in the
    // same shared queue. No direct death creation/registration drives this path.
    for(bool wounded:{false,true})for(bool quick:{false,true}){
        NativeRwGeometryProcessor::RestoreClump(drain.clump);
        skills.BecomeAlive();(void)skills.SetCurrentStat(7u,0);
        check(skills.IsStateThree(),"zero HP derives actual secondary state three before reaction");
        expected_cause=cause;const auto prior_deaths=deaths;
        combat_image.Mode()=quick?1u:0u;
        const auto prepare=[&](unsigned a,unsigned flag){manager.UpdateMonsterMotion(a,flag);};
        NativeCombatReactionDomain reaction_domain(model,skills,registry,combat_image,system,effects,router,
            message_scale_fixture,actions,prepare);
        if(wounded)reaction_domain.BindWounded(sound_events);
        NativeCombatTaskQueue reaction_queue(actions,system);
        const std::array<INativeCombatTaskDomain*,2> owners{&reaction_domain.FendOff(),&reaction_domain.Miss()};
        reaction_queue.BindActions(owners);
        if(wounded)reaction_queue.BindActions(reaction_domain.Wounded());
        const auto parent=wounded?reaction_domain.Wounded().Create(actor,cause):reaction_domain.FendOff().Create(cause,actor);
        if(wounded)check(reaction_domain.Wounded().Name(parent)==(quick?"QuickWounded":"WoundedTask"),
                         "original Monster data selects actual Wounded path without overriding FinalHit data");
        check(reaction_queue.Register(parent)==parent,"actual reaction parent registers before successor");
        rejects([&]{reaction_domain.Close();},"reaction domain cannot close while queued");
        // Parent kind must survive a mode change before result virtual4 runs.
        combat_image.Mode()=quick?0u:1u;
        bool successor_seen{};unsigned ticks{};
        for(;ticks<600u && reaction_queue.Count();++ticks){
            model.Update(.016f,router);reaction_queue.Update(.1f);
            if(actions.Count()){
                successor_seen=true;
                check(!reaction_domain.Count() && actions.OwnedChildren()==(quick?2u:5u),
                      "queue retires reaction then prepares fixed QuickDead/Dead recipe");
            }
        }
        check(ticks<600u && successor_seen && deaths==prior_deaths+1u &&
              word(actor+828u)==4u && skills.IsStateThree(),
              "reaction result queue executes real Monster544 exactly once with original cause");
        check(!reaction_queue.Count() && !reaction_domain.Count() &&
              !reaction_domain.OwnedAllocations() && !reaction_domain.OwnedTexts() && !reaction_domain.Subscriptions() &&
              !actions.Count() && !actions.OwnedChildren() && !actions.OwnedAllocations() && !actions.OwnedTexts(),
              "complete reaction/death queue retires both domains and all children");
        check(effects.CombatActiveCount()==(quick?0u:1u),"only active reaction death successor creates original EFF6");
        effects.ClearCombatEffects();reaction_queue.DetachActions();reaction_domain.Close();reaction_domain.Close();
        rejects([&]{(void)reaction_domain.FendOff();},"closed reaction domain rejects action lookup");
        std::cout<<"Native "<<(wounded?"Wounded":"FendOff")<<" death chain: Monster "
            <<(wounded?(quick?"QuickWounded -> QuickDead":"Wounded -> Dead"):(quick?"QuickFendOff -> QuickDead":"FendOff -> Dead"))
            <<" ticks="<<ticks<<", real zero-HP state, mode switch, queue transfer, Monster544/cause, EFF6 gate and zero ownership pass\n";
    }
    {
        NativeRwGeometryProcessor::RestoreClump(drain.clump);
        skills.BecomeAlive();(void)skills.SetCurrentStat(7u,0);
        expected_cause=cause;const auto previous_deaths=deaths;
        const auto old_marker=word(actor+1056u),old_text=word(actor+1052u);
        const char effect_name[]="EFF6";
        word(actor+1056u)=1u;word(actor+1052u)=reinterpret_cast<std::uint32_t>(effect_name);
        combat_image.Mode()=0u;
        const auto prepare=[&](unsigned a,unsigned flag){manager.UpdateMonsterMotion(a,flag);};
        NativeCombatReactionDomain reactions(model,skills,registry,combat_image,system,effects,router,
            message_scale_fixture,actions,prepare);
        reactions.BindWounded(sound_events);reactions.BindFinalHit(model,effects,router);
        NativeCombatTaskQueue final_queue(actions,system);final_queue.BindActions(reactions.Wounded());
        const auto final_hit=reactions.Wounded().Create(actor,cause);
        check(final_hit && reactions.Wounded().Processed(final_hit),"actual Monster selects FinalHit");
        (void)final_queue.Register(final_hit);
        unsigned ticks{};
        for(;ticks<1200u && final_queue.Count();++ticks){
            model.Update(.016f,router);effects.UpdateCombatEffects(.1);final_queue.Update(.1f);
        }
        check(ticks<1200u && !final_queue.Count() && deaths==previous_deaths+1u && word(actor+828u)==4u,
              "FinalHit naturally completes and executes Monster death exactly once");
        check(!reactions.Count() && !reactions.OwnedAllocations() && !reactions.OwnedTexts() && !reactions.Subscriptions(),
              "FinalHit queue retires parent, children, text and subscriptions");
        word(actor+1056u)=old_marker;word(actor+1052u)=old_text;
        effects.ClearCombatEffects();final_queue.DetachActions();reactions.Close();
        std::cout<<"Native FinalHit: real Monster, effect-end, GOM and actor172/544 complete in "<<ticks<<" ticks\n";
    }
    // EFF6's cached Dialog retains its real SoundEffect owner until pool/cache
    // retirement, even after its active handle has drained.
    sound_events.Close();
    effects.ReleaseArenaEffects();
    effects.BindSound(nullptr);audio.StopAll();
    NativeRwGeometryProcessor::RestoreClump(drain.clump);drain.clump=nullptr;
    check(router.Unsubscribe(drain.observer),"full Dead event observer retired");drain.observer=0u;
    manager.Close();pool.Clear();system.Destroy();
    std::cout<<"Native active Dead: original Monster2579/B01, five children, EFF6 group playback, SDK alpha0, Monster544, automatic turn death registration and cleanup pass\n";
}
}
