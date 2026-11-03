#pragma once
#include "pal4/native_arena_skill_manager_owner.h"
#include "pal4/native_ui_world_camera.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_game_world_scene_pass.h"
#include <rwcore.h>
#include <rpworld.h>
#include <d3d9.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <vector>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeArenaSkillFrame(NativeArenaSkillManagerOwner& owner, IEffectHandleContentRuntime& content,
    NativeGameArenaManagerOwner& arena,NativeWorldAsset& world,std::uint32_t record,
    const std::function<void()>& restore_states){
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    auto& memory=owner.Storage();const auto manager=owner.Address();const auto actor=memory.ReadDword(record+32u);
    check(actor && owner.SignpostActorCount(),"arena skill probe uses a creating-owner M001 actor");
    std::array<std::uint32_t,11> empty_record{};
    std::array<std::uint32_t,3> records{record,reinterpret_cast<std::uint32_t>(empty_record.data()),record};
    std::array<std::uint32_t,3> previous{};for(unsigned i=0;i<3u;++i)previous[i]=memory.ReadDword(manager+4u+i*4u);
    const auto mode=memory.ReadDword(actor+24u),visible=memory.ReadDword(actor+28u);
    struct Restore{NativeBaseObjectMemory& memory;std::uint32_t manager,actor,mode,visible;std::array<std::uint32_t,3> previous;
        ~Restore(){for(unsigned i=0;i<3u;++i)memory.WriteDword(manager+4u+i*4u,previous[i]);memory.WriteDword(actor+24u,mode);memory.WriteDword(actor+28u,visible);}} restore{memory,manager,actor,mode,visible,previous};
    const auto array=reinterpret_cast<std::uint32_t>(records.data());memory.WriteDword(manager+4u,array);memory.WriteDword(manager+8u,array+sizeof(records));memory.WriteDword(manager+12u,array+sizeof(records));
    RwSphere sphere{};bool found{};struct Bounds{RwSphere& sphere;bool& found;} bounds{sphere,found};
    RpClumpForAllAtomics(reinterpret_cast<RpClump*>(memory.ReadDword(actor+20u)),[](RpAtomic* a,void* raw)->RpAtomic*{auto& b=*static_cast<Bounds*>(raw);if(!b.found){b.sphere=*RpAtomicGetWorldBoundingSphere(a);b.found=true;}return a;},&bounds);
    check(found,"M001 first-array actor has actual atomic bounds");
    NativeUiWorldCamera camera;check(camera.Open(world.World(),320u,240u),"arena skill offscreen camera");
    const auto radius=std::max(1.f,sphere.radius);RwV3d position{sphere.center.x,sphere.center.y,sphere.center.z-4.f*radius};
    RwMatrixSetIdentity(RwFrameGetMatrix(RwCameraGetFrame(camera.Camera())));check(RwFrameTranslate(RwCameraGetFrame(camera.Camera()),&position,rwCOMBINEREPLACE),"arena skill camera pose");
    RwCameraSetNearClipPlane(camera.Camera(),radius*.01f);RwCameraSetFarClipPlane(camera.Camera(),radius*30.f);
    const auto context=reinterpret_cast<std::uint32_t>(camera.Camera());std::uint32_t count{};
    const auto capture=[&](bool draw,bool expect_failure=false){
        RwRGBA black{0,0,0,255};check(RwCameraClear(camera.Camera(),&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera.Camera()),"arena skill camera begin");
        struct End{RwCamera* camera;~End(){RwCameraEndUpdate(camera);}} end{camera.Camera()};restore_states();
        bool failed{};if(draw){try{count=owner.RenderFrame(context);}catch(const std::logic_error&){failed=true;}}
        NativeGameWorldScenePassRuntime pass([&](auto,auto){arena.RenderSequence(context);});(void)GameWorldScenePass::Process(arena.Address(),context,pass);
        check(failed==expect_failure,"arena skill actual callback failure expectation");
        auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());IDirect3DSurface9* target{};IDirect3DSurface9* staging{};
        struct Surface{IDirect3DSurface9*& p;~Surface(){if(p)p->Release();}} target_owner{target},staging_owner{staging};
        check(device && SUCCEEDED(device->GetRenderTarget(0,&target)),"arena skill GPU target");D3DSURFACE_DESC desc{};check(SUCCEEDED(target->GetDesc(&desc)),"arena skill GPU description");
        check(SUCCEEDED(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&staging,nullptr)) && SUCCEEDED(device->GetRenderTargetData(target,staging)),"arena skill GPU readback");
        D3DLOCKED_RECT lock{};check(SUCCEEDED(staging->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"arena skill GPU lock");std::vector<std::uint32_t> pixels(desc.Width*desc.Height);
        for(unsigned y=0;y<desc.Height;++y)std::memcpy(pixels.data()+y*desc.Width,static_cast<const char*>(lock.pBits)+y*lock.Pitch,desc.Width*4u);staging->UnlockRect();return pixels;
    };
    const auto baseline=capture(false);
    check(mode==0u && capture(true)==baseline && count==2u,"first arena skill array visits duplicate real actor and skips null actor in original mode0");
    memory.WriteDword(actor+24u,1u);const auto rendered=capture(true);unsigned changes{};
    for(std::size_t i=0;i<baseline.size();++i)changes+=(baseline[i]&0xffffffu)!=(rendered[i]&0xffffffu);
    check(count==2u && changes>20u,"521080 real M001 clump/empty animation callbacks reach actual GPU");
    const auto original_clump=memory.ReadDword(actor+20u);
    {struct Clump{NativeBaseObjectMemory& memory;std::uint32_t actor,value;~Clump(){memory.WriteDword(actor+20u,value);}} clump{memory,actor,original_clump};memory.WriteDword(actor+20u,original_clump^4u);(void)capture(true,true);}
    check(capture(true)==rendered && count==2u,"arena skill owner/actor guards recover after actual model identity failure");
    memory.WriteDword(manager+8u,array);check(capture(true)==baseline && !count,"empty first arena skill array submits no actor");
    {
        NativeArenaSkillManagerOwner fixture(content);auto& storage=fixture.Storage();
        const auto image=fixture.Address(),entry=storage.Allocate(44u),list=storage.Allocate(4u);
        for(unsigned offset=0;offset<44u;offset+=4u)storage.WriteDword(entry+offset,0u);
        storage.WriteDword(list,entry);storage.WriteDword(image+4u,list);storage.WriteDword(image+8u,list+4u);storage.WriteDword(image+12u,list+4u);
        storage.WriteDword(entry+32u,0x1234u);
        bool missing{};try{(void)fixture.RenderFrame(context);}catch(const std::logic_error&){missing=true;}
        check(missing,"nonempty arena skill frame cannot fabricate missing creator services");
        storage.WriteDword(entry+32u,0u);check(!fixture.RenderFrame(context),"null actor record needs no creator service");storage.WriteDword(entry+32u,0x1234u);
        class Failure final:public INativeArenaSkillActors{
        public:
            NativeArenaSkillManagerOwner& owner;std::uint32_t camera,calls{};
            Failure(NativeArenaSkillManagerOwner& owner,std::uint32_t camera):owner(owner),camera(camera){}
            std::uint32_t ReadWord(std::uint32_t)override{throw std::logic_error("unused failure fixture read");}
            void WriteWord(std::uint32_t,std::uint32_t)override{throw std::logic_error("unused failure fixture write");}
            void DestroySkillActor(std::uint32_t)override{throw std::logic_error("failure fixture never owns a successful actor");}
            std::uint32_t Call(ArenaActorCall op,std::initializer_list<std::uint32_t> args)override{
                if(op!=ArenaActorCall::RenderClump || args.size()!=2u || *args.begin()!=0x1234u || *(args.begin()+1u)!=camera)
                    throw std::logic_error("arena skill failure fixture unexpected dispatch");
                ++calls;
                for(unsigned which=0;which<6u;++which){bool rejected{};try{
                    if(which==0u)owner.Clear();
                    if(which==1u)owner.Destroy();
                    if(which==2u)owner.BindActors(nullptr);
                    if(which==3u)(void)owner.RenderFrame(camera);
                    if(which==4u)(void)owner.ShowSignpost(0u);
                    if(which==5u)(void)owner.CreateSignpostEffect({0.f,0.f,0.f});
                }catch(const std::logic_error&){rejected=true;}
                if(!rejected)throw std::runtime_error("arena skill mutation unexpectedly succeeded");}
                throw std::runtime_error("intentional arena skill frame failure");
            }
        } failure(fixture,context);
        storage.WriteDword(image+8u,list);fixture.BindActors(&failure);
        storage.WriteDword(list,entry);storage.WriteDword(image+8u,list+4u);storage.WriteDword(entry+32u,0x1234u);
        for(unsigned retry=0;retry<2u;++retry){bool failed{};try{(void)fixture.RenderFrame(context);}catch(const std::runtime_error& error){failed=std::string_view(error.what())=="intentional arena skill frame failure";}
            check(failed && failure.calls==retry+1u && storage.ReadDword(entry+32u)==0x1234u && storage.ReadDword(image+8u)==list+4u,"arena skill callback failure releases guard and preserves actual record storage");}
        storage.WriteDword(entry+32u,0u);fixture.Clear();fixture.BindActors(nullptr);fixture.Destroy();check(!storage.OutstandingAllocations(),"arena skill failure fixture retires record/array/owner storage");
    }
    std::cout<<"Native arena skill frame: actual M001 first-array traversal, duplicate/null entries, original mode0, controlled mode1 GPU_changed="<<changes<<", actual model failure/retry, empty gate, missing provider, six mutation guards and failure retry/cleanup pass\n";
}
}
