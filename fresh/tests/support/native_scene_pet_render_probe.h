#pragma once
#include "pal4/native_pet_scene_session.h"
#include "pal4/native_pet_manager_owner.h"
#include "pal4/native_ui_world_camera.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_game_world_scene_pass.h"
#include "pal4/script_pet_visibility.h"
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
inline void VerifyNativeScenePetRender(NativePetSceneSession& scene, NativePetManagerOwner& pet,
    NativeGameArenaManagerOwner& arena, NativeWorldAsset& world, const std::function<void()>& restore_states) {
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto word=[&](unsigned offset)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(pet.Address()+offset);};
    RwSphere sphere{};bool found{};
    struct Bounds {RwSphere& sphere;bool& found;} bounds{sphere,found};
    RpClumpForAllAtomics(reinterpret_cast<RpClump*>(word(0x14u)),[](RpAtomic* a,void* raw)->RpAtomic*{
        auto& b=*static_cast<Bounds*>(raw);if(!b.found){b.sphere=*RpAtomicGetWorldBoundingSphere(a);b.found=true;}return a;
    },&bounds);check(found,"original339 pet has actual atomic bounds");
    NativeUiWorldCamera camera;check(camera.Open(world.World(),320u,240u),"pet offscreen camera");
    const auto radius=std::max(1.f,sphere.radius);
    RwV3d position{sphere.center.x,sphere.center.y,sphere.center.z-4.f*radius};
    RwMatrixSetIdentity(RwFrameGetMatrix(RwCameraGetFrame(camera.Camera())));
    check(RwFrameTranslate(RwCameraGetFrame(camera.Camera()),&position,rwCOMBINEREPLACE),"pet camera pose");
    RwCameraSetNearClipPlane(camera.Camera(),radius*.01f);RwCameraSetFarClipPlane(camera.Camera(),radius*30.f);
    const auto context=reinterpret_cast<std::uint32_t>(camera.Camera());
    {
        const std::vector<std::byte> saved(pet.Image().begin(),pet.Image().end());
        auto lease=pet.BorrowFrame();
        for(unsigned which=0;which<8u;++which){bool rejected{};try{
            switch(which){
            case 0:(void)pet.Reset();break;
            case 1:pet.InitializeStoredData();break;
            case 2:pet.InitializeData(pet.Source());break;
            case 3:scene.Clear();break;
            case 4:(void)scene.Remove();break;
            case 5:scene.LoadActor();break;
            case 6:(void)scene.RenderFrame(context);break;
            case 7:(void)ScriptPetVisibility::SetEnabled(0u,scene.VisibilityRuntime());break;
            }
        }catch(const std::logic_error&){rejected=true;}
        check(rejected && std::equal(saved.begin(),saved.end(),pet.Image().begin()),"pet frame lease rejects mutation before changing actual image");}
    }
    NativeBaseObjectSceneFrameResult result{};
    const auto capture=[&](bool draw,bool expect_failure=false){
        RwRGBA black{0,0,0,255};check(RwCameraClear(camera.Camera(),&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera.Camera()),"pet camera begin");
        struct End{RwCamera* camera;~End(){RwCameraEndUpdate(camera);}} end{camera.Camera()};restore_states();
        bool failed{};if(draw){try{result=scene.RenderFrame(context);}catch(const std::invalid_argument&){failed=true;}}
        NativeGameWorldScenePassRuntime pass([&](auto,auto){arena.RenderSequence(context);});(void)GameWorldScenePass::Process(arena.Address(),context,pass);
        check(failed==expect_failure,"pet actual callback failure expectation");
        auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());IDirect3DSurface9* target{};IDirect3DSurface9* staging{};
        struct Surface{IDirect3DSurface9*& p;~Surface(){if(p)p->Release();}} target_owner{target},staging_owner{staging};
        check(device && SUCCEEDED(device->GetRenderTarget(0,&target)),"pet GPU target");D3DSURFACE_DESC desc{};check(SUCCEEDED(target->GetDesc(&desc)),"pet GPU description");
        check(SUCCEEDED(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&staging,nullptr)) && SUCCEEDED(device->GetRenderTargetData(target,staging)),"pet GPU readback");
        D3DLOCKED_RECT lock{};check(SUCCEEDED(staging->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"pet GPU lock");std::vector<std::uint32_t> pixels(desc.Width*desc.Height);
        for(unsigned y=0;y<desc.Height;++y)std::memcpy(pixels.data()+y*desc.Width,static_cast<const char*>(lock.pBits)+y*lock.Pitch,desc.Width*4u);staging->UnlockRect();return pixels;
    };
    const auto baseline=capture(false);const auto rendered=capture(true);
    unsigned changes{};for(std::size_t i=0;i<baseline.size();++i)changes+=(baseline[i]&0xffffffu)!=(rendered[i]&0xffffffu);
    std::cout<<"Pet GPU operands: changed="<<changes<<" active="<<result.active_objects<<" render="<<result.render_successes<<" visible="<<result.visible_objects<<" animation="<<result.animation_calls<<" character="<<result.character_updates<<"\n";
    check(changes>20u && result.active_objects==1u && result.render_successes==1u && result.visible_objects==1u && result.animation_calls==1u && result.character_updates==1u,"original339 pet draws actual pixels and executes animation/collision phases");
    for(const auto offset:{0x654u,0x1f0u}){
        auto& value=word(offset);const auto saved=value;value=0u;
        struct Restore{std::uint32_t& v;std::uint32_t saved;~Restore(){v=saved;}} restore{value,saved};
        check(capture(true)==baseline && !result.active_objects && !result.character_updates,"pet enabled and activity gates suppress actual drawing and callbacks");
    }
    {
        auto& token=word(0x564u+104u);const auto saved=token;
        struct Restore{std::uint32_t& v;std::uint32_t saved;~Restore(){v=saved;}} restore{token,saved};
        token^=4u;(void)capture(true,true);
    }
    check(capture(true)==rendered && !pet.FrameActive(),"pet retries actual rendering after callback failure without retaining frame lease");
    {
        const std::vector<std::byte> saved(pet.Image().begin(),pet.Image().end());
        struct RestoreImage{NativePetManagerOwner& pet;const std::vector<std::byte>& saved;
            ~RestoreImage(){std::copy(saved.begin(),saved.end(),pet.Image().begin());}} restore{pet,saved};
        const auto floating=[](float f){std::uint32_t w;std::memcpy(&w,&f,sizeof(w));return w;};
        float node[5]{0.f,0.f,4.f,5.f,6.f};
        word(0x564u+40u)=1u;word(0x564u+92u)=reinterpret_cast<std::uint32_t>(node);
        word(0x564u+96u)=0x444320u;word(0x564u+100u)=0x444330u;
        word(0x564u+12u)=floating(1.f);word(0x564u+16u)=floating(2.f);word(0x564u+20u)=floating(3.f);
        (void)capture(true);
        check(word(0x608u)==pet.Address()+0x564u+44u && word(0x60cu)==floating(1.f) &&
            word(0x610u)==floating(2.3f) && word(0x614u)==floating(3.f) &&
            word(0x630u)==floating(4.f) && word(0x634u)==floating(5.3f) &&
            word(0x638u)==floating(6.f) && word(0x624u)==0xdcf01414u && word(0x648u)==0xdcf01414u,
            "pet actual CollisionActor path/move callbacks populate complete callback record");
    }
    std::cout<<"Native pet render: original339 GPU, animation/collision, enabled/activity gates, eight mutation guards, actual path/move and callback failure/retry pass\n";
}
}
