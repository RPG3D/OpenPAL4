#pragma once
#include "pal4/native_player_scene_frame.h"
#include "pal4/ui_text_image_animation.h"
#include "pal4/native_player_manager_session.h"
#include "pal4/native_ui_text_image_activities.h"
#include "pal4/native_ui_text_image_overlay.h"
#include "pal4/native_cegui_ui_session.h"
#include <rwcore.h>
#include <bit>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
inline void VerifyNativeScenePlayerAnimations(NativePlayerSceneFrame& frame,NativePlayerManagerSession& players,
    NativeBaseObjectMemory& memory,NativeUiTextImageOverlay& overlay,NativeCeguiUiSession& ui,std::uint32_t camera){
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto reject=[&](auto&& f){bool rejected{};try{f();}catch(const std::logic_error&){rejected=true;}check(rejected,"scene Player callback owner/reentry rejection");};
    const auto allocated=memory.OutstandingAllocations(),texts=overlay.OwnedTexts();
    NativeUiTextImageActivities backend(overlay,[&](std::uint32_t a){(void)memory.Free(a);});
    class Activity final : public IAnimationEventQueueActivityRuntime {
        IAnimationEventQueueActivityRuntime& backend;std::uint32_t camera;
    public:
        std::vector<std::uint32_t> calls;std::function<void()> hook;
        Activity(IAnimationEventQueueActivityRuntime& b,std::uint32_t c):backend(b),camera(c){}
        void DeleteAnimation(std::uint32_t a)override{backend.DeleteAnimation(a);}
        void AdvanceAnimation(std::uint32_t a,float d)override{backend.AdvanceAnimation(a,d);}
        std::uint32_t AnimationFinished(std::uint32_t a)override{return backend.AnimationFinished(a);}
        void CallAnimationEvent(std::uint32_t a,std::uint32_t c)override{if(c!=camera)throw std::logic_error("scene activity context changed");calls.push_back(a);if(hook)hook();backend.CallAnimationEvent(a,c);}
    } activity(backend,camera);
    NativeAnimationEventQueueRuntime queue(memory,activity);
    std::vector<std::uint32_t> expected;std::uint32_t first_active{};
    for(auto& slot:players.State().slots){
        const auto address=reinterpret_cast<std::uint32_t>(&slot);
        check(!queue.ReadWord(address+0xaa8u+8u) && !queue.ReadWord(address+0xaa8u+56u),"scene Player animation probe starts with idle queues");
        if(!first_active && slot.base.Read(0x1f0u))first_active=address;
    }
    check(first_active,"scene Player animation probe has actual active player");
    auto& clump=*reinterpret_cast<std::uint32_t*>(first_active+20u);const auto original_clump=clump;
    bool model_failed{};
    {struct Restore{std::uint32_t& value;std::uint32_t saved;~Restore(){value=saved;}} restore{clump,original_clump};
        clump^=4u;try{(void)frame.Render(camera);}catch(const std::logic_error&){model_failed=true;}}
    check(model_failed,"actual model binding failure propagates");
    {auto lease=players.BorrowFrame();} // A failed Render must release its session lease.
    const bool queued=ui.OverlayQueueEnabled();bool begun{};
    struct Cleanup{NativePlayerManagerSession& p;IAnimationEventQueueRuntime& q;NativeCeguiUiSession& ui;std::uint32_t camera;bool& begun;bool queued;
        ~Cleanup(){try{for(auto& s:p.State().slots)(void)AnimationEventQueue::Clear(reinterpret_cast<std::uint32_t>(&s)+0xaa8u,q);
            ui.ClearQueuedOverlays();ui.SetOverlayQueueEnabled(queued);if(begun)RwCameraEndUpdate(reinterpret_cast<RwCamera*>(camera));}catch(...){}}
    } cleanup{players,queue,ui,camera,begun,queued};
    for(std::size_t i=0;i<4u;++i){
        auto& slot=players.State().slots[i];const auto address=reinterpret_cast<std::uint32_t>(&slot);
        struct Pending{NativeBaseObjectMemory& m;IAnimationEventQueueActivityRuntime& a;std::uint32_t value;bool constructed{};
            ~Pending(){if(value){if(constructed)a.DeleteAnimation(value);else(void)m.Free(value);}}} pending{memory,backend,memory.Allocate(72u)};
        (void)UiTextImageAnimation::ConstructMotion(pending.value,0u,overlay);pending.constructed=true;
        (void)UiTextImageOverlay::SetPosition(pending.value,std::bit_cast<std::uint32_t>(80.f+60.f*i),std::bit_cast<std::uint32_t>(80.f),overlay);
        UiTextImageOverlay::SetImage(pending.value,"zi0","luokong",0x3f800000u,overlay);
        (void)AnimationEventQueue::Add(address+0xaa8u,1u,pending.value,queue);
        if(slot.base.Read(0x1f0u))expected.push_back(pending.value);pending.value=0u;
    }
    bool fail=true;activity.hook=[&]{reject([&]{players.Destroy();});reject([&]{(void)frame.Render(camera);});reject([&]{(void)frame.ProcessCameraPass(camera);});
        reject([&]{(void)frame.DispatchRelatedAnimations(camera,activity);});if(fail)throw std::runtime_error("intentional scene Player activity failure");};
    bool failed{};try{(void)frame.DispatchRelatedAnimations(camera,activity);}catch(const std::runtime_error& e){failed=std::string_view(e.what())=="intentional scene Player activity failure";}
    check(failed && players.Constructed(),"activity exception preserves actual Player manager and queue ownership");
    fail=false;activity.calls.clear();
    check(RwCameraBeginUpdate(reinterpret_cast<RwCamera*>(camera)),"scene Player activity camera begin");begun=true;
    ui.ClearQueuedOverlays();ui.SetOverlayQueueEnabled(true);const auto quads=ui.QuadCalls();
    check(frame.DispatchRelatedAnimations(camera,activity)==expected.size() && activity.calls==expected,"original4414C0 visits only actual active slots in order");
    check(ui.QuadCalls()>=quads+expected.size() && ui.RenderQueuedOverlays(),"real active Player images submit through original DLL renderer");
    check(RwCameraEndUpdate(reinterpret_cast<RwCamera*>(camera)),"scene Player activity camera end");begun=false;
    for(auto& slot:players.State().slots)(void)AnimationEventQueue::Clear(reinterpret_cast<std::uint32_t>(&slot)+0xaa8u,queue);
    check(memory.OutstandingAllocations()==allocated && overlay.OwnedTexts()==texts && !overlay.TemporaryStrings(),"active and inactive scene Player activity allocations retire");
    std::cout<<"Native Scene Player animations: actual4 queues active="<<expected.size()<<" real original images, active filtering/order/context, model/activity failure recovery, manager/phase reentry guards and zero allocation delta pass\n";
}
}
