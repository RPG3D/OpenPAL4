#pragma once
#include "pal4/native_game_world_scene_frame.h"
#include "pal4/native_combat_scene_frame.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_pal_object_manager_owner.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_ui_world_camera.h"
#include "pal4/native_cegui_ui_session.h"
#include <rwcore.h>
#include <rpworld.h>
#include <d3d9.h>
#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
namespace pal4::test {
inline std::vector<GameWorldFrameOperation> CompleteSceneOperations(bool early,bool rendered){
    using Op=GameWorldFrameOperation;std::vector<Op> operations;
    if(early)operations={Op::GetPlayerManager,Op::ProcessPlayerUpdates,Op::GetNpcManager,Op::ProcessNpcAlternative};
    operations.insert(operations.end(),{Op::BeginFrame,Op::RenderWorld});
    if(rendered)operations.insert(operations.end(),{
        Op::GetGameObjectManager,Op::RenderGameObjects,Op::GetPlayerManager,Op::UpdateAndRenderPlayers,
        Op::GetNpcManager,Op::ProcessNpcObjects,Op::GetMonsterManager,Op::ProcessMonsterObjects,
        Op::GetPetManager,Op::ProcessPetObjects,Op::GetPalObjectManager,Op::RenderPalObjects,
        Op::GetArenaSkillManager,Op::RenderArenaSkills,Op::ProcessScenePass});
    operations.push_back(Op::EndFrame);
    if(rendered)operations.insert(operations.end(),{Op::GetPlayerManager,Op::ProcessPlayerUpdates,Op::GetNpcManager,Op::ProcessNpcAlternative,
        Op::GetEffectManager,Op::ProcessEffects,Op::GetPlayerManager,Op::ProcessPlayerRelatedData});
    return operations;
}
inline void VerifyNativeCompleteSceneFrame(NativeGameWorldSceneFrame& frame,NativePalObjectManagerOwner& objects,
    NativeGameArenaManagerOwner& arena,NativeWorldAsset& world,NativeCombatSystemImage& image,
    ICombatSystemFrameRenderer& renderer,NativeCeguiUiSession& ui,RwCamera* original,
    const std::function<void()>& restore_states){
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    NativeUiWorldCamera camera;check(camera.Open(world.World(),320u,240u) && original,"complete scene actual offscreen camera");
    check(RwFrameTransform(RwCameraGetFrame(camera.Camera()),RwFrameGetLTM(RwCameraGetFrame(original)),rwCOMBINEREPLACE),"complete scene camera pose");
    RwCameraSetViewWindow(camera.Camera(),RwCameraGetViewWindow(original));RwCameraSetNearClipPlane(camera.Camera(),RwCameraGetNearClipPlane(original));RwCameraSetFarClipPlane(camera.Camera(),RwCameraGetFarClipPlane(original));
    const auto context=reinterpret_cast<std::uint32_t>(camera.Camera());
    const auto settings=word(arena.Address()+504u),ready=word(arena.Address()+48u),state=image.State();const auto mode=image.Mode();
    struct Restore{NativeGameArenaManagerOwner& arena;NativeCombatSystemImage& image;std::uint32_t settings,ready,state;std::uint8_t mode;
        ~Restore(){*reinterpret_cast<std::uint32_t*>(arena.Address()+504u)=settings;*reinterpret_cast<std::uint32_t*>(arena.Address()+48u)=ready;image.State()=state;image.Mode()=mode;}} restore{arena,image,settings,ready,state,mode};
    const auto early=[&]{const auto s=word(arena.Address()+504u);return s && *reinterpret_cast<const std::uint8_t*>(s+1u) && word(s+8u)==3u;};
    NativeCombatSceneFrame scene(image,arena,frame);
    const auto capture=[&](unsigned route){
        RwRGBA black{0,0,0,255};check(RwCameraClear(camera.Camera(),&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera.Camera()),"complete scene camera begin");
        struct End{RwCamera* camera;~End(){RwCameraEndUpdate(camera);}} end{camera.Camera()};restore_states();
        bool failed{};
        if(route==1u || route==3u){try{frame.ProcessFrame(arena.Address(),context);}catch(const std::runtime_error& e){failed=std::string_view(e.what())=="intentional complete scene object failure";if(!failed)throw;}}
        if(route==2u){auto* sdk=camera.Camera();CombatSceneFrame::Render(reinterpret_cast<std::uint32_t>(&sdk),scene,renderer);}
        check(failed==(route==3u),"complete scene callback failure expectation");
        auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());IDirect3DSurface9* target{};IDirect3DSurface9* staging{};
        struct Surface{IDirect3DSurface9*& p;~Surface(){if(p)p->Release();}} target_owner{target},staging_owner{staging};
        check(device && SUCCEEDED(device->GetRenderTarget(0,&target)),"complete scene GPU target");D3DSURFACE_DESC desc{};check(SUCCEEDED(target->GetDesc(&desc)),"complete scene GPU description");
        check(SUCCEEDED(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&staging,nullptr)) && SUCCEEDED(device->GetRenderTargetData(target,staging)),"complete scene GPU readback");
        D3DLOCKED_RECT lock{};check(SUCCEEDED(staging->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"complete scene GPU lock");std::vector<std::uint32_t> pixels(desc.Width*desc.Height);
        for(unsigned y=0;y<desc.Height;++y)std::memcpy(pixels.data()+y*desc.Width,static_cast<const char*>(lock.pBits)+y*lock.Pitch,desc.Width*4u);staging->UnlockRect();return pixels;
    };
    const auto black=capture(0u),rendered=capture(1u);const auto normal=frame.LastResult();
    {
        std::ofstream out("H:/PAL4/objs/M12-complete-scene-20261021.ppm",std::ios::binary);out<<"P6\n320 240\n255\n";
        for(auto pixel:rendered){const char rgb[]{static_cast<char>(pixel>>16u),static_cast<char>(pixel>>8u),static_cast<char>(pixel)};out.write(rgb,3);}
        check(static_cast<bool>(out),"complete scene GPU evidence saved");
    }
    unsigned pixels{};for(std::size_t i=0;i<black.size();++i)pixels+=(black[i]&0xffffffu)!=(rendered[i]&0xffffffu);
    check(pixels>20u && normal.world_rendered && normal.scene_passes==1u && normal.operations==CompleteSceneOperations(early(),true),"complete native519C50 executes exact original phases and actual GPU drawing");
    check(normal.players.active_players==normal.players.character_updates && normal.player_queues==normal.players.active_players && normal.player_camera_passes>=normal.players.active_players,"complete scene active Player model/collision/camera/queue phases agree");
    check(!objects.Size() && capture(1u)==rendered,"complete scene repeats stable GPU with actual owned empty PalObject registry");
    const auto drained=[&]{
        for(auto offset:{268u,284u,300u})check(word(arena.Address()+offset+4u)==word(arena.Address()+offset+8u),"complete scene actual geometry queue is drained");
        DWORD enabled=TRUE;auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
        check(device && SUCCEEDED(device->GetRenderState(D3DRS_FOGENABLE,&enabled)) && !enabled,"complete scene actual D3D9 fog state is cleared");
    };
    drained();
    std::array<std::uint32_t,6> fog{0x101u,0x00112233u,3u,0u,0x447a0000u,0u};
    const auto allocations=objects.OutstandingAllocations();
    class Failure final:public INativePalObject{
        NativeGameWorldSceneFrame& frame;std::uint32_t world;
    public:
        unsigned calls{},reentry{};Failure(NativeGameWorldSceneFrame& frame,std::uint32_t world):frame(frame),world(world){}
        std::uint32_t Address()const noexcept override{return reinterpret_cast<std::uint32_t>(this);}
        void Render(std::uint32_t camera)override{++calls;try{frame.ProcessFrame(world,camera);}catch(const std::logic_error&){++reentry;}throw std::runtime_error("intentional complete scene object failure");}
        void Update(float)override{throw std::logic_error("unexpected complete scene update");}
    };
    {auto owner=std::make_unique<Failure>(frame,arena.Address());auto* failure=owner.get();check(objects.Register("complete-scene-failure",std::move(owner)),"register required failing native PalObject owner");
        struct Remove{NativePalObjectManagerOwner& objects;~Remove(){objects.Remove("complete-scene-failure");}} remove{objects};
        word(arena.Address()+504u)=reinterpret_cast<std::uint32_t>(fog.data());(void)capture(3u);drained();word(arena.Address()+504u)=settings;check(failure->calls==1u && failure->reentry==1u && frame.LastResult().operations.back()==GameWorldFrameOperation::RenderPalObjects,"complete scene failure reaches actual PalObject phase and rejects frame reentry");}
    check(objects.OutstandingAllocations()==allocations && capture(1u)==rendered,"complete scene restores fog and drains geometry after partial-frame failure then retries");
    word(arena.Address()+504u)=reinterpret_cast<std::uint32_t>(fog.data());(void)capture(1u);
    check(frame.LastResult().operations==CompleteSceneOperations(true,true) && frame.LastResult().player_camera_passes==normal.players.active_players*2u,"complete scene mode3 adds both actual pre-render camera passes");word(arena.Address()+504u)=settings;
    word(arena.Address()+48u)=0u;const auto gated=capture(1u);
    check(gated==black && !frame.LastResult().world_rendered && !frame.LastResult().scene_passes && frame.LastResult().operations==CompleteSceneOperations(early(),false),"world not-ready gate clears render state and skips all later managers");word(arena.Address()+48u)=ready;
    const auto ui_before=ui.RenderCalls();image.State()=0u;image.Mode()=0u;(void)capture(2u);
    check(frame.LastResult().world_rendered && frame.LastResult().operations==CompleteSceneOperations(early(),true) && ui.RenderCalls()>ui_before,"actual58AF70 state0 fallback completes native519C50 before real576220 UI renderer");
    std::cout<<"Native complete scene frame: phases="<<normal.operations.size()<<" GPU_pixels="<<pixels<<" GOB="<<normal.gob_objects<<" Players="<<normal.players.active_players<<" NPC="<<normal.npcs.active_objects<<" Monsters="<<normal.monsters.active_objects<<" Pet="<<normal.pet.active_objects<<" PalObjects="<<normal.pal_objects<<" Skills="<<normal.skill_actors<<" exact order, fog3/ready gates, partial failure/drain/retry, real Combat state0 fallback pass\n";
}
}
