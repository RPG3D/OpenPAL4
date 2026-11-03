#pragma once
#include <rwcore.h>
#include <rpworld.h>
#include <rtfsyst.h>
#include "native_property_window_layout.h"
#include "pal4/native_property_window_panel_preview.h"
#include "pal4/native_property_window_model_preview.h"
#include "pal4/native_property_window_details.h"
#include "pal4/native_property_window_list.h"
#include "pal4/native_inventory_property_reader.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_ui_collection_objects.h"
#include "pal4/native_ui_component_resource.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>

namespace pal4::test {
inline void VerifyNativePropertyPreviewComposition(const std::filesystem::path& root,
    NativePkFileManagerRuntime& packages,NativePalGameIVTextureStorage& game,
    NativeGameDatabase& database,RpWorld* world,RwCamera* camera){
    using detail::Export;
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    const auto ptr=[](std::uint32_t a){return reinterpret_cast<void*>(a);};
    {
        struct Directory {std::filesystem::path previous=std::filesystem::current_path();
            ~Directory(){std::filesystem::current_path(previous);}} directory;
        std::filesystem::current_path(root);
        check(packages.LoadAllScripts()!=0u,"composed Property startup package mounting");
    }
    struct Filesystem {bool ready{};~Filesystem(){if(ready)RtFSManagerClose();}} files;
    files.ready=RtFSManagerOpen(8)!=FALSE;check(files.ready,"composed Property SDK filesystem");
    NativeCeguiUiSession ui;check(ui.Start(root,800,600),"composed Property real DLL UI");
    NativePropertyWindowLayout layout(ui,root);auto& state=layout.state;const auto module=layout.module;
    CollectionState collection;NativeUiCollectionObjects objects(collection,world,root,packages,game);
    NativeInventorySession inventory;(void)inventory.Start();inventory.BindDatabase(database);inventory.BindItemObserver([](auto){});
    NativeInventoryPropertyReader reader(inventory);
    const auto& table=database.Tables().at("Property");
    const auto id_column=std::find_if(table.fields.begin(),table.fields.end(),[](const auto& f){return f.name=="ID";})-table.fields.begin();
    for(const auto& row:table.rows)check(inventory.AddItem(std::get<std::uint32_t>(row[id_column]),1)!=0u,"composed Property inventory");
    std::array<std::uint32_t,0x224u/4u> manager{};
    NativeEffectSceneSession effects(root,packages);effects.BindUiEffects(address(manager.data()));effects.BindUiCamera(address(camera));
    {
        NativePackageArchive archive;check(archive.Open((root/"gamedata/Effect.cpk").string().c_str()),"LH original archive evidence");
        const auto data=archive.ReadMember("script\\LH.eft");
        std::cout<<"Property LH document_bytes="<<(data?data->size():0u)<<" group_enabled="<<(manager[1]&255u)<<std::endl;
        if(data){std::ofstream file("H:/PAL4/objs/M09-property-panel-20261019-LH.eft",std::ios::binary);
            file.write(reinterpret_cast<const char*>(data->data()),static_cast<std::streamsize>(data->size()));}
    }
    struct Retire {NativeEffectSceneSession& effects;~Retire(){try{effects.ClearUiEffects();effects.UnbindUiEffects();}catch(...){}}} retire{effects};
    NativePropertyWindowPanelPreview panel(module,state,objects,effects,packages);
    NativePropertyWindowModelPreview model(state,inventory,objects,packages);
    NativePropertyWindowDetails details(module,state,inventory,[&](auto& image,auto record){return model.Update(image,record);});
    NativePropertyWindowListServices services;
    services.item_panel=[&](auto& image,bool visible){check(&image==&state,"composed Property panel state");panel.SetItemPanel(visible);};
    services.clear_details=[&](auto& image){check(&image==&state,"composed Property detail state");details.Clear();};
    NativePropertyWindowList list(module,layout.look,state,inventory,std::move(services));
    unsigned hovered{};std::uint32_t first_flower{},first_effect{};
    for(unsigned category=0;category<5u;++category){
        list.Populate(category);details.RethrowEventError();
        auto* flower=objects.Find(state.Read(0x7cu));check(flower && flower->Clump(),"composed original pedestal exists");
        const EffectSystemHandleWords handle{state.Read(0x84u),state.Read(0x88u)};
        check(effects.UiDialogNeedsUpdate(handle) && effects.UiActiveCount()==1u,"composed original LH active in UI group");
        if(!category){first_flower=state.Read(0x7cu);first_effect=handle.id;}
        check(first_flower==state.Read(0x7cu) && first_effect==handle.id,"category refresh reuses pedestal and LH");
        const auto* flower_matrix=RwFrameGetMatrix(RpClumpGetFrame(flower->Clump()));
        check(std::fabs(RwV3dLength(&flower_matrix->right)-3.5f)<0.001f &&
            std::fabs(flower_matrix->pos.y-309.85999f)<0.02f,"composed pedestal original scale and position");
        auto* effect_frame=reinterpret_cast<RwFrame*>(effects.UiEffectFrame(handle));check(effect_frame!=nullptr,"composed LH actual frame");
        const auto* effect_matrix=RwFrameGetMatrix(effect_frame);
        check(std::fabs(RwV3dLength(&effect_matrix->right)-2.2f)<0.001f &&
            std::fabs(effect_matrix->pos.y-200.86f)<0.02f,"composed LH original scale and position");
        const auto rows=Export<std::uint32_t (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(ptr(state.Read(0x58u)));
        for(unsigned row=0;row<rows;++row){
            Export<void (__thiscall*)(void*,std::int32_t)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(ptr(state.Read(0x58u)),row);
            details.RethrowEventError();
            const auto live=reader.FindSequence(state.Read(0x74u));auto* object=objects.Find(state.Read(0x78u));
            check(live && object && object->Clump() && objects.Size()==2u,"actual DLL hover owns item plus pedestal");
            const auto name=*reinterpret_cast<const std::uint32_t*>(live->definition+0x54u);
            check(!std::strcmp(reinterpret_cast<const char*>(name),reinterpret_cast<const char*>(object->State().Read(4u))),
                "actual DLL hover resolves live original model");++hovered;
        }
    }
    check(hovered==table.rows.size(),"all original Property rows reach actual hover/model composition");
    const auto update=[&]{effects.UpdateUiEffects(0.1);};
    const auto handle=EffectSystemHandleWords{state.Read(0x84u),state.Read(0x88u)};
    auto* effect_frame=reinterpret_cast<RwFrame*>(effects.UiEffectFrame(handle));
    const auto origin=RwFrameGetLTM(effect_frame)->pos;
    const RwV3d eye{origin.x,origin.y,origin.z-500.0f};auto* camera_frame=RwCameraGetFrame(camera);
    RwFrameSetIdentity(camera_frame);RwFrameTranslate(camera_frame,&eye,rwCOMBINEREPLACE);RwFrameUpdateObjects(camera_frame);
    RwCameraSetNearClipPlane(camera,1.0f);RwCameraSetFarClipPlane(camera,2000.0f);
    unsigned effect_pixels{};
    for(unsigned frame=0;frame<8u;++frame){
        update();RwRGBA black{0,0,0,255};check(RwCameraClear(camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera),"composed LH camera pass");
        effects.DrawUiEffects(address(camera));RwCameraEndUpdate(camera);
        auto* image=RwImageCreate(256,256,32);check(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image,RwCameraGetRaster(camera)),"LH GPU readback");
        unsigned pixels{};for(int y=0;y<256;++y){const auto* row=reinterpret_cast<const RwRGBA*>(RwImageGetPixels(image)+y*RwImageGetStride(image));
            for(int x=0;x<256;++x)pixels+=(row[x].red || row[x].green || row[x].blue)?1u:0u;}
        effect_pixels=std::max(effect_pixels,pixels);RwImageDestroy(image);
    }
    check(effect_pixels>20u,"original LH alone produces visible GPU pixels");
    panel.SetItemPanel(false);details.RethrowEventError();
    check(objects.Size()==0u && !state.Read(0x78u) && !state.Read(0x7cu) && !state.Read(0x80u) &&
          !state.Read(0x88u) && effects.UiActiveCount()==0u,"hide retires item, pedestal and own LH handle");
    for(auto offset:{0x8cu,0x90u,0x94u})check(!Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z")(ptr(state.Read(offset)),true),"actual Property panel controls hide");
    panel.SetItemPanel(true);check(objects.Size()==1u && effects.UiActiveCount()==1u && state.Read(0x7cu)>first_flower,"reopen creates a fresh pedestal and LH");
    details.Close();list.Clear();model.Close();panel.Close();panel.Close();objects.Clear();
    check(!effects.UiActiveCount() && !objects.OutstandingQueueAllocations(),"composed Property close drains its resources");
    std::cout<<"PASS: actual DLL Property hover/model/panel records="<<hovered<<" LH_pixels="<<effect_pixels
        <<" original pedestal/effect transforms, reuse, hide/reopen and teardown\n";
}
}
