#pragma once
#include "pal4/native_monster_manager_owner.h"
#include "pal4/native_ui_world_camera.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_game_world_scene_pass.h"
#include <rwcore.h>
#include <rpworld.h>
#include <d3d9.h>
#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <vector>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeSceneMonsterRender(NativeMonsterManagerOwner& monsters,
    NativeGameArenaManagerOwner& arena,NativeWorldAsset& world,const std::function<void()>& restore_states){
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    struct Flags{std::uint32_t address,active,mode,visible;};std::vector<Flags> flags;
    monsters.VisitObjects([&](auto a){flags.push_back({a,word(a+0x1f0u),word(a+0x18u),word(a+0x1cu)});return true;});
    check(!flags.empty(),"original scene Monster GPU probe needs actual owners");
    struct Restore{std::vector<Flags>& flags;~Restore(){for(auto f:flags){*reinterpret_cast<std::uint32_t*>(f.address+0x1f0u)=f.active;*reinterpret_cast<std::uint32_t*>(f.address+0x18u)=f.mode;*reinterpret_cast<std::uint32_t*>(f.address+0x1cu)=f.visible;}}} restore{flags};
    for(auto f:flags)word(f.address+0x1f0u)=0u;
    const auto selected=flags.front().address;word(selected+0x1f0u)=1u;
    RwSphere sphere{};bool found{};struct Bounds{RwSphere& sphere;bool& found;} bounds{sphere,found};
    RpClumpForAllAtomics(reinterpret_cast<RpClump*>(word(selected+0x14u)),[](RpAtomic* a,void* raw)->RpAtomic*{auto& b=*static_cast<Bounds*>(raw);if(!b.found){b.sphere=*RpAtomicGetWorldBoundingSphere(a);b.found=true;}return a;},&bounds);
    check(found,"original Monster has actual SDK atomic bounds");
    NativeUiWorldCamera camera;check(camera.Open(world.World(),320u,240u),"Monster offscreen camera");
    const auto radius=std::max(1.f,sphere.radius);RwV3d position{sphere.center.x,sphere.center.y,sphere.center.z-4.f*radius};
    RwMatrixSetIdentity(RwFrameGetMatrix(RwCameraGetFrame(camera.Camera())));
    check(RwFrameTranslate(RwCameraGetFrame(camera.Camera()),&position,rwCOMBINEREPLACE),"Monster camera pose");
    RwCameraSetNearClipPlane(camera.Camera(),radius*.01f);RwCameraSetFarClipPlane(camera.Camera(),radius*30.f);
    const auto context=reinterpret_cast<std::uint32_t>(camera.Camera());NativeBaseObjectSceneFrameResult result{};
    const auto capture=[&](bool draw,bool expect_failure=false){
        RwRGBA black{0,0,0,255};check(RwCameraClear(camera.Camera(),&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera.Camera()),"monster camera begin");
        struct End{RwCamera* camera;~End(){RwCameraEndUpdate(camera);}} end{camera.Camera()};restore_states();
        bool failed{};if(draw){try{result=monsters.RenderFrame(context);}catch(const std::invalid_argument&){failed=true;}}
        NativeGameWorldScenePassRuntime pass([&](auto,auto){arena.RenderSequence(context);});(void)GameWorldScenePass::Process(arena.Address(),context,pass);
        check(failed==expect_failure,"monster actual callback failure expectation");
        auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());IDirect3DSurface9* target{};IDirect3DSurface9* staging{};
        struct Surface{IDirect3DSurface9*& p;~Surface(){if(p)p->Release();}} target_owner{target},staging_owner{staging};
        check(device && SUCCEEDED(device->GetRenderTarget(0,&target)),"monster GPU target");D3DSURFACE_DESC desc{};check(SUCCEEDED(target->GetDesc(&desc)),"monster GPU description");
        check(SUCCEEDED(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&staging,nullptr)) && SUCCEEDED(device->GetRenderTargetData(target,staging)),"monster GPU readback");
        D3DLOCKED_RECT lock{};check(SUCCEEDED(staging->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"monster GPU lock");std::vector<std::uint32_t> pixels(desc.Width*desc.Height);
        for(unsigned y=0;y<desc.Height;++y)std::memcpy(pixels.data()+y*desc.Width,static_cast<const char*>(lock.pBits)+y*lock.Pitch,desc.Width*4u);staging->UnlockRect();return pixels;
    };
    const auto baseline=capture(false);
    const auto original_mode=word(selected+0x18u);
    if(!original_mode){check(capture(true)==baseline && result.active_objects==1u && result.render_successes==1u && result.character_updates==1u,"Monster mode0 retains original non-submitting body/callback behavior");}
    word(selected+0x18u)=1u;
    const auto rendered=capture(true);unsigned changes{};
    for(std::size_t i=0;i<baseline.size();++i)changes+=(baseline[i]&0xffffffu)!=(rendered[i]&0xffffffu);
    std::cout<<"Monster GPU operands: objects="<<flags.size()<<" original_mode="<<original_mode<<" changed="<<changes<<" active="<<result.active_objects<<" render="<<result.render_successes<<" visible="<<result.visible_objects<<" animation="<<result.animation_calls<<" character="<<result.character_updates<<"\n";
    check(changes>20u && result.active_objects==1u && result.render_successes==1u && result.visible_objects==1u && result.animation_calls==1u && result.character_updates==1u,"original Monster actual model/animation/collision GPU phases");
    word(selected+0x1f0u)=0u;check(capture(true)==baseline && !result.active_objects && !result.character_updates,"Monster manager skips hidden actual object");word(selected+0x1f0u)=1u;
    {auto& token=word(selected+0x564u+104u);const auto saved=token;struct RestoreToken{std::uint32_t& v;std::uint32_t saved;~RestoreToken(){v=saved;}} restore{token,saved};token^=4u;(void)capture(true,true);}
    check(capture(true)==rendered,"Monster retries exact GPU output after real CollisionActor callback failure");
    std::cout<<"Native scene Monster render: actual ordered manager, original model GPU, hidden gate, callback failure/drain/retry and production mode restoration pass\n";
}
}
