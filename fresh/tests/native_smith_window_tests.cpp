#include "pal4/native_smith_window_layout.h"
#include "pal4/native_smith_window_lists.h"
#include "pal4/native_smith_window_confirmation.h"
#include "pal4/native_smith_window_craft_actions.h"
#include "pal4/native_crafting_application.h"
#include "pal4/native_crafting_strengthening.h"
#include "pal4/native_crafting_enchantment.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/native_common_dialog_layout.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "pal4/native_inventory_prescriptions.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_player_inventory_state_runtime.h"
#include "pal4/native_inventory_equipment_mutation.h"
#include "pal4/native_crafting_requirements.h"
#include "pal4/player_manager_object.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_game_database.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_text_conversion.h"
#include "pal4/native_cegui_widget_images.h"
#include "pal4/native_package_archive.h"
#include "support/native_cegui_frame_session.h"
#include "support/native_cegui_test_click.h"
#include "support/native_smith_trade_checks.h"
#include "../native/ui/cegui_exports_native.h"
#include <rtfsyst.h>
#include <dbghelp.h>
#pragma comment(lib,"dbghelp.lib")
#include <algorithm>
#include <iostream>
#include <set>
#include <fstream>
namespace {
using pal4::detail::Export;void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}void* Ptr(unsigned a){return reinterpret_cast<void*>(a);}
}
int main(int argc,char** argv)try{
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* e)->LONG{HMODULE m{};char path[MAX_PATH]{};const auto ip=e->ExceptionRecord->ExceptionAddress;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<const char*>(ip),&m);GetModuleFileNameA(m,path,MAX_PATH);
        std::cerr<<"Smith exception code="<<std::hex<<e->ExceptionRecord->ExceptionCode<<" ip="<<ip<<" module="<<path<<" offset="<<(reinterpret_cast<unsigned>(ip)-reinterpret_cast<unsigned>(m))<<'\n';
        const auto base=GetModuleHandleA("CEGUIBase.dll");const auto object=reinterpret_cast<void*>(e->ContextRecord->Esi);
        const auto name=Export<const void* (__thiscall*)(const void*)>(base,"?getName@Window@CEGUI@@QBEABVString@2@XZ")(object);
        std::cerr<<"crashing window "<<Export<const char* (__thiscall*)(const void*)>(base,"?c_str@String@CEGUI@@QBEPBDXZ")(name)<<" image="<<*reinterpret_cast<const unsigned*>(e->ContextRecord->Esi+0x83cu)<<'\n';
        SymSetOptions(SYMOPT_UNDNAME|SYMOPT_LOAD_LINES);SymInitialize(GetCurrentProcess(),nullptr,TRUE);CONTEXT context=*e->ContextRecord;STACKFRAME64 frame{};
        frame.AddrPC={context.Eip,0,AddrModeFlat};frame.AddrFrame={context.Ebp,0,AddrModeFlat};frame.AddrStack={context.Esp,0,AddrModeFlat};
        for(unsigned i=0;i<16u && StackWalk64(IMAGE_FILE_MACHINE_I386,GetCurrentProcess(),GetCurrentThread(),&frame,&context,nullptr,SymFunctionTableAccess64,SymGetModuleBase64,nullptr);++i){
            alignas(SYMBOL_INFO) char storage[sizeof(SYMBOL_INFO)+512]{};auto* symbol=reinterpret_cast<SYMBOL_INFO*>(storage);symbol->SizeOfStruct=sizeof(SYMBOL_INFO);symbol->MaxNameLen=511;
            DWORD64 displacement{};std::cerr<<"frame "<<frame.AddrPC.Offset;if(SymFromAddr(GetCurrentProcess(),frame.AddrPC.Offset,&displacement,symbol))std::cerr<<' '<<symbol->Name<<'+'<<displacement;std::cerr<<'\n';}
        return EXCEPTION_EXECUTE_HANDLER;});
    Check(argc==2,"original root required");const std::filesystem::path root=argv[1];
    pal4::test::NativeFrameSession frame;Check(frame.Start(800,600) && !IsWindowVisible(frame.window),"hidden800x600 secondary-screen host");
    struct Files{bool ready=RtFSManagerOpen(8)!=FALSE;~Files(){if(ready)RtFSManagerClose();}} files;Check(files.ready,"filesystem");
    pal4::NativeCeguiUiSession ui;Check(ui.Start(root,800,600),"original UI start");
    Check(ui.LoadImageset(root,"gamedata\\ui\\fonts\\fontPuzzle_0.imageset") && ui.InitializeWidgetRendering(root) && ui.LoadImageset(root,"gamedata\\ui\\imagesets\\OiramLook.imageset") && ui.LoadOriginalMainLayout() && ui.HideMainLayout(),"original fonts/widgets/desktop");
    pal4::NativeGameDatabase db;Check(db.Load(root),"original database");pal4::NativePrescriptionRecordCatalog recipes(db);
    pal4::NativeInventorySession inventory;inventory.BindDatabase(db);(void)inventory.Start();inventory.Reset();inventory.BindItemObserver([](unsigned){});
    pal4::NativeBaseObjectMemory player_memory;pal4::PlayerManagerObjectState players{};
    pal4::NativePlayerInventoryStateRuntime conditions(players,player_memory,inventory);
    pal4::NativeInventoryEquipmentMutation mutation(inventory);
    pal4::NativePlayerScriptRefreshRuntime player_refresh(player_memory,inventory);unsigned mutation_sounds{};
    auto mutation_sound=[&](std::string_view,unsigned){++mutation_sounds;};
    pal4::NativeCraftingApplication foundry(recipes,inventory,players,player_memory,player_refresh,mutation_sound);
    pal4::NativeCraftingStrengthening strengthen(recipes,inventory,mutation_sound,[](){return 17u;});
    pal4::NativeCraftingEnchantment enchant(recipes,inventory,mutation_sound);
    pal4::NativeInventoryPrescriptions learned(inventory,recipes);
    {pal4::NativePackageArchive archive;Check(archive.Open((root/"gamedata/ui.cpk").string().c_str()),"open original UI diagnostics archive");
        const auto xml=archive.ReadMember("layouts\\SmithWindow.xml");Check(xml.has_value(),"original Smith XML diagnostic member");
        std::ofstream out("H:/PAL4/objs/M09-smith-ui-20261020-original-layout.xml",std::ios::binary);out.write(reinterpret_cast<const char*>(xml->data()),xml->size());}
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    pal4::NativeCommonDialogLayout common_dialog(module,ui,root);
    Check(pal4::NativeCeguiWidgetImages(module).Image("OiramLook","VertScrollThumbNormal")!=0u,"original default scrollbar image prerequisite");
    auto rows=[&](unsigned list){return Export<unsigned (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(Ptr(list));};
    auto columns=[&](unsigned list){return Export<unsigned (__thiscall*)(const void*)>(module,"?getColumnCount@MultiColumnList@CEGUI@@QBEIXZ")(Ptr(list));};
    auto cell=[&](unsigned list,unsigned row,unsigned column){const std::array<unsigned,2> ref{row,column};return reinterpret_cast<unsigned>(Export<void* (__thiscall*)(const void*,const void*)>(module,"?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z")(Ptr(list),ref.data()));};
    auto id=[&](unsigned item){return Export<unsigned (__thiscall*)(const void*)>(module,"?getID@ListboxItem@CEGUI@@QBEIXZ")(Ptr(item));};
    auto item_text=[&](unsigned item){const auto s=Export<const void* (__thiscall*)(const void*)>(module,"?getText@ListboxItem@CEGUI@@QBEABVString@2@XZ")(Ptr(item));return std::string(Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(s));};
    auto text=[&](unsigned w){const auto value=Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ")(Ptr(w));return std::string(Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(value));};
    unsigned reset_calls{},events{};
    for(unsigned pass=0;pass<3u;++pass){if(pass==1u)for(const auto& recipe:recipes.Records())(void)learned.Add(recipe[0]);
        std::unique_ptr<pal4::NativeSmithWindowLists> view;std::unique_ptr<pal4::NativeSmithWindowConfirmation> confirmation;
        std::unique_ptr<pal4::NativeSmithWindowCraftActions> actions;bool use_actions{};
        pal4::NativeSmithWindowLayout* current{};bool fail{},retire{};
        pal4::NativeEventHandlerArray activity([&](unsigned owner,unsigned slot){Check(confirmation && confirmation->OwnsActivity(owner),"real selection activity routes to confirmation owner");return confirmation->Callback(owner,slot);},
            [](unsigned,unsigned,unsigned)->std::uint8_t{throw std::logic_error("selection keyboard dispatch is not part of this fixture");});
        std::vector<unsigned> methods;
        pal4::NativeSmithWindowLayout layout(ui,root,[&](unsigned owner,unsigned method,unsigned){
            if(fail)throw std::runtime_error("Smith callback sentinel");if(retire){current->Close();return true;}
            ++events;methods.push_back(method);if(use_actions && actions && actions->FromDll(method))return true;
            return view?(view->FromDllHover(method) || view->FromDllCategory(method)):false;
        },[&](pal4::NativeSmithWindowLayout& page){++reset_calls;view=std::make_unique<pal4::NativeSmithWindowLists>(page,inventory,recipes,db,conditions);view->Reset();},
            [&](){if(actions){actions->Close();actions.reset();}if(confirmation){confirmation->Close();confirmation.reset();}if(view){view->Close();view.reset();}});current=&layout;
        bool smith_active{};unsigned sounds{};std::vector<std::pair<pal4::SmithWindowTradeCallback,unsigned>> choices;
        confirmation=std::make_unique<pal4::NativeSmithWindowConfirmation>(layout,*view,inventory,recipes,db,common_dialog,activity,activity.Address()-0x134u,
            pal4::NativeSmithConfirmationServices{[&](){return smith_active;},[&](std::string_view,unsigned){++sounds;},[&](pal4::SmithWindowTradeCallback kind,unsigned choice){choices.emplace_back(kind,choice);if(use_actions){Check(actions!=nullptr,"craft action owner bound");(void)actions->Dispatch(kind,choice);}}});
        actions=std::make_unique<pal4::NativeSmithWindowCraftActions>(layout,*view,*confirmation,inventory,recipes,db,foundry,strengthen,enchant,[&](){return smith_active;});
        const auto object=layout.FrameAddress(),list=layout.Word(0x44u);std::set<unsigned> controls;
        for(const auto offset:{0x60u,0x64u,0x68u,0x40u,0x48u,0x78u,0x4cu,0x58u,0x5cu,0x44u,0x50u,0x54u,0x6cu,0x70u,0x74u,0x7cu,0x90u,0x94u,0x98u,0x84u,0x88u,0x8cu})Check(layout.Word(offset) && controls.insert(layout.Word(offset)).second,"22 distinct original Smith controls");
        Check(layout.Word(0)==0x843dc4u && layout.Word(0x28u)==0x843d9cu && layout.Word(0xecu)==object && layout.Word(0xf4u)==object,"shared constructor vtable IDs and self links");
        Check(layout.Word(0x34u)-layout.Word(0x30u)==12u && columns(layout.Word(0x40u))==2u && columns(list)==3u && rows(layout.Word(0x40u))==0u,"three-tab navigation and real initial reset columns");
        if(!pass)Check(!rows(list) && !view->OwnedItemCount(),"normal empty acquired list is not auto-unlocked");
        if(!pass){view->LoadEquipment(0u);Check(!rows(layout.Word(0x40u)),"equipment list gated by absent recipe hover");}
        if(!pass){const auto before=common_dialog.SelectionGeneration();Check(confirmation->Confirm() && common_dialog.SelectionGeneration()==before && !activity.Size(),"no recipe returns without dialog or activity");}
        if(pass){
            for(unsigned mode=0;mode<3;++mode){view->Load(mode);layout.RethrowEventError();const auto count=learned.Count(mode);const auto cols=mode?2u:3u;
                Check(rows(list)==static_cast<unsigned>(count) && columns(list)==cols && view->OwnedItemCount()==static_cast<unsigned>(count)*cols,"actual learned rows and all owned DLL cells per mode");
                for(unsigned row=0;row<static_cast<unsigned>(count);++row){Check(id(cell(list,row,0))==learned.At(mode,row),"every real recipe name cell ID matches acquired sorted ID");
                    const auto recipe=recipes.Address(learned.At(mode,row));Check(item_text(cell(list,row,0))==*pal4::ConvertGameTextToUtf8(recipes.Text(inventory.ReadDword(recipe+8u))),"every real recipe name reaches DLL cell as correct UTF8");
                    if(mode)Check(item_text(cell(list,row,1))==std::to_string(static_cast<int>(inventory.ReadDword(recipe+(mode==1u?0x6cu:0x70u)))),"original potential/magic requirement text for every real recipe");
                    for(unsigned col=0;col<cols;++col)Check(*reinterpret_cast<const unsigned char*>(cell(list,row,col)+0x1a5u)==1u,"empty inventory disables every prescription cell");}
                const auto record=recipes.Address(learned.At(mode,0));Check(text(layout.Word(0x50u))==*pal4::ConvertGameTextToUtf8(recipes.Text(inventory.ReadDword(record+0x18u))),"hover shows actual original recipe description");
                const auto key=learned.At(mode,0);for(unsigned n=0;n<4;++n){const auto amount=inventory.ReadDword(record+0xa8u+n*4u),item=inventory.ReadDword(record+0x98u+n*4u);
                    if(static_cast<int>(amount)>0){if(inventory.PropertyAddress(item))(void)inventory.AddItem(item,static_cast<int>(amount));else for(unsigned i=0;i<amount;++i)(void)inventory.ProcessEquipment(inventory.Address()+0x7cu,item);}}
                view->RefreshAvailability();for(unsigned col=0;col<cols;++col)Check(!*reinterpret_cast<const unsigned char*>(cell(list,0,col)+0x1a5u),"actual materials enable complete original recipe row");
                Check(id(cell(list,0,0))==key,"availability preserves row identity");
                inventory.Reset();for(const auto& recipe:recipes.Records())(void)learned.Add(recipe[0]);
            }
        }
        if(pass){
            std::array<unsigned,7> definitions{};
            for(const auto& row:db.Tables().at("Equip").rows){const auto key=std::get<unsigned>(row[0]);const auto entry=inventory.EquipmentDefinitionAddress(key);
                if(entry){const auto type=inventory.ReadDword(entry+0x94u);if(type<7u && !definitions[type])definitions[type]=key;}}
            std::array<std::array<unsigned,2>,7> sequences{};
            players.Fill(0u);
            for(unsigned category=0;category<7u;++category){Check(definitions[category]!=0u,"original equipment definition for each category");
                for(unsigned copy=0;copy<2u;++copy){const auto entry=inventory.ProcessEquipment(inventory.Address()+0x7cu,definitions[category]);
                    sequences[category][copy]=inventory.ReadDword(entry+0x190u);}
                const auto owner=category%4u;const auto player=reinterpret_cast<unsigned>(&players.slots[owner]);
                player_memory.WriteDword(player+0xb08u,1u);player_memory.WriteDword(player+0x920u+(category/4u)*4u,sequences[category][0]);
            }
            const auto equipment_list=layout.Word(0x40u);
            for(unsigned mode=0;mode<3u;++mode){view->Load(mode);const auto recipe=recipes.Address(learned.At(mode,0));
                const auto required=static_cast<int>(inventory.ReadDword(recipe+0x70u));
                for(unsigned category=0;category<7u;++category){
                    for(unsigned copy=0;copy<2u;++copy){mutation.WriteScalar(sequences[category][copy],0x184u,copy?0u:3u);
                        // Explicit boundary fixture for original instance+17C;
                        // this metadata is intentionally outside public mutation.
                        const auto entry=inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequences[category][copy],7u);
                        player_memory.WriteDword(entry+0x17cu,static_cast<unsigned>(copy?required-1:required));}
                    view->LoadEquipment(category);layout.RethrowEventError();
                    Check(rows(equipment_list)==2u && columns(equipment_list)==2u,"two independent equipment instances in all seven native category lists");
                    for(unsigned row=0;row<2u;++row){const auto sequence=sequences[category][row];const auto item=cell(equipment_list,row,0);
                        const auto entry=inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequence,7u);
                        Check(id(item)==sequence,"equipment DLL ID is instance sequence, not definition ID");
                        Check(item_text(item)==*pal4::ConvertGameTextToUtf8(reinterpret_cast<const char*>(inventory.ReadDword(entry+0x9cu))),"actual equipment instance name reaches native cell");
                        Check((*reinterpret_cast<const unsigned*>(item+0x210u)!=0u)==(row==0u),"existing game-derived item draws icon only for active worn instance");
                        for(unsigned col=0;col<2u;++col)Check((*reinterpret_cast<const unsigned char*>(cell(equipment_list,row,col)+0x1a5u)!=0u)==(mode!=0u && row==1u),"potential and lingyun gates apply to whole native row");
                    }
                    Check(!text(layout.Word(0x48u)).empty(),"equipment hover displays real instance description");
                    if(mode==1u){for(const auto value:{0xffffffffu,0u,1u}){
                        mutation.WriteScalar(sequences[category][0],0x184u,value);view->RefreshAvailability();
                        for(unsigned col=0;col<2u;++col)Check((*reinterpret_cast<const unsigned char*>(cell(equipment_list,0,col)+0x1a5u)!=0u)==(value!=1u),"signed potential negative/zero/positive boundary");}}
                }
            }
            if(pass==1u){
                // Capture the actual derived worn-icon draw and both panels;
                // only the fixture exposes panels without the effect controller.
                for(const auto offset:{0x84u,0x88u,0x8cu,0x90u,0x94u,0x98u})
                    Export<void (__thiscall*)(void*,bool)>(module,"?setVisible@Window@CEGUI@@QAEX_N@Z")(Ptr(layout.Word(offset)),true);
                Check(ui.BindDialogRootParent(layout.RootAddress(),true) && frame.Begin() && ui.Render(),"equipment derived item actual GPU draw");
                const auto image=frame.ReadPixels();std::ofstream out("H:/PAL4/objs/M09-smith-equipment-20261020-frame.rgba",std::ios::binary);
                out.write(reinterpret_cast<const char*>(image.data()),image.size());
            }
            players.Fill(0u);view->LoadEquipment(6u);
            Check(!*reinterpret_cast<const unsigned*>(cell(equipment_list,0,0)+0x210u),"inactive party no longer marks former worn instance");
            const std::array<unsigned,2> categories{0u,6u};layout.Word(0xccu)=reinterpret_cast<unsigned>(categories.data());layout.Word(0xd0u)=layout.Word(0xccu)+8u;
            view->RefreshAvailability();auto visible=[&](unsigned w){return Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z")(Ptr(w),true);};
            Check(visible(layout.Word(0x58u)) && visible(layout.Word(0x5cu)),"two available categories show both page arrows");
            layout.Word(0xd0u)=layout.Word(0xccu)+4u;view->RefreshAvailability();Check(!visible(layout.Word(0x58u)) && !visible(layout.Word(0x5cu)),"one category hides both page arrows");
            layout.Word(0xccu)=layout.Word(0xd0u)=0u;
            // All original category masks intersect actual inventory, preserving
            // order; the fixture enables the controller's selection gate only
            // after testing that its cleared state blocks navigation.
            unsigned category_recipes{},multi_categories{};
            for(unsigned mode=1u;mode<3u;++mode){view->Load(mode);
                for(unsigned row=0;row<static_cast<unsigned>(learned.Count(mode));++row){
                    Export<void (__thiscall*)(void*,int)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(Ptr(list),static_cast<int>(row));
                    const auto recipe=recipes.Address(learned.At(mode,row));std::vector<unsigned> expected;
                    for(unsigned c=0;c<7u;++c)if(static_cast<int>(inventory.ReadDword(recipe+0x7cu+c*4u))>0)expected.push_back(c);
                    view->SelectHoveredPrescription();layout.RethrowEventError();const auto begin=layout.Word(0xccu);
                    Check((layout.Word(0xd0u)-begin)/4u==expected.size(),"all original recipe masks intersect native inventory");
                    for(unsigned c=0;c<expected.size();++c)Check(inventory.ReadDword(begin+c*4u)==expected[c],"category order follows original seven mask fields");
                    if(expected.empty()){Check(layout.Word(0xb0u)==0xffffffffu,"no available category retains minus-one index");continue;}
                    Check(layout.Word(0xb0u)==0u && id(cell(equipment_list,0,0))==sequences[expected[0]][0],"selection loads first real available category");
                    *reinterpret_cast<unsigned char*>(object+0xacu)=0u;Check(view->FromDllCategory(0x49dd80u) && !layout.Word(0xb0u),"cleared selection gate prevents next navigation");
                    *reinterpret_cast<unsigned char*>(object+0xacu)=1u;
                    for(unsigned i=0;i<expected.size()+1u;++i){Check(view->FromDllCategory(0x49dd80u),"shared next handler bound");const auto index=(i+1u)%expected.size();
                        Check(layout.Word(0xb0u)==index && id(cell(equipment_list,0,0))==sequences[expected[index]][0],"next category wraps and repopulates actual instances");}
                    const auto before=layout.Word(0xb0u);Check(view->FromDllCategory(0x49dd00u),"shared previous handler bound");
                    Check(layout.Word(0xb0u)==(before+expected.size()-1u)%expected.size(),"previous category wraps");
                    if(expected.size()>1u && !multi_categories){for(const auto offset:{0x90u,0x94u,0x98u})Export<void (__thiscall*)(void*,bool)>(module,"?setVisible@Window@CEGUI@@QAEX_N@Z")(Ptr(layout.Word(offset)),true);
                        Check(ui.BindDialogRootParent(layout.RootAddress(),true),"category click fixture root");const auto old=layout.Word(0xb0u);
                        pal4::test::ClickNativeCeguiWindow(ui,layout.Word(0x5cu));layout.RethrowEventError();Check(layout.Word(0xb0u)==(old+1u)%expected.size(),"real DLL next-arrow click changes category");++multi_categories;}
                    ++category_recipes;
                }
            }
            Check(category_recipes==96u && multi_categories==1u,"all original strengthening/enchantment recipe category masks");
            // Foundry selects matching material instances, excluding other
            // definitions in the same category. Its arrow path refreshes after
            // removal rather than cycling category masks.
            pal4::NativeCraftingRequirements requirements(recipes,inventory);unsigned foundry_recipes{};
            view->Reset();inventory.Reset();for(const auto& r:recipes.Records())(void)learned.Add(r[0]);
            for(unsigned row=0;row<static_cast<unsigned>(learned.Count(0));++row){const auto key=learned.At(0,row);if(!requirements.HasEquipmentMaterial(key))continue;
                view->Load(0u);const auto definition=static_cast<unsigned>(requirements.FirstEquipmentValue(key));const auto type=inventory.ReadDword(inventory.EquipmentDefinitionAddress(definition)+0x94u);
                std::array<unsigned,2> matching{};for(auto& sequence:matching){const auto entry=inventory.ProcessEquipment(inventory.Address()+0x7cu,definition);sequence=inventory.ReadDword(entry+0x190u);}
                unsigned distractor{};for(const auto& r:db.Tables().at("Equip").rows){const auto id=std::get<unsigned>(r[0]);const auto entry=inventory.EquipmentDefinitionAddress(id);
                    if(id!=definition && entry && inventory.ReadDword(entry+0x94u)==type){distractor=inventory.ProcessEquipment(inventory.Address()+0x7cu,id);break;}}
                Check(distractor!=0u,"same-category different-definition distractor");
                Export<void (__thiscall*)(void*,int)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(Ptr(list),static_cast<int>(row));view->SelectHoveredPrescription();layout.RethrowEventError();
                Check(rows(equipment_list)==2u && (layout.Word(0xc0u)-layout.Word(0xbcu))==8u,"foundry collects only two matching material sequences");
                Check(id(cell(equipment_list,0,0))==matching[0] && id(cell(equipment_list,1,0))==matching[1],"foundry collection preserves instance order");
                Check(inventory.RemoveEquipment(matching[0]),"remove selected foundry instance fixture");*reinterpret_cast<unsigned char*>(object+0xacu)=1u;
                Check(view->FromDllCategory(0x49dd80u),"foundry arrow runs actual inventory refresh");layout.RethrowEventError();
                Check(rows(equipment_list)==1u && id(cell(equipment_list,0,0))==matching[1],"removed sequence disappears while other copy remains");
                view->Reset();inventory.Reset();for(const auto& r:recipes.Records())(void)learned.Add(r[0]);++foundry_recipes;
            }
            Check(foundry_recipes==5u,"all five original foundry equipment-material recipes");
            view->Reset();inventory.Reset();players.Fill(0u);for(const auto& recipe:recipes.Records())(void)learned.Add(recipe[0]);view->Load(2u);
        }
        if(pass){
            view->Load(0u);const auto before=inventory.OutstandingAllocations();
            (void)inventory.UpdateCounter(79);Check(confirmation->Confirm(),"original insufficient-money confirmation");
            Check(text(common_dialog.Window("Message"))==*pal4::ConvertGameTextToUtf8(*db.FindTableTextById("GameString",0x2du)),"exact original money gate message");
            Check(!activity.Size() && !*reinterpret_cast<const unsigned char*>(object+0xacu) && inventory.OutstandingAllocations()==before,"money rejection neither opens selection nor mutates inventory");
            (void)inventory.UpdateCounter(1);Check(confirmation->Confirm(),"money boundary80 proceeds to real material check");
            Check(text(common_dialog.Window("Message")).find("%s")==std::string::npos && !activity.Size(),"original missing-material formatting replaces placeholder");
            {pal4::NativeCraftingRequirements query(recipes,inventory);const auto missing=query.Deficits(learned.At(0,0));Check(!missing.empty(),"original material deficit fixture");
                const auto rendered=text(common_dialog.Window("Message"));for(const auto& deficit:missing){const auto property=inventory.PropertyAddress(deficit.item);
                    const auto field=property?property+0x14u:inventory.EquipmentDefinitionAddress(deficit.item)+0x9cu;
                    const auto name=reinterpret_cast<const char*>(inventory.ReadDword(field));
                    Check(rendered.find(*pal4::ConvertGameTextToUtf8(name))!=std::string::npos && rendered.find(std::string("\xc3\x97")+std::to_string(deficit.count))!=std::string::npos,"every missing material name and actual deficit quantity displayed");}}
            common_dialog.Close();
            pal4::NativeCraftingRequirements requirements(recipes,inventory);
            auto fill=[&](unsigned record){for(unsigned n=0;n<4u;++n){const auto amount=inventory.ReadDword(record+0xa8u+n*4u),item=inventory.ReadDword(record+0x98u+n*4u);
                if(static_cast<int>(amount)>0){if(inventory.PropertyAddress(item))(void)inventory.AddItem(item,static_cast<int>(amount));else for(unsigned i=0;i<amount;++i)(void)inventory.ProcessEquipment(inventory.Address()+0x7cu,item);}}};
            // Ordinary foundry reaches real yes/no dialog; callback is copied
            // and only dispatched through the mandatory gameplay service.
            unsigned plain{};for(unsigned r=0;r<static_cast<unsigned>(learned.Count(0));++r)if(!requirements.HasEquipmentMaterial(learned.At(0,r))){plain=r;break;}
            Export<void (__thiscall*)(void*,int)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(Ptr(list),static_cast<int>(plain));
            const auto recipe=recipes.Address(learned.At(0,plain));fill(recipe);Check(confirmation->Confirm(),"direct foundry confirmation dialog");
            Check(text(common_dialog.Window("Message"))==*pal4::ConvertGameTextToUtf8(*db.FindTableTextById("GameString",0x5bu)),"normal foundry fee prompt");
            pal4::test::ClickNativeCeguiWindow(ui,common_dialog.Window("BtNo"));common_dialog.RethrowEventError();
            Check(choices.size()==1u && choices.back()==std::pair{pal4::SmithWindowTradeCallback::SelectionConfirm,2u},"real DLL No keeps original callback choice2");
            smith_active=true;Check(confirmation->Confirm(),"system foundry no-money prompt with product name");
            Check(text(common_dialog.Window("Message")).find("%s")==std::string::npos,"system product name formatted before DLL display");
            pal4::test::ClickNativeCeguiWindow(ui,common_dialog.Window("BtYes"));common_dialog.RethrowEventError();
            Check(choices.size()==2u && choices.back()==std::pair{pal4::SmithWindowTradeCallback::SelectionConfirm,1u},"real DLL Yes keeps original callback choice1");
            Check(!activity.Size() && !*reinterpret_cast<const unsigned char*>(object+0x81u),"direct foundry does not expose equipment selection");
            for(unsigned mode=0u;mode<3u;++mode){unsigned row{};if(!mode){while(row<static_cast<unsigned>(learned.Count(0)) && !requirements.HasEquipmentMaterial(learned.At(0,row)))++row;}
                view->Load(mode);const auto selected=recipes.Address(learned.At(mode,row));fill(selected);
                Export<void (__thiscall*)(void*,int)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(Ptr(list),static_cast<int>(row));
                Check(confirmation->Confirm(),"genuine confirmation enters equipment selection");layout.RethrowEventError();
                Check(*reinterpret_cast<const unsigned char*>(object+0x81u) && *reinterpret_cast<const unsigned char*>(object+0xacu) && activity.GetLast()==object+0xf0u,"real panel state, selection gate and activity record");
                const auto count=activity.Size();Check(confirmation->Confirm() && activity.Size()==count,"shown selection panel does not duplicate activity");
                confirmation->HideSelection();Check(!*reinterpret_cast<const unsigned char*>(object+0x81u),"shared hide clears selection flag");
                Check(activity.GetLast()==object+0xf0u,"original hide retains selection activity until owner retirement");
                (void)activity.Remove(object+0xf0u);
            }
            Check(sounds>0u,"CommonDialog used explicit silent sound policy");
            {
                auto reset=[&](unsigned mode,unsigned row){confirmation->HideSelection();(void)activity.Remove(object+0xe8u);(void)activity.Remove(object+0xf0u);view->Reset();inventory.Reset();players.Fill(0u);
                    for(const auto& r:recipes.Records())(void)learned.Add(r[0]);(void)inventory.UpdateCounter(1000);view->Load(mode);
                    Export<void (__thiscall*)(void*,int)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(Ptr(list),static_cast<int>(row));};
                auto dismiss=[&](){pal4::test::ClickNativeCeguiWindow(ui,common_dialog.Window("BtOk"));common_dialog.RethrowEventError();
                    Check(activity.GetLast()==object+0xe8u,"result dialog original close callback restores effect activity");(void)activity.Remove(object+0xe8u);};
                smith_active=false;use_actions=true;reset(0u,plain);fill(recipe);const auto initial_materials=inventory.OutstandingAllocations();const auto initial_sounds=mutation_sounds;
                Check(confirmation->Confirm(),"full native foundry starts with confirmation");pal4::test::ClickNativeCeguiWindow(ui,common_dialog.Window("BtNo"));common_dialog.RethrowEventError();
                Check(inventory.ReadDword(inventory.Address()+0x134u)==1000u && inventory.RecentEquipmentSequences().empty() && mutation_sounds==initial_sounds && inventory.OutstandingAllocations()==initial_materials,"No callback leaves actual materials/money/equipment unchanged");
                Check(confirmation->Confirm(),"full native foundry reconfirm");pal4::test::ClickNativeCeguiWindow(ui,common_dialog.Window("BtYes"));common_dialog.RethrowEventError();
                const auto product=requirements.Product(learned.At(0,plain));const auto made=inventory.RecentEquipmentSequences();
                Check(made.size()==1u && inventory.ReadDword(inventory.FindInventoryEntry(inventory.Address()+0x7cu,made[0],7u)+0x90u)==product && inventory.ReadDword(inventory.Address()+0x134u)==920u && mutation_sounds==initial_sounds+1u,"real DLL Yes produces actual equipment and charges80 once");
                Check(text(common_dialog.Window("Message")).find("%s")==std::string::npos && !*reinterpret_cast<const unsigned char*>(object+0x81u),"actual creation result prompt replaces confirmation and hides selection");dismiss();
                // Original product limit is checked before mutation even when
                // the initial material confirmation succeeded.
                reset(0u,plain);fill(recipe);for(unsigned i=0;i<99u;++i)(void)inventory.ProcessEquipment(inventory.Address()+0x7cu,product);const auto full_sounds=mutation_sounds;
                Check(requirements.ProductAtLimit(product),"99 real copies satisfy original product limit predicate");
                const auto full_sequences=inventory.RecentEquipmentSequences();
                Check(inventory.EquipmentCount(inventory.ReadDword(inventory.EquipmentDefinitionAddress(product)+0x94u))==99u && full_sequences.size()==6u,"all99 owned instances are distinct from original six-entry recent queue");
                Check(confirmation->Confirm(),"full inventory confirmation");pal4::test::ClickNativeCeguiWindow(ui,common_dialog.Window("BtYes"));common_dialog.RethrowEventError();
                Check(text(common_dialog.Window("Message"))==*pal4::ConvertGameTextToUtf8(*db.FindTableTextById("GameString",0x73u)),"99-product rejection uses original GameString73");
                Check(inventory.RecentEquipmentSequences()==full_sequences && requirements.ProductAtLimit(product),"99-product rejection preserves actual instance sequence view and limit");
                Check(inventory.ReadDword(inventory.Address()+0x134u)==1000u && mutation_sounds==full_sounds,"99-product gate rejects before fee/sound mutation");common_dialog.Close();
                // All mutation backends are reached through their original UI
                // callback functions. The equipment trade/prompt entry remains
                // a separate controller boundary.
                for(unsigned mode=1u;mode<3u;++mode){reset(mode,0u);const auto key=learned.At(mode,0),record=recipes.Address(key);fill(record);unsigned target{};
                    for(const auto& r:db.Tables().at("Equip").rows){const auto definition=std::get<unsigned>(r[0]),entry=inventory.EquipmentDefinitionAddress(definition);
                        if(entry && static_cast<int>(inventory.ReadDword(record+0x7cu+inventory.ReadDword(entry+0x94u)*4u))>0){target=inventory.ProcessEquipment(inventory.Address()+0x7cu,definition);break;}}
                    Check(target!=0u,"original recipe eligible target definition");const auto sequence=inventory.ReadDword(target+0x190u);mutation.WriteScalar(sequence,0x184u,100u);
                    player_memory.WriteDword(target+0x17cu,inventory.ReadDword(record+0x70u));
                    Check(confirmation->Confirm(),"actual initial gates and selection before mutation callback");const auto previous=mutation_sounds;
                    const auto kind=mode==1u?(pass==1u?pal4::SmithWindowTradeCallback::EquipmentSelectionConfirm:pal4::SmithWindowTradeCallback::ItemUseConfirm):
                        (pass==1u?pal4::SmithWindowTradeCallback::PageChangeConfirm:pal4::SmithWindowTradeCallback::ExtendedCraftConfirm);
                    Check(actions->Dispatch(kind,2u) && inventory.ReadDword(inventory.Address()+0x134u)==1000u && mutation_sounds==previous,"equipment mutation cancellation has no side effects");
                    Check(actions->Dispatch(kind,1u),"shared confirmed mutation callback");
                    Check(inventory.ReadDword(inventory.Address()+0x134u)==920u && mutation_sounds==previous+1u && !*reinterpret_cast<const unsigned char*>(object+0x81u),"confirmed equipment action charges once and hides selection");
                    if(mode==1u)Check(inventory.ReadDword(target+0x184u)==100u-inventory.ReadDword(record+0x6cu),"shared strengthening callback writes actual potential");
                    else Check(inventory.ReadDword(target+0x180u)==inventory.ReadDword(record+0x74u),"shared enchantment callback writes actual Skill");
                    dismiss();
                }
                // Actual system entry bypasses the fee in the same primitive.
                smith_active=true;reset(0u,plain);fill(recipe);Check(confirmation->Confirm(),"system foundry action prompt");
                pal4::test::ClickNativeCeguiWindow(ui,common_dialog.Window("BtYes"));common_dialog.RethrowEventError();
                Check(inventory.ReadDword(inventory.Address()+0x134u)==1000u && inventory.RecentEquipmentSequences().size()==1u,"system foundry produces equipment without fee");dismiss();
                // A legitimate positive potential below recipe cost can fail.
                // The fixed fixture clock seeds the real CRT, never a fake roll.
                smith_active=false;unsigned failure_row{};while(failure_row<static_cast<unsigned>(learned.Count(1)) && inventory.ReadDword(recipes.Address(learned.At(1,failure_row))+0x6cu)<=1u)++failure_row;
                Check(failure_row<static_cast<unsigned>(learned.Count(1)),"original strengthening cost above one");reset(1u,failure_row);
                const auto failure_recipe=recipes.Address(learned.At(1,failure_row));fill(failure_recipe);unsigned failure_target{};
                for(const auto& r:db.Tables().at("Equip").rows){const auto definition=std::get<unsigned>(r[0]),entry=inventory.EquipmentDefinitionAddress(definition);
                    if(entry && static_cast<int>(inventory.ReadDword(failure_recipe+0x7cu+inventory.ReadDword(entry+0x94u)*4u))>0){failure_target=inventory.ProcessEquipment(inventory.Address()+0x7cu,definition);break;}}
                Check(failure_target!=0u,"strengthening failure target");const auto failure_sequence=inventory.ReadDword(failure_target+0x190u);mutation.WriteScalar(failure_sequence,0x184u,1u);
                Check(confirmation->Confirm() && actions->Dispatch(pal4::SmithWindowTradeCallback::EquipmentSelectionConfirm,1u),"actual positive-potential failed strengthening callback");
                Check(text(common_dialog.Window("Message"))==*pal4::ConvertGameTextToUtf8(*db.FindTableTextById("GameString",0x38u)) && inventory.ReadDword(inventory.Address()+0x134u)==920u &&
                    inventory.ReadDword(failure_target+0x184u)==1u-inventory.ReadDword(failure_recipe+0x6cu),"failure result retains original potential subtraction and fee");dismiss();
                pal4::test::CheckNativeSmithTrade({ui,layout,*view,*confirmation,*actions,inventory,recipes,db,common_dialog,activity,mutation,player_memory,players,smith_active,mutation_sounds});
                use_actions=false;reset(0u,plain);fill(recipe);smith_active=true;
            }
            const auto recorded_choices=choices.size();
            actions->Close();actions.reset();
            // Closing the owner cancels its own prompt, but must preserve a
            // prompt that a different owner has subsequently installed.
            view->Load(0u);fill(recipe);Export<void (__thiscall*)(void*,int)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(Ptr(list),static_cast<int>(plain));
            Check(confirmation->Confirm(),"pending callback before owner retirement");unsigned foreign{};
            common_dialog.Show(U"Other owner",4u,[&](unsigned){++foreign;},[](std::string_view,unsigned){});
            confirmation->Close();confirmation->Close();Check(text(common_dialog.Window("Message"))=="Other owner","retiring Smith preserves another owner's dialog generation");
            pal4::test::ClickNativeCeguiWindow(ui,common_dialog.Window("BtOk"));common_dialog.RethrowEventError();Check(foreign==1u && choices.size()==recorded_choices,"foreign callback survives and stale Smith callback never runs");
            confirmation.reset();activity.ReleaseStorage();Check(!activity.OutstandingAllocations(),"selection activity storage fully released");
            {pal4::NativeSmithWindowConfirmation retired_confirmation(layout,*view,inventory,recipes,db,common_dialog,activity,activity.Address()-0x134u,
                pal4::NativeSmithConfirmationServices{[&](){return smith_active;},[](std::string_view,unsigned){},[&](pal4::SmithWindowTradeCallback,unsigned){throw std::logic_error("retired Smith callback executed");}});
                Check(retired_confirmation.Confirm(),"own pending callback retirement fixture");retired_confirmation.Close();
                const auto system=Export<void* (__cdecl*)()>(module,"?getSingletonPtr@System@CEGUI@@SAPAV12@XZ")();
                Check(!Export<const void* (__thiscall*)(const void*)>(module,"?getParent@Window@CEGUI@@QBEPAV12@XZ")(Ptr(common_dialog.RootAddress())) &&
                    !Export<void* (__thiscall*)(const void*)>(module,"?getModalTarget@System@CEGUI@@QBEPAVWindow@2@XZ")(system),"retirement detaches its CommonDialog and releases modal target");}
            view->Reset();inventory.Reset();for(const auto& r:recipes.Records())(void)learned.Add(r[0]);view->Load(2u);
        }
        // Constructor deliberately hides these panels. Expose them only in
        // this list fixture; the production controller must run ShowEffectPanel
        // with its real effect and active-frame services before showing them.
        for(const auto offset:{0x84u,0x88u,0x8cu})
            Export<void (__thiscall*)(void*,bool)>(module,"?setVisible@Window@CEGUI@@QAEX_N@Z")(Ptr(layout.Word(offset)),true);
        Check(ui.BindDialogRootParent(layout.RootAddress(),true) && frame.Begin() && ui.Render(),"original Smith DLL layout GPU draw");const auto pixels=frame.ReadPixels();unsigned lit{};
        for(std::size_t i=0;i+3u<pixels.size();i+=4u)if(pixels[i] || pixels[i+1u] || pixels[i+2u])++lit;Check(lit>1000u,"original Smith layout produces GPU pixels");
        if(pass==1u){std::ofstream out("H:/PAL4/objs/M09-smith-ui-20261020-frame.rgba",std::ios::binary);out.write(reinterpret_cast<const char*>(pixels.data()),pixels.size());}
        for(unsigned mode=0;mode<3;++mode){const auto handler=0x49e200u+mode*0x160u;const auto before=std::count(methods.begin(),methods.end(),handler);
            pal4::test::ClickNativeCeguiWindow(ui,layout.Word(0x60u+mode*4u));layout.RethrowEventError();Check(std::count(methods.begin(),methods.end(),handler)==before+1,"real DLL mouse click reaches each original tab subscriber");}
        fail=true;pal4::test::ClickNativeCeguiWindow(ui,layout.Word(0x60u));bool caught{};try{layout.RethrowEventError();}catch(const std::runtime_error& e){caught=std::string_view(e.what())=="Smith callback sentinel";}Check(caught,"DLL callback exceptions retained for host");fail=false;
        retire=true;pal4::test::ClickNativeCeguiWindow(ui,layout.Word(0x64u));caught=false;try{layout.RethrowEventError();}catch(const std::logic_error&){caught=true;}Check(caught && layout.RootAddress(),"live callback cannot destroy its own layout");retire=false;
        if(actions){actions->Close();actions.reset();}if(confirmation){confirmation->Close();confirmation.reset();}activity.ReleaseStorage();
        view->Close();Check(!view->OwnedItemCount(),"list detaches cells before destroying owners");view.reset();layout.Close();layout.Close();Check(!layout.RootAddress(),"original Smith layout destroys idempotently");
    }
    {
        std::unique_ptr<pal4::NativeSmithWindowLists> view;unsigned retired{};bool failed{};
        try{pal4::NativeSmithWindowLayout rejected(ui,root,[](auto,auto,auto){return false;},[&](pal4::NativeSmithWindowLayout& page){
            view=std::make_unique<pal4::NativeSmithWindowLists>(page,inventory,recipes,db,conditions);view->Reset();throw std::runtime_error("Smith reset failure");
        },[&](){++retired;if(view){view->Close();view.reset();}});}catch(const std::runtime_error& e){failed=std::string_view(e.what())=="Smith reset failure";}
        const auto wm=Export<void* (__cdecl*)()>(module,"?getSingletonPtr@WindowManager@CEGUI@@SAPAV12@XZ")();pal4::detail::NativeString name(module,"SmithWindow/Root");
        Check(failed && retired==1u && !view && !Export<bool (__thiscall*)(const void*,const void*)>(module,"?isWindowPresent@WindowManager@CEGUI@@QBE_NABVString@2@@Z")(wm,name.Data()),"failed reset retires list owners before destroying original XML root");
    }
    inventory.Close();Check(!inventory.OutstandingAllocations() && reset_calls==3u && events,"real Smith construction/recreation and native ownership balance");
    std::cout<<"PASS original Smith layout22 controls,9 page+6 navigation subscribers,real reset,150 learned recipes three list modes,seven equipment categories/two instance copies/worn icons/potential and lingyun gates/page arrows,details/material availability,DLL mouse callbacks,GPU,error/lifetime/recreation\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}catch(...){std::cerr<<"FAIL unknown original DLL exception\n";return 1;}
