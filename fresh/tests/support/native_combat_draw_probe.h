#pragma once
#include "pal4/native_combat_world_resources.h"
#include "pal4/native_combat_prerequisites.h"
#include "pal4/native_ui_world_camera.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_renderware_pipeline_lifetime.h"
#include "pal4/native_combat_world_frame_services.h"
#include "pal4/native_rw_geometry_processor.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/actor_animation_control.h"
#include "native_combat_monster_render_probe.h"
#include <d3d9.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace pal4::test {
inline void VerifyNativeCombatDrawing(NativeCombatWorldResources& world,
    NativePkFileManagerRuntime& packages,NativeCameraManagerOwner& cameras,const std::filesystem::path& root) {
    const auto check=[](bool ok,const char* message){if(!ok) throw std::runtime_error(message);};
    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    NativeMatFxScriptSession shader(packages,device);
    check(shader.Load(),"M01 original MatFX source compiles for drawing");
    const auto compiled_effect=shader.Effect();
    check(!world.RenderPipelineReady(),"M01 resources begin without render callbacks");
    bool rejected{};
    try{world.InstallRenderPipeline(0u);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && !world.RenderPipelineReady(),"M01 missing effect fails without binding sectors");
    world.InstallRenderPipeline(shader.Effect());
    check(world.RenderPipelineReady(),"M01 persistent resource owner installs original pipeline");
    const auto pipeline=*reinterpret_cast<const std::uint32_t*>(world.Address()+228u);
    check(pipeline!=0u,"M01 embedded WorldManager pipeline field mirrors native owner");
    rejected=false;
    try{world.InstallRenderPipeline(shader.Effect());}catch(const std::logic_error&){rejected=true;}
    check(rejected,"M01 duplicate pipeline installation rejected");
    rejected=false;
    try{world.Render();}catch(const std::logic_error&){rejected=true;}
    check(rejected,"M01 draw requires its active world camera");
    // This owner pins the compiled effect independently of its source loader.
    shader.Reset();
    NativeUiWorldCamera camera;
    check(camera.Open(world.Asset().World(),320u,240u),"M01 draw camera offscreen allocation");
    (void)SetupNativeCombatCamera(cameras);
    const auto& combat=cameras.CameraDataForObject(cameras.FindByName("CombatCamera"));
    auto* original=reinterpret_cast<RwCamera*>(combat.At(0u));
    check(original && RwFrameTransform(RwCameraGetFrame(camera.Camera()),
        RwFrameGetLTM(RwCameraGetFrame(original)),rwCOMBINEREPLACE),"M01 original CombatCamera pose");
    RwCameraSetViewWindow(camera.Camera(),RwCameraGetViewWindow(original));
    const auto capture=[&](NativeCombatWorldRenderer* alternate=nullptr,INativeCombatWorldFrameServices* actors=nullptr,std::uint32_t context=0x1234u) {
        RwRGBA clear{17u,31u,47u,255u};
        check(RwCameraClear(camera.Camera(),&clear,rwCAMERACLEARIMAGE|rwCAMERACLEARZ)!=nullptr,"M01 offscreen clear");
        check(RwCameraBeginUpdate(camera.Camera())!=nullptr,"M01 draw camera begin");
        IDirect3DSurface9* target{};
        struct End {RwCamera* camera;~End(){if(camera)RwCameraEndUpdate(camera);}} end{camera.Camera()};
        check(SUCCEEDED(device->GetRenderTarget(0u,&target)) && target,"M01 render target acquisition");
        struct Surface {IDirect3DSurface9* p;~Surface(){if(p)p->Release();}} output{target};
        // The enclosing frame normally supplies these states; isolate this world
        // from earlier screen-effect/UI probes in the combined integration test.
        RwD3D9SetRenderState(D3DRS_LIGHTING,FALSE);
        RwD3D9SetRenderState(D3DRS_FOGENABLE,FALSE);
        check(RwRenderStateSet(rwRENDERSTATEZTESTENABLE,reinterpret_cast<void*>(TRUE)) &&
              RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,reinterpret_cast<void*>(TRUE)) &&
              RwRenderStateSet(rwRENDERSTATECULLMODE,reinterpret_cast<void*>(rwCULLMODECULLBACK)),"M01 enclosing frame depth/cull states");
        if(alternate) alternate->Render();else if(actors)check(world.RenderFrame(context,*actors)==0x55u,"frame forwards component phase result");else world.Render();
        RwCameraEndUpdate(camera.Camera());end.camera=nullptr;
        D3DSURFACE_DESC desc{};check(SUCCEEDED(target->GetDesc(&desc)),"M01 render target description");
        check(desc.Format==D3DFMT_A8R8G8B8 || desc.Format==D3DFMT_X8R8G8B8,"M01 readback pixel format");
        IDirect3DSurface9* staging{};
        check(SUCCEEDED(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&staging,nullptr)),"M01 staging allocation");
        Surface readback{staging};
        check(SUCCEEDED(device->GetRenderTargetData(target,staging)),"M01 GPU pixel readback");
        D3DLOCKED_RECT lock{};check(SUCCEEDED(staging->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"M01 staging lock");
        std::vector<std::uint32_t> pixels(desc.Width*desc.Height);
        for(unsigned y=0;y<desc.Height;++y)
            std::memcpy(pixels.data()+y*desc.Width,static_cast<const char*>(lock.pBits)+y*lock.Pitch,desc.Width*4u);
        staging->UnlockRect();
        return pixels;
    };
    const auto first=capture();
    const auto stats=world.RenderStats();
    unsigned changed{};
    for(const auto pixel:first) if((pixel&0xffffffu)!=0x111f2fu) ++changed;
    std::cout<<"Native Combat draw M01: sectors="<<stats.sectors<<" draws="<<stats.draws
             <<" primitives="<<stats.primitives<<" effect-passes="<<stats.effect_passes
             <<" dual="<<stats.dual_meshes<<" changed-pixels="<<changed<<'\n';
    check(stats.sectors && stats.draws && stats.effect_passes && stats.dual_meshes && changed>100u,
          "M01 original world/shader produces populated GPU pixels");
    check(capture()==first,"M01 repeated original drawing has deterministic pixels");
    {
        auto dark=world.LightmapParameters();dark[0]=0.0f;
        NativeCombatWorldRenderer alternate(*world.Asset().World(),compiled_effect,dark);
        const auto dimmed=capture(&alternate);
        std::uint64_t original_light{},dark_light{};unsigned differences{};
        for(std::size_t i=0;i<first.size();++i) {
            for(const auto shift:{0u,8u,16u}) {
                original_light+=(first[i]>>shift)&255u;
                dark_light+=(dimmed[i]>>shift)&255u;
            }
            if(first[i]!=dimmed[i]) ++differences;
        }
        std::cout<<"Native Combat shader response: changed="<<differences<<" brightness="<<original_light<<"->"<<dark_light<<'\n';
        check(differences>100u && dark_light<original_light,"M01 original shader brightness changes actual GPU output");
    }
    check(capture()==first,"M01 primary pipeline restores after temporary renderer/resource retirement");
    RpWorldSector* first_sector{};
    RpWorldForAllWorldSectors(world.Asset().World(),[](RpWorldSector* sector,void* data)->RpWorldSector* {
        auto& first=*static_cast<RpWorldSector**>(data);if(!first)first=sector;return sector;
    },&first_sector);
    const auto usage=RpD3D9WorldSectorGetUsageFlags(first_sector);
    {
        struct Restore {
            RpWorldSector* sector;RpD3D9WorldSectorUsageFlag usage;
            ~Restore(){RpD3D9WorldSectorSetUsageFlags(sector,usage);NativeRenderWarePipelineLifetime lifetime;
                (void)RenderWarePipelineLifetime::BindAndReleaseResource(reinterpret_cast<std::uint32_t>(sector),
                    reinterpret_cast<std::uint32_t>(sector->pipeline),lifetime);}
        } restore{first_sector,usage};
        RpD3D9WorldSectorSetUsageFlags(first_sector,static_cast<RpD3D9WorldSectorUsageFlag>(usage|0x10000u));
        NativeRenderWarePipelineLifetime lifetime;
        (void)RenderWarePipelineLifetime::BindAndReleaseResource(reinterpret_cast<std::uint32_t>(first_sector),
            reinterpret_cast<std::uint32_t>(first_sector->pipeline),lifetime);
        rejected=false;
        try{(void)capture();}catch(const std::runtime_error& e){rejected=std::string(e.what()).find("tangent conversion ABI")!=std::string::npos;}
        check(rejected,"M01 vertex failure reaches world owner even if SDK preserves world return value");
    }
    check(capture()==first,"M01 draw recovers original pixels after unsupported vertex request");
    {
        NativeAnimatedActorAsset actor,scenery_actor;
        check(actor.Load(root,"B01") && actor.SelectAction("Z01"),"Combat frame loads real original B01");
        check(scenery_actor.Load(root,"B02") && scenery_actor.SelectAction("Z01"),"Combat frame loads real original B02");
        auto* clump=actor.Main();
        const auto queue=world.GeometryQueue(),scenery=world.Address()+196u;
        check(queue==world.Address()+212u,"WorldManager328 uses existing scene212 queue storage");
        for(auto* body:{clump,scenery_actor.Main()})RpClumpForAllAtomics(body,[](RpAtomic* a,void*){
            auto* g=RpAtomicGetGeometry(a);if(g)for(int i=0;i<RpGeometryGetNumMaterials(g);++i){
                auto* m=RpGeometryGetMaterial(g,i);auto color=*RpMaterialGetColor(m);color.alpha=128u;RpMaterialSetColor(m,&color);
            }return a;
        },nullptr);
        struct Restore {NativeCombatWorldResources& world;RpClump* clump;RpClump* second;~Restore(){world.ClearGeometryQueues();NativeRwGeometryProcessor::RestoreClump(clump);NativeRwGeometryProcessor::RestoreClump(second);}} restore{world,clump,scenery_actor.Main()};
        class Actors final : public INativeCombatWorldFrameServices {
        public:
            NativeCombatWorldResources& world;RpClump* clump;RpClump* second;unsigned queue{},scenery{},draws{},components{};bool fail{};
            Actors(NativeCombatWorldResources& w,RpClump* c,RpClump* b,unsigned q,unsigned s):world(w),clump(c),second(b),queue(q),scenery(s){}
            unsigned Address() const override{return reinterpret_cast<unsigned>(this);}
            static unsigned Word(unsigned a){return *reinterpret_cast<unsigned*>(a);}
            void ProcessActorUpdates(unsigned context) override{
                if(context!=0x1234u)throw std::runtime_error("Combat frame context mismatch");
                // Protocol fixture supplies actual SDK draws to both real world
                // queues. It does not claim the full529AC0 actor loop is bound.
                (void)NativeRwGeometryProcessor::ProcessClump(clump,queue);
                if(!RpClumpRender(clump))throw std::runtime_error("B01 queued render failed");
                if(auto error=NativeRwGeometryProcessor::TakeRenderError())std::rethrow_exception(error);
                if(Word(queue+8u)<=Word(queue+4u))throw std::runtime_error("B01 did not populate world transparent queue");
                (void)NativeRwGeometryProcessor::ProcessClump(second,scenery);
                if(!RpClumpRender(second))throw std::runtime_error("B02 scenery queue render failed");
                if(auto error=NativeRwGeometryProcessor::TakeRenderError())std::rethrow_exception(error);
                if(Word(scenery+8u)<=Word(scenery+4u))throw std::runtime_error("B01 did not populate world scenery queue");
                ++draws;
                bool rejected=false;try{world.Reset();}catch(const std::logic_error&){rejected=true;}
                if(!rejected)throw std::runtime_error("Combat frame allowed world retirement");
                if(fail)throw std::runtime_error("intentional Combat frame actor failure");
            }
            unsigned UpdateActorComponents(unsigned context) override{
                if(context!=0x1234u || Word(queue+8u)!=Word(queue+4u) || Word(scenery+8u)!=Word(scenery+4u))
                    throw std::runtime_error("Combat component phase preceded queue rendering/clear");
                ++components;return 0x55u;
            }
        } actors(world,clump,scenery_actor.Main(),queue,scenery);
        (void)capture(nullptr,&actors);
        check(actors.draws==1u && actors.components==1u && world.GeometryAllocations()==2u,
              "actual B01 fills both world-owned queues, renders, clears and retains capacity");
        const auto first_capacity=Actors::Word(queue+4u),second_capacity=Actors::Word(scenery+4u);
        (void)capture(nullptr,&actors);
        check(actors.draws==2u && actors.components==2u && Actors::Word(queue+4u)==first_capacity &&
              Actors::Word(scenery+4u)==second_capacity,"Combat frames reuse original queue capacity");
        actors.fail=true;rejected=false;
        try{(void)capture(nullptr,&actors);}catch(const std::runtime_error& e){rejected=std::string(e.what())=="intentional Combat frame actor failure";}
        check(rejected && actors.components==2u,"frame failure suppresses final phase and unwinds camera");
        world.ClearGeometryQueues();actors.fail=false;(void)capture(nullptr,&actors);
        check(actors.components==3u,"Combat frame owner recovers after actor phase failure");
        std::cout<<"Native Combat world queues: scene196/212, real B01, two owned capacities, ordered frame drain and failure recovery pass\n";
    }
    VerifyNativeCombatMonsterRendering(root,world,camera.Camera(),capture,first);
    BITMAPFILEHEADER header{};BITMAPINFOHEADER info{};
    header.bfType=0x4d42u;header.bfOffBits=sizeof header+sizeof info;
    header.bfSize=header.bfOffBits+static_cast<DWORD>(first.size()*4u);
    info.biSize=sizeof info;info.biWidth=320;info.biHeight=-240;info.biPlanes=1;info.biBitCount=32;info.biCompression=BI_RGB;
    std::ofstream image("H:/PAL4/objs/M12-native-draw-20261017-M01.bmp",std::ios::binary);
    image.write(reinterpret_cast<const char*>(&header),sizeof header);
    image.write(reinterpret_cast<const char*>(&info),sizeof info);
    image.write(reinterpret_cast<const char*>(first.data()),static_cast<std::streamsize>(first.size()*4u));
    check(bool(image),"M01 GPU artifact written");
    // The matched helper reads SDK metadata, including a null raster; no
    // substitute based on format or decoded pixel alpha is used.
    auto* texture=RwTextureCreate(nullptr);
    check(texture && !NativeWorldTextureHasAlpha(texture),"matched SDK null-raster alpha query");
    RwTextureDestroy(texture);
    rejected=false;
    try{(void)NativeWorldTextureHasAlpha(nullptr);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"native alpha adapter enforces original nonnull texture precondition");
}
}
