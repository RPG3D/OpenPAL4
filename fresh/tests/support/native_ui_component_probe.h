#pragma once
#include "pal4/native_ui_component_resource.h"
#include "pal4/native_ui_collection_objects.h"
#include "pal4/native_property_window_model_preview.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_world_material_texture_runtime.h"
#include "native_property_preview_composition_probe.h"
#include "native_property_window_probe.h"
#include "native_system_property_probe.h"
#include "native_equipment_preview_probe.h"
#include "native_smith_preview_probe.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_game_database.h"
#include <rwcore.h>
#include <rpworld.h>
#include <rpuvanim.h>
#include <array>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeUiComponents(const std::filesystem::path& root,bool composition_only=false,bool system_property_only=false,bool equipment_only=false,bool smith_only=false){
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    struct Raster {
        RpWorld* world{};RpLight* light{};RwCamera* camera{};RwFrame* frame{};RwRaster* color{};RwRaster* depth{};
        ~Raster(){
            if(camera && RwCameraGetWorld(camera))RpWorldRemoveCamera(world,camera);
            if(light && world)RpWorldRemoveLight(world,light);
            if(light)RpLightDestroy(light);
            if(camera){RwCameraSetRaster(camera,nullptr);RwCameraSetZRaster(camera,nullptr);RwCameraSetFrame(camera,nullptr);RwCameraDestroy(camera);}
            if(frame)RwFrameDestroy(frame);if(color)RwRasterDestroy(color);if(depth)RwRasterDestroy(depth);if(world)RpWorldDestroy(world);
        }
    } raster;
    RwBBox bounds{{10000.0f,10000.0f,10000.0f},{-10000.0f,-10000.0f,-10000.0f}};
    raster.world=RpWorldCreate(&bounds);check(raster.world!=nullptr,"UI component test world");
    raster.light=RpLightCreate(rpLIGHTAMBIENT);const RwRGBAReal white{1,1,1,1};
    check(raster.light && RpLightSetColor(raster.light,&white) && RpWorldAddLight(raster.world,raster.light),"UI component ambient light");
    raster.camera=RwCameraCreate();raster.frame=RwFrameCreate();
    raster.color=RwRasterCreate(256,256,32,rwRASTERTYPECAMERATEXTURE|rwRASTERFORMAT8888);
    raster.depth=RwRasterCreate(256,256,0,rwRASTERTYPEZBUFFER);
    check(raster.camera && raster.frame && raster.color && raster.depth,"UI component camera resources");
    RwCameraSetFrame(raster.camera,raster.frame);RwCameraSetRaster(raster.camera,raster.color);RwCameraSetZRaster(raster.camera,raster.depth);
    const RwV2d view{0.6f,0.6f};RwCameraSetViewWindow(raster.camera,&view);RpWorldAddCamera(raster.world,raster.camera);
    NativeGameDatabase database;check(database.Load(root),"UI component original Property table");
    NativePalGameIVTextureStorage game([](std::uint32_t){});check(game.Construct()!=0u,"UI component game");
    NativePkFileManagerRuntime packages;std::array<std::uint32_t,20> filesystem{};filesystem[10]=0x1010u;
    const auto manager=packages.InitializePackFileManager();packages.ProcessPackFiles(manager,address(filesystem.data()));
    struct UvDictionary {
        RtDict* previous=RtDictSchemaGetCurrentDict(RpUVAnimGetDictSchema());
        RtDict* owned=RtDictSchemaCreateDict(RpUVAnimGetDictSchema());
        ~UvDictionary(){(void)RtDictSchemaSetCurrentDict(RpUVAnimGetDictSchema(),previous);if(owned)(void)RtDictDestroy(owned);}
    } uv;
    check(uv.owned!=nullptr,"external UV dictionary sentinel");
    (void)RtDictSchemaSetCurrentDict(RpUVAnimGetDictSchema(),uv.owned);
    if(equipment_only){VerifyNativeEquipmentPreview(root,packages,game,database,raster.world,raster.camera);return;}
    if(smith_only){VerifyNativeSmithPreview(root,packages,game,database,raster.world,raster.camera);VerifyNativeSmithOwner(root,packages,game,database,raster.world,raster.camera);return;}
    if(system_property_only){VerifyNativeSystemPropertyFactory(root,packages,game,database,raster.world,raster.camera);return;}
    if(composition_only){
        VerifyNativePropertyPreviewComposition(root,packages,game,database,raster.world,raster.camera);
        VerifyNativePropertyWindow(root,packages,game,database,raster.world,raster.camera);
        VerifyNativeSystemPropertyFactory(root,packages,game,database,raster.world,raster.camera);
        check(RtDictSchemaGetCurrentDict(RpUVAnimGetDictSchema())==uv.owned,"composed Property preview teardown preserves external UV dictionary");
        return;
    }
    NativeUiComponentResource component(root,packages,game);
    CollectionState collection;
    NativeUiCollectionObjects objects(collection,raster.world,root,packages,game);
    check(!component.Load(1u,"palObject\\","MISSING_UI_COMPONENT_MODEL") && !component.Clump() && !component.OutstandingAllocations(),
        "failed UI component load retires all partial resources");
    std::vector<std::uint8_t> raster_pixels;
    const auto render=[&](NativeUiComponentResource& component,bool sorted=false,bool draw=true){
        const auto* sphere=reinterpret_cast<const RwSphere*>(component.State().Data()+56u);
        check(std::isfinite(sphere->radius) && sphere->radius>0.0f,"UI component native bounds sphere");
        const auto radius=std::max(1.0f,sphere->radius);
        const RwV3d eye{sphere->center.x,sphere->center.y,sphere->center.z-radius*3.0f};
        RwFrameSetIdentity(raster.frame);RwFrameTranslate(raster.frame,&eye,rwCOMBINEREPLACE);RwFrameUpdateObjects(raster.frame);
        RwCameraSetNearClipPlane(raster.camera,radius*0.01f);RwCameraSetFarClipPlane(raster.camera,radius*10.0f);
        RwRGBA black{0,0,0,255};check(RwCameraClear(raster.camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ)!=nullptr,"UI component clear");
        check(RwCameraBeginUpdate(raster.camera)!=nullptr,"UI component camera begin");
        const auto visible=!draw?true:sorted?(objects.Render(address(raster.camera)),component.State().Read(28u)!=0u)
                                 :component.Render(address(raster.camera));
        RwCameraEndUpdate(raster.camera);
        check(visible,"original component resource frustum/render path");
        check(!draw || component.State().Read(28u)==(component.State().Read(24u)?1u:0u),
              "draw mode0 skips frustum/draw; enabled visible component publishes visibility");
        auto* image=RwImageCreate(256,256,32);
        check(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image,raster.color),"UI component GPU readback");
        const auto image_depth=RwImageGetDepth(image),image_stride=RwImageGetStride(image);
        check((image_depth==24 || image_depth==32) && image_stride>=256*(image_depth/8),"UI component readback RGB/RGBA row bounds");
        if(!component.State().Read(24u))std::cout<<"Throw GPU readback: depth="<<image_depth<<" stride="<<image_stride<<'\n';
        unsigned nonblack{};raster_pixels.clear();raster_pixels.reserve(256u*256u*3u);
        for(int y=0;y<256;++y){const auto* row=RwImageGetPixels(image)+y*image_stride;
            for(int x=0;x<256;++x){const auto* pixel=row+x*(image_depth/8);nonblack+=(pixel[0] || pixel[1] || pixel[2])?1u:0u;
                raster_pixels.insert(raster_pixels.end(),{pixel[0],pixel[1],pixel[2]});}}
        RwImageDestroy(image);return nonblack;
    };
    const auto& table=database.Tables().at("Property");
    const auto column=[&](const char* name){const auto it=std::find_if(table.fields.begin(),table.fields.end(),[&](const auto& f){return f.name==name;});
        check(it!=table.fields.end(),"UI component Property field");return it-table.fields.begin();};
    const auto model_column=column("PA_MODEL"),texture_column=column("PA_TEXTURE");
    const auto throw_id_column=column("ID");
    const auto attack_property=std::find_if(table.rows.begin(),table.rows.end(),[&](const auto& row){return std::get<std::uint32_t>(row[throw_id_column])==3069u;});
    check(attack_property!=table.rows.end(),"original3069 supplies Throw render operands");
    const auto throw_model=std::get<std::string>((*attack_property)[model_column]),throw_texture=std::get<std::string>((*attack_property)[texture_column]);
    check(component.Load(0u,"palObject\\",throw_model,0u),"original Throw loads actual component with final draw flag0");
    component.SetTexture(throw_texture);
    // Compare actual RGB against the same camera with no component call. The
    // SDK readback can retain nonblack pixels even on a nominal black clear;
    // exact baseline equality detects any added geometry without a threshold.
    const auto baseline_throw_pixels=render(component,false,false);
    const auto baseline_throw_rgb=raster_pixels;
    const auto hidden_throw_pixels=render(component);
    check(!RpClumpGetWorld(component.Clump()) && component.State().Read(24u)==0u && raster_pixels==baseline_throw_rgb,
          "original hidden Throw component owns real resources but adds no RGB changes to no-draw baseline outside world");
    component.Clear();check(!component.OutstandingAllocations(),"hidden Throw component cleanup releases original resources");
    check(component.Load(0u,"palObject\\",throw_model,1u),"same original model loads for explicit enabled preview control");
    component.SetTexture(throw_texture);const auto enabled_throw_pixels=render(component);
    check(enabled_throw_pixels>baseline_throw_pixels+20u && raster_pixels!=baseline_throw_rgb,
          "enabled control proves hidden Throw pixels were not caused by missing model or texture");
    component.Clear();check(!component.OutstandingAllocations(),"enabled control resources also retire");
    std::cout<<"Original Throw component render: model="<<throw_model<<" texture="<<throw_texture
             <<" mode0/world_none added_RGB_changes=0 baseline_nonblack="<<baseline_throw_pixels
             <<" hidden_nonblack="<<hidden_throw_pixels<<" same-resource mode1 control pixels="<<enabled_throw_pixels<<'\n';
    std::set<std::pair<std::string,std::string>> variants;std::set<std::string> models;
    std::string previous;unsigned loads{},drawn{},max_pixels{},empty_models{};
    std::uint32_t id=2u;
    for(const auto& row:table.rows){
        const auto model=std::get<std::string>(row[model_column]),texture=std::get<std::string>(row[texture_column]);
        if(model.empty()){++empty_models;continue;}
        if(!variants.emplace(model,texture).second)continue;
        models.insert(model);
        if(_stricmp(previous.c_str(),model.c_str())){
            component.Clear();check(!component.Clump() && !component.OutstandingAllocations(),"UI component replacement cleanup");
            if(!component.Load(id++,"palObject\\",model)){
                std::cout<<"UI component missing model="<<model<<'\n';check(false,"original Property model resource loads");}
            previous=model;++loads;
            check(RtDictSchemaGetCurrentDict(RpUVAnimGetDictSchema())==uv.owned,"component loading restores another owner's UV dictionary");
            check(component.State().Read(0x170u)==id-1u && component.State().Read(0x14u)==address(component.Clump()),"original component ID/clump image fields");
            component.AddToWorld(address(raster.world));
        }
        component.SetTexture(texture);component.SetBaseRotation({30.0f,10.0f,0.0f});
        component.SetPosition({-59.669998f,349.85999f,298.26001f});
        component.SetRotationRates(true,{0.0f,50.0f,0.0f});
        const auto before=*RwFrameGetMatrix(RpClumpGetFrame(component.Clump()));component.Advance(0.25f);
        const auto after=*RwFrameGetMatrix(RpClumpGetFrame(component.Clump()));
        check(std::memcmp(&before,&after,sizeof(RwMatrix))!=0,"original component rates rotate actual SDK frame");
        check(std::fabs(after.pos.x+59.669998f)<0.02f && std::fabs(after.pos.y-349.85999f)<0.02f &&
            std::fabs(after.pos.z-298.26001f)<0.02f,"component rotation preserves original preview position");
        const auto pixels=render(component);max_pixels=std::max(max_pixels,pixels);drawn+=pixels>20u;
        std::cout<<"UI component model="<<model<<" texture="<<texture<<" pixels="<<pixels<<'\n';
    }
    check(loads && drawn==variants.size() && max_pixels>20u,"every original Property model/texture variant renders visible GPU pixels");
    component.Clear();check(!component.Clump() && !component.OutstandingAllocations(),"Property components release all resources");
    check(component.Load(id,"ui\\model\\","_ui_flower"),"original UI preview pedestal loads from UI package");
    component.SetScale({3.5f,3.5f,3.5f});component.SetPosition({-59.669998f,309.85999f,298.26001f});
    component.AddToWorld(address(raster.world));check(render(component)>20u,"original UI preview pedestal renders");
    component.Clear();component.Clear();check(!component.OutstandingAllocations(),"UI pedestal and repeated cleanup release allocations");
    check(RtDictSchemaGetCurrentDict(RpUVAnimGetDictSchema())==uv.owned,"component cleanup preserves external UV dictionary");
    unsigned sorted_drawn{},queued_variants{};std::uint32_t previous_id{};
    for(const auto& [model,texture]:variants){
        auto* object=objects.Find(previous_id);
        if(!object || _stricmp(previous.c_str(),model.c_str())){
            objects.Remove(previous_id);
            object=&objects.Create("palObject\\",model);
            previous_id=object->State().Read(0x170u);previous=model;
            check(previous_id && objects.Find(previous_id)==object && objects.Size()==1u,
                "shared collection create/ID lookup and replacement");
        }
        object->SetTexture(texture);object->SetBaseRotation({30.0f,10.0f,0.0f});
        object->SetPosition({-59.669998f,349.85999f,298.26001f});
        object->SetRotationRates(true,{0.0f,50.0f,0.0f});objects.Advance(object->Address(),0.25);
        const auto pixels=render(*object,true);sorted_drawn+=pixels>20u;
        queued_variants+=objects.OutstandingQueueAllocations()>0u;
        check(objects.PendingAtomics()==0u,"collection sorted pass flushes every atomic");
        std::cout<<"UI sorted model="<<model<<" texture="<<texture<<" pixels="<<pixels<<'\n';
    }
    check(sorted_drawn==variants.size() && queued_variants>0u,"every Property variant renders through original collection atomic queues");
    objects.Clear();check(objects.Size()==0u && !collection.first && !objects.OutstandingQueueAllocations(),
        "collection clear retires owners, published range and queue allocations");
    check(RtDictSchemaGetCurrentDict(RpUVAnimGetDictSchema())==uv.owned,"component destruction preserves external UV dictionary");
    std::vector<std::uint32_t> ids;
    for(unsigned i=0;i<12u;++i){auto& o=objects.Create("palObject\\",variants.begin()->first);ids.push_back(o.State().Read(0x170u));}
    check(objects.Size()==12u && collection.last-collection.first==12,"collection grows beyond original initial capacity");
    objects.Remove(ids[5]);objects.Remove(ids[0]);objects.Remove(ids[11]);objects.Remove(0xffffffffu);
    check(objects.Size()==9u && !objects.Find(ids[5]) && !objects.Find(ids[0]) && !objects.Find(ids[11]) && objects.Find(ids[6]),
        "shared collection erases middle, first and last without disturbing surviving IDs");
    const auto retained_id=ids.back();objects.Clear();objects.Clear();
    auto& missing=objects.Create("palObject\\","MISSING_UI_COLLECTION_MODEL");
    check(!missing.Clump() && missing.State().Read(0x170u)>retained_id,"missing model keeps original empty component and monotonic ID");
    objects.Remove(missing.State().Read(0x170u));
    try{(void)objects.Create("unsupported\\","bad");check(false,"unsupported component root must throw");}
    catch(const std::invalid_argument&){}
    check(!objects.Size() && !collection.first,"failed component create rolls back published ownership");
    objects.Clear();check(!objects.OutstandingQueueAllocations(),"collection final queue cleanup");
    check(RtDictSchemaGetCurrentDict(RpUVAnimGetDictSchema())==uv.owned,"collection ownership preserves external UV dictionary");
    NativeInventorySession inventory;(void)inventory.Start();inventory.BindDatabase(database);
    inventory.BindItemObserver([](std::uint32_t){});
    PropertyWindowState preview_state;
    NativePropertyWindowModelPreview preview(preview_state,inventory,objects,packages);
    const auto id_column=column("ID");unsigned preview_drawn{},preview_reused{},preview_replaced{},mixed_texture_records{},missing_texture_records{};
    NativePackageArchive object_archive;check(object_archive.Open((root/"gamedata/palobject.cpk").string().c_str()),"preview texture evidence archive");
    std::uint32_t last_entry{},last_property{},last_preview_id{};std::string last_model;
    check(packages.AddPrimaryScriptAndProcess()!=0u,"preview nested outer package scope");
    const auto script_depth=[&]{return *reinterpret_cast<const std::uint32_t*>(manager+120u);};
    const auto outer_depth=script_depth();
    for(const auto& row:table.rows){
        const auto property=std::get<std::uint32_t>(row[id_column]);
        const auto entry=inventory.AddItem(property,1);check(entry!=0u,"preview actual inventory entry");
        const auto model=std::get<std::string>(row[model_column]),texture=std::get<std::string>(row[texture_column]);
        (void)preview.Update(preview_state,entry);
        const auto current_id=preview_state.Read(0x78u);auto* object=objects.Find(current_id);
        check(object && object->Clump() && objects.Size()==1u,"preview owns exactly one real loaded component");
        check(object->State().Read(4u) && model==reinterpret_cast<const char*>(object->State().Read(4u)),
              "preview actual component model metadata");
        const auto replacement=object->State().Read(16u);
        if(replacement)check(texture==reinterpret_cast<const char*>(replacement),"preview applied texture metadata matches original request");
        else{
            NativeWorldMaterialTextureRuntime materials;
            if(!GameWorldMaterialOperations::CheckClumpTexturePathOnly(address(object->Clump()),materials))++mixed_texture_records;
            else{
                const auto member=model+"\\"+texture+".png";
                std::cout<<"UI preview no replacement model="<<model<<" texture="<<texture<<std::endl;
                check(!object_archive.ReadMember(member.c_str()),"uniform preview without replacement has no requested texture in original archive");
                ++missing_texture_records;
            }
        }
        if(last_preview_id){
            if(!_stricmp(last_model.c_str(),model.c_str())){check(current_id==last_preview_id,"same model preview reuses ID");++preview_reused;}
            else{check(current_id>last_preview_id && !objects.Find(last_preview_id),"different model preview retires previous ID");++preview_replaced;}
        }
        const auto* matrix=RwFrameGetMatrix(RpClumpGetFrame(object->Clump()));
        check(std::fabs(matrix->pos.x+59.669998f)<0.02f && std::fabs(matrix->pos.y-349.85999f)<0.02f &&
              std::fabs(matrix->pos.z-298.26001f)<0.02f,"shared model preview sets original UI position");
        objects.Advance(object->Address(),0.1);
        preview_drawn+=render(*object,true)>20u;
        check(script_depth()==outer_depth,"preview restores caller's outer package scope");
        last_entry=entry;last_property=property;last_preview_id=current_id;last_model=model;
    }
    check(preview_drawn==table.rows.size() && preview_reused && preview_replaced,"all original inventory previews render with reuse and replacement");
    (void)packages.CleanupAllScripts();
    (void)inventory.RemoveItem(last_property,1);
    bool stale_rejected{};try{(void)preview.Update(preview_state,last_entry);}catch(const std::invalid_argument&){stale_rejected=true;}
    check(stale_rejected && preview_state.Read(0x78u)==last_preview_id,"stale preview entry is rejected before dereference or mutation");
    preview.Clear();preview.Clear();check(!objects.Size() && !preview_state.Read(0x78u),"empty details clear original model ID and component");
    const auto entry=inventory.AddItem(last_property,1);(void)preview.Update(preview_state,entry);
    auto& pedestal=objects.Create("ui\\model\\","_ui_flower");const auto pedestal_id=pedestal.State().Read(0x170u);
    preview.Close();preview.Close();check(objects.Size()==1u && objects.Find(pedestal_id) && !preview_state.Read(0x78u),
        "preview close preserves independently owned pedestal");
    bool closed_rejected{};try{(void)preview.Update(preview_state,entry);}catch(const std::logic_error&){closed_rejected=true;}
    check(closed_rejected,"closed preview endpoint rejects callbacks");
    objects.Clear();check(!objects.OutstandingQueueAllocations(),"preview final collection cleanup");
    check(RtDictSchemaGetCurrentDict(RpUVAnimGetDictSchema())==uv.owned,"preview destruction preserves external UV dictionary");
    std::cout<<"PASS: Property native model preview records="<<preview_drawn<<" reused="<<preview_reused<<" replaced="<<preview_replaced
        <<" mixed_texture_records="<<mixed_texture_records
        <<" missing_texture_records="<<missing_texture_records
        <<" actual inventory, package scopes, stale entry, close and independent pedestal\n";
    std::cout<<"PASS: UI collection sorted_variants="<<sorted_drawn<<" queued_variants="<<queued_variants
        <<" growing collection, shared IDs/find/remove, rollback and cleanup\n";
    std::cout<<"PASS: UI component unique_models="<<models.size()<<" loads="<<loads<<" variants="<<variants.size()<<" drawn="<<drawn
        <<" empty_models="<<empty_models<<" original pedestal, rotation, textures and cleanup\n";
}
}
