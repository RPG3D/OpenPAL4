#include "support/native_system_property_probe.h"
#include "support/native_exception_trace.h"
#include "pal4/native_trade_window_details.h"
#include "pal4/native_trade_window_preview.h"
#include "pal4/native_trade_window_layout.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_inventory_property_reader.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_ui_collection_objects.h"
#include "pal4/native_ui_component_resource.h"
#include "pal4/native_cegui_default_list_item.h"
#include "pal4/native_cegui_text_conversion.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/player_manager_object.h"
#include "pal4/native_trade_window_comparison.h"
#include "pal4/native_player_snapshot.h"
#include "pal4/native_role_record_image.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/player_manager_slot_data_copier.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/native_player_skill_callbacks.h"
#include "pal4/native_player_skill_runtime.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/random_number_generator.h"
#include "pal4/inventory_item_slots.h"
#include "pal4/player_manager_slot.h"
#include <cstring>
#include "../native/ui/cegui_exports_native.h"
#include <rwcore.h>
#include <rpworld.h>
#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
namespace pal4::test {
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}unsigned Address(const void* p){return reinterpret_cast<unsigned>(p);}unsigned Word(unsigned p){return *reinterpret_cast<const unsigned*>(p);}
std::vector<unsigned> Ids(const NativeGameDatabase& db,const char* table){const auto& t=db.Tables().at(table);unsigned col{};while(t.fields.at(col).name!="ID")++col;std::vector<unsigned> result;for(const auto& row:t.rows)result.push_back(std::get<unsigned>(row[col]));return result;}
}
void VerifyNativeTradeWindowDetails(const std::filesystem::path& root,NativeCeguiUiSession& ui,const NativeGameDatabase& database,NativePkFileManagerRuntime& packages,NativePalGameIVTextureStorage& game,RpWorld* world,RwCamera* camera){
    NativeExceptionTrace trace;const auto module=GetModuleHandleA("CEGUIBase.dll"),look=GetModuleHandleA("OIRAMLOOK.dll");
    Check(ui.LoadOriginalMinimapLayout(root) && ui.SetMinimapLayoutVisible(false),"Trade details original startup fonts");
    NativeInventorySession inventory;inventory.BindDatabase(database);(void)inventory.Start();inventory.Reset();inventory.BindItemObserver([](auto){});(void)inventory.UpdateCounter(123456);
    NativePrescriptionRecordCatalog recipes(database);PlayerManagerObjectState players{};CollectionState collection{};NativeUiCollectionObjects objects(collection,world,root,packages,game);
    NativeRoleRecordCatalog roles(database);NativePlayerSlotCopyRuntime copier;
    for(unsigned n=0;n<4u;++n){const auto player=Address(&players.slots[n]);(void)PlayerManagerSlotDataCopier::Copy(player,roles.Address(n),copier);players.Word(n*(0xb14u/4u)+0xb08u/4u)=1u;
        (void)NativePlayerScriptPropertyRuntime::Set(player+0x654u,0x44u,4u,99u);}
    NativeBaseObjectMemory memory;NativeSkillRecordCatalog skill_records(database);NativePlayerScriptRefreshRuntime refresh(memory,inventory);
    for(auto& player:players.slots)memory.WriteDword(Address(&player)+0x654u,PlayerManagerSlot::kScriptVtable);
    NativePlayerSkillCallbacks callbacks(skill_records,refresh);RandomNumberGenerator random;random.Seed(123u);NativePlayerCombatEvents events;
    NativeSkillSystemOwner skill_system(random,events);Check(skill_system.Construct("H:/PAL4/objs")!=0u,"Trade comparison original SkillSystem");
    NativePlayerSkillRuntime skills(memory,skill_records,callbacks,skill_system);refresh.BindSkills(&skills);
    const NativeSkillRecordCatalog::Record* passive{};for(std::size_t n=0;n<skill_records.Size();++n){const auto& r=skill_records.At(n);if(r[2]==1u && r[3]==0xffffffffu && r[4]==9114u && !r[14]){passive=&r;break;}}
    Check(passive && skills.Apply(Address(&players.slots[0])+0x654u,(*passive)[0],Address(&players.slots[0])+0x654u),"Trade comparison actual permanent skill");
    InventoryItemSlotsState equipped{};
    for(auto id:Ids(database,"Equip")){const auto definition=inventory.EquipmentDefinitionAddress(id);const auto slot=InventoryItemSlots::Classify(equipped,definition,inventory);
        if(slot<6u && !equipped.ids[slot]){const auto item=inventory.ProcessEquipment(inventory.Address()+0x7cu,id);equipped.ids[slot]=Word(item+400u);}}
    std::memcpy(reinterpret_cast<void*>(Address(&players.slots[0])+0x920u),&equipped,sizeof(equipped));
    for(auto& player:players.slots)(void)refresh.Refresh(Address(&player)+0x654u);
    std::array<unsigned,0x224u/4u> manager{};std::unique_ptr<NativeTradeWindowDetails> details;std::unique_ptr<NativeTradeWindowPreview> preview;
    std::unique_ptr<NativeCeguiDefaultListItem> row;
    const auto reset=detail::Export<void (__thiscall*)(void*)>(module,"?resetList@MultiColumnList@CEGUI@@QAEXXZ");
    NativeTradeWindowLayout layout(ui,root,Address(manager.data()),[&](auto method,auto){Check(method==0x48d1d0u,"only actual hover callback in details fixture");return details?details->Hover():true;},[&]{if(details)details->Close();if(preview)preview->Close();reset(reinterpret_cast<void*>(layout.Word(0x28u)));row.reset();});
    preview=std::make_unique<NativeTradeWindowPreview>(layout,objects,packages,Address(manager.data()));
    NativeTradeWindowComparison comparison(layout,inventory,players,memory,refresh,skills);
    struct Request{std::optional<std::string> model,texture,path;};std::vector<Request> requests;bool real_models{},retire_from_model{};unsigned totals{},slots{},hidden{};
    NativeTradeDetailServices services;
    services.model=[&](auto model,auto texture,auto path){requests.push_back({model?std::optional<std::string>(*model):std::nullopt,texture?std::optional<std::string>(*texture):std::nullopt,path?std::optional<std::string>(*path):std::nullopt});if(real_models)preview->Update(model,texture,path);if(retire_from_model)details.reset();};
    services.player_slot=[&](unsigned slot,unsigned player,unsigned item){comparison.Update(slot,player,item);++slots;};
    services.hide_slot=[&](unsigned slot){comparison.Hide(slot);++hidden;};
    services.total=[&]{++totals;};
    auto missing=services;missing.model={};bool missing_rejected{};try{NativeTradeWindowDetails bad(layout,inventory,players,database,recipes,std::move(missing));}catch(const std::invalid_argument&){missing_rejected=true;}Check(missing_rejected,"Trade details refuses an unbound model service");
    details=std::make_unique<NativeTradeWindowDetails>(layout,inventory,players,database,recipes,services);
    const auto list=layout.Word(0x28u);layout.BindParent(true);detail::NativeString empty(module,"");detail::Export<void (__thiscall*)(void*,const void*,unsigned,float)>(module,"?addColumn@MultiColumnList@CEGUI@@QAEXABVString@2@IM@Z")(reinterpret_cast<void*>(list),empty.Data(),0u,0.9f);
    const auto stage=[&](unsigned group,unsigned key){reset(reinterpret_cast<void*>(list));row.reset();layout.Word(0xe8u)=group;layout.Word(0xf8u)=1u;(void)detail::Export<unsigned (__thiscall*)(void*,unsigned)>(module,"?addRow@MultiColumnList@CEGUI@@QAEII@Z")(reinterpret_cast<void*>(list),0u);row=std::make_unique<NativeCeguiDefaultListItem>(module,look,list,"product");row->SetDataKey(key);detail::Export<void (__thiscall*)(void*,void*,unsigned,unsigned)>(module,"?setItem@MultiColumnList@CEGUI@@QAEXPAVListboxItem@2@II@Z")(reinterpret_cast<void*>(list),reinterpret_cast<void*>(row->Address()),0u,0u);detail::Export<void (__thiscall*)(void*,int)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(reinterpret_cast<void*>(list),0);layout.RethrowEventError();Check(details->Hover(),"actual hover detail routing");};
    const auto text=[&](unsigned offset){const auto value=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ")(reinterpret_cast<void*>(layout.Word(offset)));return std::string(detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(value));};
    const auto visible=[&](unsigned offset){return detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z")(reinterpret_cast<void*>(layout.Word(offset)),false);};
    const auto converted=[&](unsigned p){const auto utf8=ConvertGameTextToUtf8(p?std::string_view(reinterpret_cast<const char*>(p)):std::string_view{});Check(bool(utf8),"original detail text encoding");return *utf8;};
    unsigned property_cases{},equipment_cases{},recipe_cases{},weapon{};
    const auto player_baseline=players;const auto allocation_baseline=memory.AllocationAddresses();
    for(auto id:Ids(database,"Property")){const auto definition=inventory.PropertyAddress(id);stage(1u,id);Check(text(0x2cu)==converted(Word(definition+36u)) && text(0x30u)==converted(Word(definition+52u)) && text(0x34u)=="123456","all original property hover text/description/balance in real controls");++property_cases;}
    for(auto id:Ids(database,"Equip")){const auto definition=inventory.EquipmentDefinitionAddress(id);stage(0u,id);Check(text(0x2cu)==converted(Word(definition+172u)) && text(0x34u)=="123456" && text(0x38u)==std::to_string(std::bit_cast<int>(Word(definition+376u))),"all original equipment detail name/balance/price in real controls");if(!weapon && Word(definition+148u)<=2u && Word(definition+220u) && *reinterpret_cast<const char*>(Word(definition+220u)))weapon=id;++equipment_cases;}
    Check(!std::memcmp(&players,&player_baseline,sizeof(players)) && memory.AllocationAddresses()==allocation_baseline,"all equipment comparisons preserve live players and skill allocation owners");
    // Independent oracle: equip a disposable player and run the existing actual
    // refresh path, then compare its complete derived stat prefix to the snapshot.
    NativePlayerSnapshot snapshots(memory,inventory,skills);unsigned snapshot_cases{};
    const auto source_player=Address(&players.slots[0]);
    for(auto id:Ids(database,"Equip")){
        const auto definition=inventory.EquipmentDefinitionAddress(id);
        const auto slot=InventoryItemSlots::Classify(equipped,definition,inventory);if(slot>=6u)continue;
        const auto instance=inventory.ProcessEquipment(inventory.Address()+0x7cu,id);Check(instance!=0u,"comparison oracle original equipment instance");
        auto disposable=players.slots[0];const auto disposable_address=Address(&disposable);
        memory.WriteDword(disposable_address+0x920u+slot*4u,Word(instance+400u));
        (void)refresh.Refresh(disposable_address+0x654u);
        std::vector<unsigned> selected{definition};for(unsigned n=0;n<6u;++n)if(n!=slot){const auto item=inventory.FindInventoryEntry(inventory.Address()+0x7cu,equipped.ids[n],7u);if(item)selected.push_back(item);}
        const auto result=snapshots.Build(source_player,selected);
        Check(!std::memcmp(result.data(),reinterpret_cast<const void*>(disposable_address+0x7a8u),57u*4u),"snapshot derived stats/passive equal actual equipment refresh on disposable player");++snapshot_cases;
        comparison.Update(layout.FrameAddress()+0x54u,0u,definition);
        if(Word(definition+0x168u)>0u && Word(definition+0x168u)<=99u)for(unsigned n=0;n<5u;++n){
            const auto difference=std::bit_cast<int>(result[4u+n])-std::bit_cast<int>(Word(source_player+0x7b8u+n*4u));
            const auto value=text(0x5cu+n*4u);const auto expected=(difference>0?"+":"")+std::to_string(difference);
            Check(visible(0x5cu+n*4u) && (difference?value.find(expected+"</colour>")!=std::string::npos:value.empty()),"actual comparison labels show independent refresh differences");}
        Check(!std::memcmp(&players,&player_baseline,sizeof(players)) && memory.AllocationAddresses()==allocation_baseline,"snapshot does not mutate source skills/players or leak cloned contexts");
    }
    std::array<unsigned,100> gated{};std::memcpy(gated.data(),reinterpret_cast<const void*>(inventory.EquipmentDefinitionAddress(weapon)),sizeof(gated));
    for(auto requirement:{0u,100u}){gated[0x168u/4u]=requirement;comparison.Update(layout.FrameAddress()+0x54u,0u,Address(gated.data()));
        Check(visible(0x54u) && !visible(0x5cu),"zero or insufficient requirement hides actual comparison labels");}
    comparison.Update(layout.FrameAddress()+0x54u,0u,0u);Check(text(0x5cu).empty(),"empty equipment clears actual comparison text");
    const auto saved_pair_end=memory.ReadDword(source_player+0x654u+0x2bcu);memory.WriteDword(source_player+0x654u+0x2bcu,1u);
    bool invalid_range{};try{(void)snapshots.Build(source_player,{});}catch(const std::invalid_argument&){invalid_range=true;}
    memory.WriteDword(source_player+0x654u+0x2bcu,saved_pair_end);
    Check(invalid_range && memory.AllocationAddresses()==allocation_baseline,"malformed second skill range releases first cloned owner");
    // Inactive players compact into the remaining original role panels.
    players.Word(0xb08u/4u)=0u;stage(0u,weapon);Check(hidden>0u,"inactive player invokes actual Hide on unused comparison panel");players.Word(0xb08u/4u)=1u;
    for(const auto& r:recipes.Records()){stage(2u,r[0]);Check(text(0x2cu)==converted(r[6]) && text(0x34u)=="123456" && text(0x38u)==std::to_string(std::bit_cast<int>(r[30])) && requests.back().model=="zza" && requests.back().texture==std::string("ZZA")+std::to_string(r[25]+1u),"all original recipe detail controls and model request mode");if(r[25]!=0u)Check(text(0x30u)==converted(r[10]),"recipe direct description");++recipe_cases;}
    const auto property_id=Ids(database,"Property").front();(void)inventory.AddItem(property_id,2);const auto property_entry=NativeInventoryPropertyReader(inventory).FindId(property_id);Check(bool(property_entry),"actual property sell entry");stage(1u,property_id);row->SetDataKey(property_entry->sequence);layout.Word(0xf8u)=0u;Check(details->Hover() && text(0x2cu)==converted(Word(property_entry->definition+36u)),"actual property sale hover resolves sequence to definition");
    const auto instance=inventory.ProcessEquipment(inventory.Address()+124u,weapon);Check(instance!=0u,"actual equipment sale instance");stage(0u,weapon);row->SetDataKey(Word(instance+400u));layout.Word(0xf8u)=0u;Check(details->Hover() && text(0x38u)==std::to_string(std::bit_cast<int>(Word(instance+376u))/4),"actual equipment sale detail reads instance and quarter price");
    layout.Word(0xe8u)=1u;details->Clear();Check(text(0x2cu).empty() && text(0x30u).empty() && text(0x34u)=="123456" && !requests.back().model,"null property clears original controls/model request");
    real_models=true;stage(2u,recipes.Records().front()[0]);Check(layout.Word(0x11cu) && objects.Size()==1u,"recipe details load actual original model");const auto book=layout.Word(0x11cu);preview->Update("zza","ZZA2","PALObject\\");Check(layout.Word(0x11cu)==book && objects.Size()==1u,"same-name model reuses actual component for texture change");
    preview->Update("ZZA","ZZA3","palobject\\");Check(layout.Word(0x11cu)==book && objects.Size()==1u,"case-insensitive model reuse preserves one actual component");
    Check(weapon!=0u,"original weapon preview record");stage(0u,weapon);Check(layout.Word(0x11cu)!=book && objects.Size()==1u,"weapon detail replaces original book component through weapon package");
    auto* object=objects.Find(layout.Word(0x11cu));Check(object && object->Clump(),"Trade preview has actual RW clump");
    for(unsigned axis=0;axis<3u;++axis){const auto configured=object->State().Read(0xc4u+axis*4u);
        Check(std::bit_cast<float>(configured)>0.f && object->State().Read(0xb8u+axis*4u)==configured,
              "Trade weapon applies the actual configured scale at byte offset C4");}
    // Resource word56 is the transformed sphere at byte E0 (IDA667510).
    const auto& sphere=*reinterpret_cast<const RwSphere*>(object->State().Data()+56u);const float radius=std::max(1.f,sphere.radius);const RwMatrix saved=*RwFrameGetMatrix(RwCameraGetFrame(camera));const auto near_plane=RwCameraGetNearClipPlane(camera),far_plane=RwCameraGetFarClipPlane(camera);
    struct Restore{RwCamera* camera;RwMatrix matrix;float near_plane,far_plane;~Restore(){RwFrameTransform(RwCameraGetFrame(camera),&matrix,rwCOMBINEREPLACE);RwFrameUpdateObjects(RwCameraGetFrame(camera));RwCameraSetNearClipPlane(camera,near_plane);RwCameraSetFarClipPlane(camera,far_plane);}} restore{camera,saved,near_plane,far_plane};
    const RwV3d eye{sphere.center.x,sphere.center.y,sphere.center.z-radius*3.f};RwFrameSetIdentity(RwCameraGetFrame(camera));RwFrameTranslate(RwCameraGetFrame(camera),&eye,rwCOMBINEREPLACE);RwFrameUpdateObjects(RwCameraGetFrame(camera));RwCameraSetNearClipPlane(camera,radius*0.01f);RwCameraSetFarClipPlane(camera,radius*10.f);RwRGBA black{0,0,0,255};Check(RwCameraClear(camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera),"Trade preview GPU begin");Check(object->Render(Address(camera)),"Trade weapon reaches actual native render");(void)RwCameraEndUpdate(camera);
    auto* raster=RwCameraGetRaster(camera);auto* image=RwImageCreate(RwRasterGetWidth(raster),RwRasterGetHeight(raster),32);Check(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image,raster),"Trade preview GPU readback");unsigned lit{};for(int y=0;y<RwImageGetHeight(image);++y){const auto* pixels=reinterpret_cast<const RwRGBA*>(RwImageGetPixels(image)+y*RwImageGetStride(image));for(int x=0;x<RwImageGetWidth(image);++x)lit+=(pixels[x].red || pixels[x].green || pixels[x].blue)?1u:0u;}RwImageDestroy(image);Check(lit>0u,"actual Trade weapon preview produces nonblack GPU pixels");
    layout.Word(0xe8u)=0u;details->Clear();Check(!layout.Word(0x11cu) && !objects.Size(),"empty equipment detail removes actual model");
    layout.Word(0xe8u)=1u;retire_from_model=true;details->Clear();retire_from_model=false;Check(!details && text(0x2cu).empty(),"detail operation retains its runtime when a model callback retires its wrapper");
    details=std::make_unique<NativeTradeWindowDetails>(layout,inventory,players,database,recipes,services);comparison.Close();layout.Close();inventory.Close();objects.Clear();
    bool retired_comparison{};try{comparison.Hide(layout.FrameAddress()+0x54u);}catch(const std::logic_error&){retired_comparison=true;}Check(retired_comparison,"retired comparison refuses UI access");
    Check(!inventory.OutstandingAllocations() && !objects.Size() && !objects.OutstandingQueueAllocations() && hidden>0u && totals>0u,"Trade detail/model ownership balances");
    bool closed_details{},closed_model{};try{(void)details->Hover();}catch(const std::logic_error&){closed_details=true;}try{preview->Update("zza","ZZA1","PALObject\\");}catch(const std::logic_error&){closed_model=true;}Check(closed_details && closed_model,"retired detail and model owners reject further operations");
    std::cout<<"PASS: native Trade details "<<equipment_cases<<" equipment,"<<property_cases<<" properties,"<<recipe_cases<<" recipes, actual controls/hover/description/price/balance; real book reuse/texture and weapon replace/GPU pixels="<<lit<<"; actual player comparison, "<<snapshot_cases<<" independent equipment refresh oracles including permanent skill; total service explicit\n";
}
}
