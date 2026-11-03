#pragma once
#include "pal4/native_combat_main_joint_selection.h"
#include "pal4/native_combat_main_joint_hint.h"
#include "pal4/native_combat_main_window.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_attack_actions.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_action_children.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include "pal4/cegui_window_pointer_events.h"
#include "pal4/native_cegui_ui_session.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <iostream>
#include <vector>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatMainJointSelection(NativeCombatMainJointSelection& selection,
    NativeCombatMainJointHint& hint,NativeCombatMainWindow& main,NativeCombatSystemImage& system,
    NativeCombatTaskQueue& queue,NativeCombatAttackActions& actions,NativeCombatActionChildren& children,
    NativeCombatActorSkillRuntime& source,std::uint32_t target,NativeScriptEventRouter& router,
    INativeSkillSystemServices& skills,NativeCeguiUiSession& ui,
    const NativeCombatActorSkillRuntime::ActionMotionRefresh& motion) {
    const auto check=[](bool b,const char* reason){if(!b)throw std::runtime_error(reason);};
    auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    auto* window=reinterpret_cast<std::uint32_t*>(main.Address());
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    const auto show=detail::Export<void (__thiscall*)(void*,bool)>(module,"?setVisible@Window@CEGUI@@QAEX_N@Z");
    check(!queue.Count() && !actions.Count(),"joint selection probe starts with actual idle tasks");
    struct Restore {
        NativeCombatMainWindow& main;NativeCombatTaskQueue& queue;NativeCombatSystemImage& system;
        INativeSkillSystemServices& skills;RandomNumberGenerator random;
        std::array<std::uint32_t,4> saved;decltype(show) show_fn;bool visible,root_visible;
        NativeScriptEventRouter& router;NativeCeguiUiSession& ui;std::vector<std::uint32_t> subscriptions;
        NativeCombatMainWindow::ClickHandler previous_click;bool replaced_click{};
        ~Restore(){if(replaced_click)main.BindClickHandler(std::move(previous_click));for(auto h:subscriptions)(void)router.Unsubscribe(h);
            queue.ResetForStart();skills.Random()=random;
            (void)ui.BindCombatMainParent(false);
            show_fn(reinterpret_cast<void*>(main.Root()),root_visible);
            auto* w=reinterpret_cast<std::uint32_t*>(main.Address());w[67]=saved[0];w[68]=saved[1];
            system.translator.words[2]=saved[2];system.translator.words[3]=saved[3];show_fn(reinterpret_cast<void*>(w[54]),visible);}
    } restore{main,queue,system,skills,skills.Random(),{window[67],window[68],system.translator.words[2],system.translator.words[3]},show,
        visible(reinterpret_cast<void*>(window[54]),true),visible(reinterpret_cast<void*>(main.Root()),true),router,ui};
    const auto actor=source.ActorAddress();const std::array pair{actor+716u,target};
    unsigned selected{},joints{};bool fail_start{};
    restore.subscriptions.push_back(router.Subscribe(router.Key("select_join_atk"),2u,[&](std::uint32_t p){selection.Handle(p);++selected;}));
    restore.subscriptions.push_back(router.Subscribe(router.Key("ae_start_action"),1u,[&](std::uint32_t p){
        if(word(p)==actor+716u && fail_start)throw std::runtime_error("intentional joint task preparation failure");
    }));
    restore.subscriptions.push_back(router.Subscribe(router.Key("ae_joint_attack"),1u,[&](std::uint32_t p){
        check(word(p)==pair[0] && word(p+4u)==target,"actual virtual556 emits original facet/target pair");
        check(queue.Count()==1u && actions.Count()==1u && !window[67] && !window[68] &&
              system.translator.words[2]==actor && system.translator.words[3]==target,
              "joint notification follows shared selection writes and real queued CheckProtect preparation");
        bool rejected=false;try{selection.Handle(reinterpret_cast<std::uint32_t>(pair.data()));}catch(const std::logic_error&){rejected=true;}
        check(rejected,"synchronous joint selection reentry rejected");++joints;
    }));
    restore.previous_click=main.ExchangeClickHandler([&](std::uint32_t e){return selection.Click(e);});restore.replaced_click=true;
    check(selection.Click(0u) && !selected && !queue.Count(),"hidden hint click consumes without dispatch");
    check(ui.BindCombatMainParent(true),"joint selection resource probe attaches actual Main to hidden test desktop");
    for(unsigned iteration=0;iteration<2u;++iteration){
        hint.Handle(reinterpret_cast<std::uint32_t>(pair.data()));
        if(!iteration){
            const auto fire=detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
            CeguiMouseEvent event{};event.vtable=reinterpret_cast<std::uint32_t>(detail::Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));event.window=main.Root();
            detail::NativeString empty(module,"");fire(reinterpret_cast<void*>(main.Root()),
                detail::Export<const void*>(module,"?EventMouseClick@Window@CEGUI@@2VString@2@B"),&event,empty.Data());main.RethrowClickError();
        }else (void)selection.ConfirmVisible();
        check(selected==iteration+1u && joints==iteration+1u && !visible(reinterpret_cast<void*>(window[54]),true),
              "actual DLL root click and direct confirmation each queue one action and hide hint");
        queue.ResetForStart();source.EndAction(motion);
        check(!actions.Count() && !actions.OwnedAllocations() && !actions.OwnedTexts() && !children.Count(),
              "queued joint parent cancellation releases actual children, names and storage");
    }
    hint.Handle(reinterpret_cast<std::uint32_t>(pair.data()));fail_start=true;bool failed{};
    try{(void)selection.ConfirmVisible();}catch(const std::runtime_error&){failed=true;}fail_start=false;
    check(failed && !queue.Count() && !actions.Count() && !actions.OwnedAllocations() && !children.Count() && joints==2u,
          "failed real CheckProtect preparation is cancelled without a joint notification or orphan");source.EndAction(motion);
    hint.Handle(reinterpret_cast<std::uint32_t>(pair.data()));
    const auto before=std::array{window[67],window[68]};
    const std::array foreign{0x1000u,target};failed=false;
    try{selection.Handle(reinterpret_cast<std::uint32_t>(foreign.data()));}catch(const std::invalid_argument&){failed=true;}
    check(failed && before==std::array{window[67],window[68]} && visible(reinterpret_cast<void*>(window[54]),true),
          "foreign joint facet rejects before UI mutation");
    const std::array<std::uint32_t,2> none{};selection.Handle(reinterpret_cast<std::uint32_t>(none.data()));
    check(!window[67] && !window[68] && !queue.Count() && joints==2u,"null actor selection hides without creating a task");
    const auto parent=detail::Export<void* (__thiscall*)(const void*)>(module,"?getParent@Window@CEGUI@@QBEPAV12@XZ");
    check(ui.BindCombatMainParent(false) && !parent(reinterpret_cast<void*>(main.Root())),"joint selection probe detaches Main after interaction");
    std::cout<<"Native combat joint selection: actual DLL click/confirm, select_join_atk, CheckProtect queue, virtual556, reentry, preparation failure and zero allocations pass\n";
}
}
