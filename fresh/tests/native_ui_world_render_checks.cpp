#include "support/native_ui_world_render_probe.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_ui_world_camera.h"
#include "pal4/native_ui_collection_objects.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_effect_creation.h"
#include "pal4/native_ui_world_prerender.h"
#include <rwcore.h>
#include <rpworld.h>
#include "support/native_frame_diagnostics.h"
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace pal4::test {
void VerifyNativeUiWorldRender(const std::filesystem::path& root) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    NativePkFileManagerRuntime packages;
    const auto manager=packages.InitializePackFileManager();std::array<std::uint32_t,20> filesystem{};filesystem[10]=0x1010u;
    packages.ProcessPackFiles(manager,reinterpret_cast<std::uint32_t>(filesystem.data()));
    {struct Directory{std::filesystem::path previous=std::filesystem::current_path();~Directory(){std::filesystem::current_path(previous);}} directory;
        std::filesystem::current_path(root);check(packages.LoadAllScripts()!=0u,"original UI effect package mounts");}
    NativePalGameIVTextureStorage game([](std::uint32_t){});
    check(game.Construct()!=0u,"UI world fixture game storage");
    NativeWorldAsset asset;check(asset.LoadUi(root,"ZJM"),"original ZJM world");
    NativeUiWorldCamera camera;check(camera.Open(asset.World(),800u,600u),"ZJM GPU camera");
    const auto address=reinterpret_cast<std::uint32_t>(camera.Camera());
    std::array<std::uint32_t,0x224u/4u> ui_manager{};
    NativeEffectSceneSession effects(root,packages);effects.BindUiEffects(reinterpret_cast<std::uint32_t>(ui_manager.data()));effects.BindUiCamera(address);
    struct Retire {NativeEffectSceneSession& effects;~Retire(){effects.ClearUiEffects();effects.UnbindUiEffects();}} retire{effects};
    const auto handle=effects.UiEffectCreation().Create("ZJM",{16.0f,209.0f,-70.0f},-1,0u);
    check(handle.system && effects.UiActiveCount()==1u,"original Logo ZJM effect");
    (void)effects.SetUiEffectScale({handle.id,handle.system},{1.2f,1.2f,1.2f},false);
    for(unsigned frame=0;frame<180u;++frame)(void)effects.UpdateUiEffects(1.0/60.0);
    const auto begin=[&]{RwRGBA black{0,0,0,255};
        check(RwCameraClear(camera.Camera(),&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera.Camera()),"ZJM begin GPU frame");};
    const auto pixels=[&](const char* label,unsigned frame){
        check(RwCameraEndUpdate(camera.Camera())!=nullptr,"ZJM end GPU frame");
        DiagnoseNativeFrame(camera.Camera(),label,frame);
        auto* raster=RwCameraGetRaster(camera.Camera());
        std::unique_ptr<RwImage,decltype(&RwImageDestroy)> image(RwImageCreate(800,600,32),&RwImageDestroy);
        check(image && RwImageAllocatePixels(image.get()) && RwImageSetFromRaster(image.get(),raster),"ZJM actual GPU pixels");
        unsigned black{};
        for(int y=0;y<600;++y){const auto* row=reinterpret_cast<const RwRGBA*>(RwImageGetPixels(image.get())+y*RwImageGetStride(image.get()));
            for(int x=0;x<800;++x)black+=row[x].red<32u && row[x].green<32u && row[x].blue<32u;}
        return black;
    };
    // Reproduce the old application's world + every-clump draw on the same
    // assets, camera and UV time before installing game transparent callbacks.
    for(unsigned slot=1;slot<5u;++slot)if(auto* clump=asset.ClumpAt(slot)){
        check(!RpClumpGetWorld(clump),"UI auxiliary clump stays detached like IDA445170");
        check(RpWorldAddClump(asset.World(),clump)!=nullptr,"legacy comparison associates auxiliary clump");}
    begin();check(RpWorldRender(asset.World())!=nullptr,"legacy UI world");
    for(unsigned slot=0;slot<5u;++slot)if(auto* clump=asset.ClumpAt(slot))RpClumpRender(clump);
    effects.DrawUiEffects(address);
    const auto legacy_black=pixels("ui-world-legacy",0u);
    for(unsigned slot=1;slot<5u;++slot)if(auto* clump=asset.ClumpAt(slot))RpWorldRemoveClump(asset.World(),clump);
    auto* trans=asset.ClumpAt(0u);check(trans!=nullptr,"original UI transparent clump");
    using Callback=RpAtomicCallBackRender;
    std::vector<std::pair<RpAtomic*,Callback>> callbacks;
    RpClumpForAllAtomics(trans,[](RpAtomic* atomic,void* context)->RpAtomic*{
        static_cast<decltype(callbacks)*>(context)->emplace_back(atomic,RpAtomicGetRenderCallBack(atomic));return atomic;},&callbacks);
    unsigned native_black{};
    {
        CollectionState state;NativeUiCollectionObjects objects(state,asset.World(),root,packages,game,trans);
        for(unsigned frame=0;frame<2u;++frame){begin();
            if(!frame){objects.RenderWorld(address);
                check(objects.PendingAtomics()!=0u,"original UI world traversal queues transparent atoms without a second clump render");
                effects.DrawUiEffects(address);objects.FlushQueued(address);
            }else NativeUiWorldPreRender(reinterpret_cast<std::uint32_t>(ui_manager.data()),address,objects,effects);
            check(!objects.PendingAtomics(),"UI shared queue flushes once");
            const auto dark=pixels("ui-world-native",frame);
            if(frame)check(dark==native_black,"shared original pre-render matches the independently composed GPU stages");
            native_black=dark;
        }
        objects.Clear();check(!objects.OutstandingQueueAllocations(),"UI background queue storage releases");
    }
    for(const auto& [atomic,callback]:callbacks)check(RpAtomicGetRenderCallBack(atomic)==callback,"UI background callbacks restore before asset destruction");
    std::cout<<"ZJM GPU comparison legacy_dark="<<legacy_black<<" original_path_dark="<<native_black<<" trans_atomics="<<callbacks.size()<<std::endl;
    check(native_black+1000u<legacy_black,"original UI world path removes spurious near-black coverage from the legacy all-clump draw");
    std::cout<<"PASS: original UI world association, SDK traversal, shared transparent queue, repeated frames and callback/storage ownership\n";
}
}
