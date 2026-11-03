#pragma once
#include "pal4/native_combat_renderables.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_combat_console.h"
#include "pal4/native_combat_console_visual_events.h"
#include "pal4/native_ui_text_image_overlay.h"
#include "pal4/native_ui_text_image_activities.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/camera_data.h"
#include "pal4/ui_text_image_animation.h"
#include <rwcore.h>
#include <d3d9.h>
#include <array>
#include <bit>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatRenderables(NativeCombatPlayerManager& players,NativeCombatMonsterManager& monsters,
    NativeCombatConsole& console,NativeCombatConsoleVisualEvents& visuals,NativeUiTextImageOverlay& overlay,
    NativeCeguiUiSession& ui,CameraDataState& camera){
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    std::cout<<"Native render probe baseline: Players="<<players.Count()<<" Monsters="<<monsters.Count()<<" visual_allocations="<<visuals.PendingAllocations()<<" console_bound="<<console.AnimationActivitiesBound()<<"\n";
    // Creation legitimately emits stat-change number activities. Let their
    // actual queue clocks finish before introducing this probe's owned images.
    unsigned initial_frames{};
    for(;initial_frames<600u && visuals.PendingAllocations();++initial_frames){
        for(std::size_t i=0;i<players.Count();++i)players.Actor(players.ActorAt(i)).UpdateAnimations(1.f/60.f);
        for(std::size_t i=0;i<monsters.Count();++i)monsters.ModelOwner(monsters.ActorAt(i)).Actor().UpdateAnimations(1.f/60.f);
    }
    check(players.Count() && monsters.Count() && !visuals.PendingAllocations() && !console.AnimationActivitiesBound(),"render probe actual idle actor/console queues");
    for(std::size_t i=0;i<players.Count();++i)check(!players.Actor(players.ActorAt(i)).AnimationCount(),"empty actual Player queue before render probe");
    for(std::size_t i=0;i<monsters.Count();++i)check(!monsters.ModelOwner(monsters.ActorAt(i)).Actor().AnimationCount(),"empty actual Monster queue before render probe");
    const auto texts=overlay.OwnedTexts();const bool queued=ui.OverlayQueueEnabled();
    NativeBaseObjectMemory memory;
    NativeUiTextImageActivities activity(overlay,[&](std::uint32_t a){(void)memory.Free(a);});
    auto* sdk_camera=reinterpret_cast<RwCamera*>(camera.At(0u));bool begun{};
    struct Restore{NativeCombatPlayerManager& p;NativeCombatMonsterManager& m;NativeCombatConsole& c;
        NativeCeguiUiSession& ui;RwCamera* camera;bool& begun;bool queued;
        ~Restore(){try{for(std::size_t i=0;i<p.Count();++i)p.Actor(p.ActorAt(i)).ClearAnimations();
            for(std::size_t i=0;i<m.Count();++i)m.ModelOwner(m.ActorAt(i)).Actor().ClearAnimations();
            c.Reset();c.BindAnimationActivity(nullptr);ui.ClearQueuedOverlays();ui.SetOverlayQueueEnabled(queued);if(begun)RwCameraEndUpdate(camera);}catch(...){}}
    } restore{players,monsters,console,ui,sdk_camera,begun,queued};
    check(sdk_camera!=nullptr,"render probe actual CombatCamera");
    RwRGBA black{0,0,0,255};check(RwCameraClear(sdk_camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(sdk_camera),"render probe begins actual camera");begun=true;
    ui.ClearQueuedOverlays();ui.SetOverlayQueueEnabled(true);
    const auto add=[&](std::uint32_t actor){const std::array<std::uint32_t,2> event{0u,actor};visuals.Dispatch(0x540020u,reinterpret_cast<std::uint32_t>(event.data()));};
    for(std::size_t i=0;i<players.Count();++i)add(players.ActorAt(i));
    for(std::size_t i=0;i<monsters.Count();++i)add(monsters.ActorAt(i));
    check(visuals.PendingAllocations()==players.Count()+monsters.Count(),"original miss constructors enqueue one owned image per real first-roster actor");
    const auto before=ui.QuadCalls();DispatchNativeCombatRenderables(camera.At(0u),players,monsters);
    const auto actor_quads=ui.QuadCalls()-before;
    check(actor_quads>=players.Count()+monsters.Count() && !players.ActiveIterators() && !monsters.ActiveIterators(),"529B70 dispatches all original actor image queues and releases cursors");
    console.BindAnimationActivity(&activity);
    const auto element=memory.Allocate(72u);(void)UiTextImageAnimation::ConstructMotion(element,0u,overlay);
    (void)UiTextImageOverlay::SetPosition(element,std::bit_cast<std::uint32_t>(400.f),std::bit_cast<std::uint32_t>(300.f),overlay);
    UiTextImageOverlay::SetImage(element,"zi0","luokong",0x3f800000u,overlay);
    console.AddAnimation(element,true,false);console.Render(camera.At(0u));
    check(ui.QuadCalls()>before+actor_quads,"original CombatConsole4C3210 dispatches its populated owned queue");
    check(ui.RenderQueuedOverlays(),"actual CEGUI DLL renderer draws queued actor/console images");
    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());IDirect3DSurface9* target{};IDirect3DSurface9* staging{};
    check(device && SUCCEEDED(device->GetRenderTarget(0,&target)),"render probe GPU target");
    struct Surface{IDirect3DSurface9*& p;~Surface(){if(p)p->Release();}} target_owner{target},staging_owner{staging};
    D3DSURFACE_DESC desc{};check(SUCCEEDED(target->GetDesc(&desc)) && (desc.Format==D3DFMT_A8R8G8B8 || desc.Format==D3DFMT_X8R8G8B8),"render probe target format");
    check(SUCCEEDED(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&staging,nullptr)) &&
          SUCCEEDED(device->GetRenderTargetData(target,staging)),"render probe GPU readback");
    D3DLOCKED_RECT lock{};check(SUCCEEDED(staging->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"render probe GPU lock");
    unsigned colored{};for(unsigned y=0;y<desc.Height;++y)for(unsigned x=0;x<desc.Width;++x){
        std::uint32_t pixel;std::memcpy(&pixel,static_cast<const char*>(lock.pBits)+y*lock.Pitch+4u*x,4u);colored+=(pixel&0xffffffu)!=0u;
    }
    (void)staging->UnlockRect();check(colored>20u,"actual original actor/console images produce GPU pixels before SDK EndUpdate logo");
    check(RwCameraEndUpdate(sdk_camera)!=nullptr,"render probe ends actual camera");begun=false;
    for(std::size_t i=0;i<players.Count();++i)players.Actor(players.ActorAt(i)).ClearAnimations();
    for(std::size_t i=0;i<monsters.Count();++i)monsters.ModelOwner(monsters.ActorAt(i)).Actor().ClearAnimations();
    console.Reset();console.BindAnimationActivity(nullptr);
    check(!visuals.PendingAllocations() && !memory.OutstandingAllocations() && overlay.OwnedTexts()==texts && !overlay.TemporaryStrings(),"rendered original activities retire through actual queue destructors");
    std::cout<<"Native combat renderables: Players="<<players.Count()<<" Monsters="<<monsters.Count()<<" actor_quads="<<actor_quads<<" Console=1 GPU_pixels="<<colored<<" first_cursors=0 activity_allocations=0 pass\n";
}
}
