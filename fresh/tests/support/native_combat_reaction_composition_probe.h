#pragma once
#include "native_combat_fend_off_actions_probe.h"
#include "pal4/native_combat_reaction_children.h"
#include "pal4/native_combat_actor_reaction_services.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_actor_action_tasks.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_effect_creation.h"
#include "pal4/effect_handle.h"
#include "pal4/combat_fend_off_sequence_task.h"
#include "pal4/combat_action_task_names.h"
#include "pal4/combat_global_strings.h"
#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include <array>
#include <bit>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
class ReactionRecipeProbe final : public IFendOffStartRuntime {
public:
    NativeCombatReactionChildren& children;NativeCombatActorSkillRuntime& target;
    GomTaskContainer& container;GomTaskContainerState& state;std::uint32_t parent;
    NativeCombatFatalChildren::PrepareAction prepare;std::uint32_t message{};
    ReactionRecipeProbe(NativeCombatReactionChildren& c,NativeCombatActorSkillRuntime& t,
        GomTaskContainer& g,GomTaskContainerState& s,std::uint32_t p,NativeCombatFatalChildren::PrepareAction f)
        :children(c),target(t),container(g),state(s),parent(p),prepare(std::move(f)){}
    std::uint32_t ReadWord(std::uint32_t a)override{
        for(const auto& binding:CombatActionTaskNames::kBindings)
            if(binding.buffer+4u==a)return reinterpret_cast<std::uint32_t>(binding.text.data());
        return children.ContainerRuntime().ReadWord(a);
    }
    void WriteWord(std::uint32_t a,std::uint32_t v)override{children.ContainerRuntime().WriteWord(a,v);}
    bool ActorCheckStateThree(std::uint32_t a)override{if(a!=target.ActorAddress())throw std::logic_error("reaction recipe target");return target.IsStateThree();}
    void SetActorState(std::uint32_t a,std::uint32_t state,std::uint32_t notify)override{
        if(a!=target.ActorAddress())throw std::logic_error("reaction state target");target.SetFacetState(state,notify!=0u);
    }
    void ActorVirtual396(std::uint32_t a,std::uint32_t flag)override{prepare(a,flag);}
    std::uint32_t CreateChildTask(std::uint32_t factory)override{
        const auto task=children.Create(factory);if(factory==0x845a84u)message=task;return task;
    }
    std::uint32_t AddTaskAndInitialize(std::uint32_t address,std::uint32_t child,std::uint32_t text,
        const std::array<std::uint32_t,3>& timing)override{
        if(address!=parent+4u || timing!=std::array<std::uint32_t,3>{1u,0u,0u})throw std::logic_error("reaction original container/timing");
        const auto* name=text==0x841964u?"":reinterpret_cast<const char*>(text);
        return container.AddTaskAndInitialize(state,address,children.ContainerRuntime().TaskAt(child),child,name,
            {timing[0],std::bit_cast<float>(timing[1]),std::bit_cast<float>(timing[2])});
    }
    void AssignText(std::uint32_t destination,std::uint32_t source,std::uint32_t first,std::uint32_t count)override{
        if(source!=0x8fa32cu || first || count!=0xffffffffu)throw std::logic_error("reaction original z05 source");
        children.AssignAction(destination-64u,CombatGlobalStrings::kFendOffAnimation);
    }
};
inline void VerifyNativeCombatReactionComposition(NativeCombatActorActionTasks& playback,
    NativeCombatActorSkillRuntime& source,NativeCombatActorSkillRuntime& target,
    NativeCombatSkillActorRegistry& registry,NativeCombatMessageChildren::Model model,
    NativeEffectSceneSession& effects,NativeScriptEventRouter& router,
    const std::function<void(float)>& advance,NativeCombatFatalChildren::PrepareAction prepare,
    float& scale,const char* label,NativeCombatSystemImage& image,INativeSkillSystemServices& system){
    const auto check=[](bool v,const char* why){if(!v)throw std::runtime_error(why);};
    const auto frame=[&](float delta){advance(delta);(void)effects.UpdateCombatEffects(delta);};
    const auto reject=[&](auto&& f,const char* why="mixed reaction gate must reject"){bool failed{};try{f();}catch(const std::logic_error&){failed=true;}check(failed,why);};
    const auto a=source.ActorAddress(),b=target.ActorAddress();
    const auto hp=target.CurrentStat(7u),source_hp=source.CurrentStat(7u);
    const auto prior=playback.GetAnimationName(b);
    auto& facet=*reinterpret_cast<std::uint32_t*>(b+828u);auto& health=*reinterpret_cast<std::uint32_t*>(b+648u);
    std::array<std::uint32_t,4> text_header{};const auto header=reinterpret_cast<std::uint32_t>(text_header.data());
    NativePlayerSlotCopyRuntime text;
    struct Restore {NativeCombatActorActionTasks& playback;NativePlayerSlotCopyRuntime& text;
        std::uint32_t actor,header;std::string prior;std::uint32_t& facet;std::uint32_t& health;std::uint32_t old_facet,old_health;
        ~Restore(){try{text.AssignText(header,prior);playback.SetActorTarget(actor,header,1u);}catch(...){}text.ReleaseText(header);facet=old_facet;health=old_health;}
    } restore{playback,text,b,header,prior,facet,health,facet,health};
    facet=1u;health=0u; // Reversible pre-Entry fixture for real396, as in existing reaction probes.
    unsigned events{},immediate{},prepared{},attempts{},model_checks{};bool fail_prepare{},observe_model{};
    NativeCombatReactionChildren* active{};
    auto prepare_tracked=[&](std::uint32_t actor,std::uint32_t flag){
        check(actor==b && flag<=1u,"mixed recipe actual396 receiver/flag");
        if(flag){++attempts;if(fail_prepare)throw std::runtime_error("intentional mixed prepare failure");++prepared;}else ++immediate;
        reject([&]{active->Close();},"mixed prepare callback must reject Close");prepare(actor,flag);
    };
    auto model_tracked=[&]() -> NativeActorInstanceOwner& {
        if(observe_model){++model_checks;reject([&]{active->Close();},"mixed model callback must reject Close");}
        return model();
    };
    NativeCombatActorReactionServices impact(registry);
    NativeCombatReactionChildren children(impact,target,playback,prepare_tracked,model_tracked,effects,router,scale);active=&children;
    const auto baseline=router.SubscriptionCount();
    const auto token=router.Subscribe(router.Key("ae_fend_offed"),1u,[&](std::uint32_t payload){
        const auto* p=reinterpret_cast<const std::uint32_t*>(payload);if(p[0]!=a+716u)return;
        check(p[1]==b,"mixed real508 payload");++events;
        reject([&]{active->Close();},"mixed event callback must reject Close");reject([&]{(void)active->Create(0x845a84u);},"mixed event callback must reject Create");
    });
    struct Unsubscribe {NativeScriptEventRouter& router;std::uint32_t token;~Unsubscribe(){(void)router.Unsubscribe(token);}} unsubscribe{router,token};
    // Only a parent image fixture: actual factory/result/queue registration is
    // deliberately not claimed by this shared568E30 composition test.
    std::array<std::uint32_t,13> parent{};const auto address=reinterpret_cast<std::uint32_t>(parent.data());
    parent[11]=a;parent[12]=b;auto& state=*reinterpret_cast<GomTaskContainerState*>(address+4u);
    GomTaskBase base(children.BaseRuntime());GomTaskContainer container(children.ContainerRuntime(),base);
    (void)container.Initialize(state,0u);
    struct Cleanup {GomTaskContainer& container;GomTaskContainerState& state;bool active{true};~Cleanup(){if(active)(void)container.Destroy(state);}} cleanup{container,state};
    ReactionRecipeProbe recipe(children,target,container,state,address,prepare_tracked);
    const auto schedule=[&]{
        observe_model=true;(void)FendOffSequenceTask::Start(address,recipe);
        check(state.count==3u && children.Count()==3u,"shared FendOff recipe appends actual three child kinds");
        reject([&]{children.Close();});(void)container.BeginWaiting(state);
    };
    unsigned frames{};schedule();
    for(;frames<600u && state.count;++frames){frame(.016f);(void)container.Update(state,.016f);}
    if(state.count || !state.ready || children.OwnedAllocations()!=1u || events!=1u || prepared!=1u || immediate!=1u)
        std::cerr<<"Mixed reaction diagnostic: "<<label<<" frames="<<frames<<" count="<<state.count
            <<" ready="<<unsigned(state.ready)<<" children="<<children.Count()<<" allocations="<<children.OwnedAllocations()
            <<" texts="<<children.OwnedTexts()<<" subscriptions="<<children.Subscriptions()<<" effects="<<effects.CombatActiveCount()
            <<" events="<<events<<" prepared="<<prepared<<" immediate="<<immediate<<" action="<<playback.GetAnimationName(b)<<'\n';
    check(frames<600u && state.ready && !children.Count() && children.OwnedAllocations()==1u &&
          !children.OwnedTexts() && !children.Subscriptions() && events==1u && prepared==1u && immediate==1u,
          "mixed actual event/z05/prepare sequence naturally retires to sentinel only");
    schedule();fail_prepare=true;bool caught{};
    for(unsigned n=0;n<600u && !caught;++n){frame(.016f);try{(void)container.Update(state,.016f);}catch(const std::runtime_error&){caught=true;}}
    check(caught && state.count==3u && events==2u && attempts==2u,"mixed preparation failure retains retryable GOM");
    fail_prepare=false;for(unsigned n=0;n<8u && state.count;++n)(void)container.Update(state,.016f);
    check(!state.count && events==2u && prepared==2u && attempts==3u && children.OwnedAllocations()==1u,
          "mixed retry does not repeat impact or leak cross-owner names");
    schedule();
    const auto held=effects.CombatEffectCreation().Create("foot_2",{0,1,1},1,0u);
    *reinterpret_cast<EffectSystemHandleWords*>(recipe.message+96u)={held.id,held.system};
    check(held.system && EffectHandle::DialogNeedsUpdate({held.id,held.system},effects.ContentHandles()),
          "mixed cancellation uses an actual live effect slot fixture");
    for(unsigned n=0;n<10u && !children.Subscriptions();++n){frame(.016f);(void)container.Update(state,.016f);}
    check(children.Subscriptions()==1u,"mixed cancellation reaches active216-byte message task");
    (void)container.CleanupAndReady(state);
    check(!children.Count() && !children.Subscriptions() && !children.OwnedTexts() && children.OwnedAllocations()==1u &&
          !EffectHandle::DialogNeedsUpdate({held.id,held.system},effects.ContentHandles()),
          "mixed cancellation retires all child types in the common domain");
    (void)container.Destroy(state);cleanup.active=false;
    check(!children.OwnedAllocations() && model_checks && router.SubscriptionCount()==baseline+1u &&
          source.CurrentStat(7u)==source_hp && target.CurrentStat(7u)==hp,"mixed memory/subscriptions balance and unchanged HP");
    children.Close();children.Close();reject([&]{(void)children.Create(0x845d10u);});
    observe_model=false;
    NativeCombatReactionChildren parent_children(impact,target,playback,prepare_tracked,model_tracked,effects,router,scale);
    active=&parent_children;observe_model=true;
    VerifyNativeCombatFendOffActions(source,target,registry,image,system,parent_children,frame,prepare_tracked,label);
    parent_children.Close();
    std::cout<<"Native combat mixed reaction: "<<label<<" shared568E30 actual68/216/64 children frames="<<frames
        <<", common GOM allocation domain, real508/z05/396, retry, active cancellation and zero ownership pass\n";
}
}
