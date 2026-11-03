#pragma once
#include "pal4/native_equipment_model_preview.h"
#include "pal4/native_equipment_window.h"
#include "pal4/native_role_record_image.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/player_manager_slot_data_copier.h"
#include "pal4/player_manager_slot.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_common_dialog_layout.h"
#include "native_property_window_layout.h"
#include "native_cegui_test_click.h"
#include "pal4/native_menu_window.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_ui_collection_objects.h"
#include "pal4/native_ui_component_resource.h"
#include <rwcore.h>
#include <rpworld.h>
#include <rtfsyst.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
namespace pal4::test {
inline void VerifyNativeEquipmentPreview(const std::filesystem::path& root,NativePkFileManagerRuntime& packages,
    NativePalGameIVTextureStorage& game,NativeGameDatabase& db,RpWorld* world,RwCamera* camera){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    {struct Directory{std::filesystem::path previous=std::filesystem::current_path();~Directory(){std::filesystem::current_path(previous);}} directory;
        std::filesystem::current_path(root);check(packages.LoadAllScripts()!=0u,"equipment preview mounts original packages");}
    CollectionState collection;NativeUiCollectionObjects objects(collection,world,root,packages,game);
    NativeInventorySession inventory;(void)inventory.Start();inventory.BindDatabase(db);inventory.BindItemObserver([](auto){});
    std::array<std::uint32_t,0x224u/4u> manager{};NativeEffectSceneSession effects(root,packages);
    effects.BindUiEffects(address(manager.data()));effects.BindUiCamera(address(camera));
    struct Retire{NativeEffectSceneSession& e;~Retire(){try{e.ClearUiEffects();e.UnbindUiEffects();}catch(...){}}} retire{effects};
    EquipmentWindowState state;NativeEquipmentModelPreview preview(state,inventory,objects,effects,packages);
    const auto draw=[&](NativeUiComponentResource& object){
        const auto& sphere=*reinterpret_cast<const RwSphere*>(object.State().Data()+56u);
        const auto radius=std::max(1.f,sphere.radius);const RwV3d eye{sphere.center.x,sphere.center.y,sphere.center.z-radius*3.f};
        auto* frame=RwCameraGetFrame(camera);RwFrameSetIdentity(frame);RwFrameTranslate(frame,&eye,rwCOMBINEREPLACE);RwFrameUpdateObjects(frame);
        RwCameraSetNearClipPlane(camera,radius*0.01f);RwCameraSetFarClipPlane(camera,radius*10.f);
        RwRGBA black{0,0,0,255};check(RwCameraClear(camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera),"equipment preview camera begins");
        check(object.Render(address(camera)),"equipment item alone reaches native render path");RwCameraEndUpdate(camera);
        auto* image=RwImageCreate(256,256,32);check(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image,RwCameraGetRaster(camera)),"equipment preview GPU readback");
        unsigned pixels{};for(int y=0;y<256;++y){const auto* row=reinterpret_cast<const RwRGBA*>(RwImageGetPixels(image)+y*RwImageGetStride(image));
            for(int x=0;x<256;++x)pixels+=(row[x].red || row[x].green || row[x].blue)?1u:0u;}RwImageDestroy(image);
        check(RwCameraClear(camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera),"equipment sorted camera begins");
        objects.Render(address(camera));RwCameraEndUpdate(camera);check(!objects.PendingAtomics(),"equipment preview flushes sorted atomics");return pixels;};
    preview.SetVisible(true);const auto flower=state.Read(0xfcu);const EffectSystemHandleWords effect{state.Read(0x104u),state.Read(0x108u)};
    auto* pedestal=objects.Find(flower);check(pedestal && pedestal->Clump() && objects.Size()==1u && effects.UiActiveCount()==1u,"equipment original pedestal and LH created");
    const auto* flower_matrix=RwFrameGetMatrix(RpClumpGetFrame(pedestal->Clump()));
    check(std::fabs(RwV3dLength(&flower_matrix->right)-3.5f)<0.001f && std::fabs(flower_matrix->pos.y-309.85999f)<0.02f,"equipment pedestal scale and position");
    auto* effect_frame=reinterpret_cast<RwFrame*>(effects.UiEffectFrame(effect));check(effect_frame,"equipment LH actual frame");
    check(std::fabs(RwV3dLength(&RwFrameGetMatrix(effect_frame)->right)-2.2f)<0.001f && std::fabs(RwFrameGetMatrix(effect_frame)->pos.y-200.86f)<0.02f,"equipment LH scale and position");
    preview.SetVisible(true);check(state.Read(0xfcu)==flower && effects.UiActiveCount()==1u,"equipment repeated show reuses pedestal/effect");
    unsigned weapons{},items{},drawn{},empty{},max_pixels{},scaled_weapons{};std::uint32_t previous{};
    for(const auto& row:db.Tables().at("Equip").rows){
        const auto record=inventory.ProcessEquipment(inventory.Address()+0x7cu,std::get<std::uint32_t>(row[0]));check(record,"equipment preview real inventory acquisition");
        const auto name_ptr=inventory.ReadDword(record+0xdcu);const std::string name=name_ptr?reinterpret_cast<const char*>(name_ptr):"";
        if(name.empty()){++empty;continue;}
        const auto category=inventory.ReadDword(record+0x94u);std::cout<<"Equipment load id="<<std::get<std::uint32_t>(row[0])<<" model="<<name<<" type="<<category<<std::endl;
        preview.Update(record);auto* object=objects.Find(state.Read(0xf8u));
        if(!object || !object->Clump())std::cerr<<"Equipment missing clump model="<<name<<" type="<<category<<'\n';
        check(object && object->Clump() && objects.Size()==2u,"equipment model loads beside original pedestal");
        check(!std::strcmp(name.c_str(),reinterpret_cast<const char*>(object->State().Read(4u))),"equipment model matches live record");
        const auto id=state.Read(0xf8u);preview.Update(record);check(state.Read(0xf8u)==id && objects.Find(id)==object,"same model texture update reuses object");
        const auto* matrix=RwFrameGetMatrix(RpClumpGetFrame(object->Clump()));
        if(category<=2u){const auto& image=object->State();
            for(unsigned axis=0;axis<3u;++axis)check(image.Read(0xb8u+axis*4u)==image.Read(0xc4u+axis*4u),"weapon applies configured original scale");
            const auto scale=std::bit_cast<float>(image.Read(0xc4u));
            scaled_weapons+=std::fabs(scale-1.f)>0.001f;
            check(std::fabs(RwV3dLength(&matrix->right)-scale)<0.01f,"weapon SDK frame carries configured scale");}
        check(std::fabs(matrix->pos.x+59.669998f)<0.02f && std::fabs(matrix->pos.y-349.85999f)<0.02f && std::fabs(matrix->pos.z-298.26001f)<0.02f,"equipment original preview position");
        const auto before=*matrix;objects.Advance(object->Address(),0.1);check(std::memcmp(&before,RwFrameGetMatrix(RpClumpGetFrame(object->Clump())),sizeof(before))!=0,"equipment actual SDK rotation advances");
        const auto pixels=draw(*object);max_pixels=std::max(max_pixels,pixels);drawn+=pixels>20u;
        if(category<=2u)++weapons;else ++items;previous=id;
        std::cout<<"Equipment preview model="<<name<<" type="<<category<<" pixels="<<pixels<<'\n';
    }
    check(weapons && items && scaled_weapons && drawn==weapons+items && max_pixels>20u,"every original nonempty equipment model renders alone and configured weapon scale is exercised");
    preview.Update(0u);check(state.Read(0xf8u)==previous,"null equipment preview retains original no-op");
    bool stale{};try{preview.Update(0xdeadbeefu);}catch(const std::invalid_argument&){stale=true;}check(stale,"equipment stale pointer rejected before dereference");
    check(preview.RemoveItem(previous)==0u && !state.Read(0xf8u) && objects.Size()==1u,"details clears current item independently of pedestal");
    preview.SetVisible(false);check(!objects.Size() && !state.Read(0xfcu) && !state.Read(0x100u) && !state.Read(0x108u) && !effects.UiActiveCount(),"equipment hide releases models and LH");
    preview.SetVisible(true);check(objects.Size()==1u && effects.UiActiveCount()==1u,"equipment preview reopens after teardown");
    preview.Close();preview.Close();objects.Clear();check(!objects.OutstandingQueueAllocations() && !collection.first,"equipment preview clears queue and collection");
    {
        struct Files{bool ready{};~Files(){if(ready)RtFSManagerClose();}} files;
        files.ready=RtFSManagerOpen(8)!=FALSE;check(files.ready,"complete equipment page filesystem");
        NativeCeguiUiSession ui;check(ui.Start(root,800,600),"complete equipment page real DLL UI");
        NativePropertyWindowLayout bootstrap(ui,root);check(ui.BindDialogRootParent(bootstrap.state.Read(36u),false),"hide equipment UI bootstrap");
        check(ui.LoadOriginalMinimapLayout(root) && ui.SetMinimapLayoutVisible(false),"equipment page original systemBold font");
        PlayerManagerObjectState players{};NativeRoleRecordCatalog roles(db);NativePlayerSlotCopyRuntime copier;
        NativeBaseObjectMemory memory;NativePlayerScriptRefreshRuntime refresh(memory,inventory);
        for(unsigned c=0;c<4u;++c){const auto p=address(&players.slots[c]);(void)PlayerManagerSlotDataCopier::Copy(p,roles.Address(c),copier);
            players.Word((c*0xb14u+0x654u)/4u)=PlayerManagerSlot::kScriptVtable;
            (void)NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x44u,4u,40u);players.Word((c*0xb14u+0xb08u)/4u)=c<2u?1u:0u;(void)refresh.Refresh(p+0x654u);}
        NativeEquipmentWindow* current{};
        NativeEventHandlerArray activity([&](auto m,auto s){if(current && current->OwnsActivity(m))return current->Callback(m,s);check(m==0xabcdu,"complete page known parent callback");return 1u;},
            [&](auto m,auto s,auto e)->std::uint8_t{if(current && current->OwnsActivity(m))return current->Predicate(m,s,e);check(m==0xabcdu,"complete page known parent predicate");return 0u;});
        NativeCommonDialogLayout confirmation(bootstrap.module,ui,root);std::vector<unsigned> events;unsigned tutorials{};
        NativeEquipmentWindowServices services;services.event=[&](unsigned n,auto,auto){events.push_back(n);};
        services.tutorial=[&](unsigned id){check(id==3u,"complete page original tutorial");++tutorials;};services.sound=[](auto,unsigned mode){if(mode!=1u)throw std::logic_error("equipment page sound mode");};
        NativeEquipmentWindow page(ui,root,inventory,players,db,refresh,objects,effects,packages,activity,0x1000u,confirmation,std::move(services));current=&page;
        (void)activity.Insert(0xabcdu);page.SetVisible(true);
        auto* first=objects.Find(page.State().Read(0xf8u));check(first && first->Clump() && objects.Size()==2u && effects.UiActiveCount()==1u,"complete page entry creates actual item/pedestal/LH owners");
        check(draw(*first)>20u,"complete page actual selected equipment renders isolated GPU pixels");
        for(unsigned category=0;category<6u;++category){
            std::array<unsigned,5> key{};key[4]=category==5u?2u:category+3u;
            check(activity.Contains(address(key.data())) && page.State().Read(0xf0u)==category,"complete page category shortcut routes through actual activity predicate chain");page.RethrowEventError();
            auto* item=objects.Find(page.State().Read(0xf8u));check(item && item->Clump() && objects.Size()==2u && effects.UiActiveCount()==1u,"complete category update replaces actual item and preserves pedestal/effect");
        }
        {
            using detail::Export;const auto module=bootstrap.module;
            auto* row=Export<void* (__thiscall*)(const void*)>(module,"?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ")(reinterpret_cast<void*>(page.State().Read(0x40u)));
            check(row,"complete page live row before discard");const auto sequence=Export<unsigned (__thiscall*)(const void*)>(module,"?getID@ListboxItem@CEGUI@@QBEIXZ")(row);
            std::array<unsigned,5> enter{};enter[4]=28u;
            check(activity.DispatchKeyDown(activity.GetLast(),address(enter.data())) && page.Menu().Visible(),"complete page real menu before discard");
            ClickNativeCeguiWindow(ui,page.Menu().Button(1u));page.RethrowEventError();
            ClickNativeCeguiWindow(ui,confirmation.Window("BtNo"));page.RethrowEventError();
            check(confirmation.LastSelection()==2u && inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequence,7u),"complete page actual No preserves current inventory/model selection");
            check(activity.DispatchKeyDown(activity.GetLast(),address(enter.data())) && page.Menu().Visible(),"complete page discard menu reopens");
            ClickNativeCeguiWindow(ui,page.Menu().Button(1u));page.RethrowEventError();
            ClickNativeCeguiWindow(ui,confirmation.Window("BtYes"));page.RethrowEventError();
            check(confirmation.LastSelection()==1u && !inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequence,7u) && objects.Size()==2u && effects.UiActiveCount()==1u,
                "complete page actual Yes removes instance and synchronously replaces real item preview without losing pedestal/effect");
        }
        std::array<unsigned,5> key{};key[4]=1u;
        check(activity.DispatchKeyDown(activity.GetLast(),address(key.data())) && objects.Size()==0u && !effects.UiActiveCount(),"complete page first Esc retires real item/pedestal/LH");
        check(activity.DispatchKeyDown(activity.GetLast(),address(key.data())) && activity.GetLast()==0xabcdu && events.back()==0x8a80acu,"complete page second Esc returns parent toolbar");
        page.SetVisible(true);check(objects.Size()==2u && effects.UiActiveCount()==1u && tutorials==2u,"complete page reopen reconstructs actual preview");
        page.Close();page.Close();check(activity.Size()==1u && !objects.Size() && !effects.UiActiveCount() && !page.OwnsActivity(page.FrameAddress()+0x28u),"complete page Close drains activities/models/effects and callbacks before XML teardown");
        activity.ReleaseStorage();objects.Clear();check(!objects.OutstandingQueueAllocations() && !memory.OutstandingAllocations(),"complete page native queue/refresh scratch cleaned");
        std::cout<<"PASS: complete native equipment page actual DLL layout + six category shortcuts + real188 inventory + model/pedestal/LH + isolated GPU + Esc/reopen/Close\n";
    }
    inventory.Close();check(!inventory.OutstandingAllocations(),"equipment preview inventory cleanup");
    std::cout<<"PASS: equipment preview weapons="<<weapons<<" objects="<<items<<" empty_models="<<empty<<" drawn="<<drawn<<" max_pixels="<<max_pixels<<" original pedestal/LH/reuse/rotation/cleanup\n";
}
}
