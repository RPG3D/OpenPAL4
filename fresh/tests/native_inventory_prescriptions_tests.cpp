#include "pal4/native_inventory_prescriptions.h"
#include "pal4/native_script_prescription_addition.h"
#include "pal4/native_script_inventory_change.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_inventory_save_runtime.h"
#include "pal4/native_inventory_load_runtime.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/game_text_format.h"
#include <windows.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(int argc,char** argv)try{
    Check(argc==2,"original root required");pal4::NativeGameDatabase db;Check(db.Load(argv[1]),"original database");
    pal4::NativePrescriptionRecordCatalog recipes(db);pal4::NativeInventorySession inventory;inventory.BindDatabase(db);(void)inventory.Start();inventory.Reset();
    pal4::NativeInventoryPrescriptions learned(inventory,recipes);
    for(int mode=0;mode<3;++mode)Check(!learned.Count(mode),"new inventory has no unlocked recipes");
    unsigned creates{},shown{},sounds{};bool allow_hint=true,throw_show{};std::string hint_text;
    pal4::NativeScriptInventoryChange hints(inventory,db,[&](){++creates;return allow_hint?1u:0u;},
        [&](unsigned hint,std::string_view text,float x,float y,float duration,unsigned parent,std::uint8_t flag){
            Check(hint==1u && x==0.5f && y==0.15f && duration==1.0f && !parent && flag==1u,"original recipe hint ABI");
            if(throw_show)throw std::runtime_error("injected hint presentation failure");hint_text=text;++shown;
        },[&](std::string_view path,unsigned mode){Check(path=="gamedata\\PALSound\\WA06.wav" && mode==1u,"recipe gain sound ABI");++sounds;});
    pal4::NativeScriptPrescriptionAddition addition(inventory,recipes,db,hints);
    pal4::NativeGameScriptServices services;services.prescription_addition=&addition;
    pal4::NativeGameScriptCallbacks callbacks(services);pal4::NativeAngelScriptEngine engine;engine.SetCallbacks(&callbacks);
    std::array<std::vector<unsigned>,3> expected;std::string script="void All(){\n";
    for(auto it=recipes.Records().rbegin();it!=recipes.Records().rend();++it){const auto& r=*it;expected[r[25]].push_back(r[0]);script+="giAddPrescription("+std::to_string(r[0])+", false);\n";}
    script+="giAddPrescription(-123, true);}\n";std::array<unsigned,3> sample{};
    for(unsigned mode=0;mode<3;++mode){std::sort(expected[mode].begin(),expected[mode].end());sample[mode]=expected[mode].front();}
    script+="void Shown(){giAddPrescription("+std::to_string(sample[0])+", true);}\nvoid Gate(){giAddPrescription("+std::to_string(sample[1])+", false);}\n";
    Check(engine.Raw()->AddScriptSection("prescriptions","test.cs",script.data(),static_cast<int>(script.size()))>=0 && engine.Raw()->Build("prescriptions")>=0,"compile actual SDK prescription calls");
    asIScriptContext* raw{};Check(engine.Raw()->CreateContext(&raw)>=0 && raw,"create SDK context");
    struct Release{asIScriptContext* c;~Release(){(void)c->Release();}} release{raw};
    const auto run=[&](const char* name){const auto f=engine.FunctionId("prescriptions",name);Check(f>=0 && raw->Prepare(f)>=0,"prepare SDK entry");return raw->Execute();};
    Check(run("All")==asEXECUTION_FINISHED && creates==recipes.Records().size()+1u && !shown && !sounds && !addition.OutstandingTexts(),"SDK silent calls add real recipes,missing ID ignores hints");
    for(unsigned mode=0;mode<3;++mode){Check(learned.Count(mode)==expected[mode].size(),"learned category sizes");for(unsigned i=0;i<expected[mode].size();++i)Check(learned.At(mode,i)==expected[mode][i] && learned.Contains(static_cast<int>(expected[mode][i])),"shared sorted unique collection maps every recipe");}
    const auto allocations=inventory.OutstandingAllocations();for(const auto& row:recipes.Records())(void)learned.Add(row[0]);
    Check(inventory.OutstandingAllocations()==allocations && !learned.Add(0xfefefefeu) && !learned.Contains(-123),"duplicates/unknown IDs do not allocate or enter collection");
    Check(run("Shown")==asEXECUTION_FINISHED && shown==1u && sounds==1u && !addition.OutstandingTexts(),"duplicate script still shows original hint but remains unique");
    const auto row=recipes.Address(sample[0]);Check(hint_text==pal4::FormatGameTextArgument(*db.FindTableTextById("GameString",0x7au),recipes.Text(inventory.ReadDword(row+8u))),"GameString7A includes complete recipe name");
    throw_show=true;Check(run("Shown")==asEXECUTION_EXCEPTION && !addition.OutstandingTexts() && sounds==1u,"shared cleanup releases both texts after UI failure");throw_show=false;
    pal4::NativeGameSaveArchiveBuffer buffer;pal4::NativeInventorySaveRuntime save(inventory,buffer);Check(save.Serialize()==1u,"serialize all unlocked recipes");
    const auto root=std::filesystem::path("H:/PAL4/objs/prescriptions-roundtrip")/std::to_string(GetCurrentProcessId());std::filesystem::create_directories(root/"save");
    {std::ofstream out(root/"save/0.dat",std::ios::binary);out.write(reinterpret_cast<const char*>(buffer.Bytes().data()),buffer.Bytes().size());Check(static_cast<bool>(out),"write recipe fixture");}
    pal4::NativeInventorySession restored;restored.BindDatabase(db);(void)restored.Start();pal4::NativeGameSaveArchiveReader input;Check(input.OpenSlot(root,0),"open recipe fixture");
    pal4::NativeInventoryLoadRuntime load(restored,input);Check(load.Deserialize()==1u && !input.Remaining(),"restore recipes through actual inventory codec");pal4::NativeInventoryPrescriptions loaded(restored,recipes);
    for(unsigned mode=0;mode<3;++mode){Check(loaded.Count(mode)==expected[mode].size(),"restored category count");for(unsigned i=0;i<expected[mode].size();++i)Check(loaded.At(mode,i)==expected[mode][i],"restored recipe order");}
    restored.Close();Check(!restored.OutstandingAllocations(),"restored recipe buffers released");
    bool bad_mode{},bad_index{};try{(void)learned.Count(3);}catch(const std::out_of_range&){bad_mode=true;}try{(void)learned.At(0,static_cast<unsigned>(learned.Count(0)));}catch(const std::out_of_range&){bad_index=true;}
    Check(bad_mode && bad_index,"shared mode/subscript rejection");inventory.Reset();for(int mode=0;mode<3;++mode)Check(!learned.Count(mode),"NewGame reset empties learned recipes");
    allow_hint=false;Check(run("Gate")==asEXECUTION_FINISHED && !learned.Contains(sample[1]),"missing hint prevents add even with show=false");allow_hint=true;
    Check(run("Gate")==asEXECUTION_FINISHED && learned.Contains(sample[1]),"same native view works after reset");inventory.Close();bool closed{};try{(void)learned.Count(0);}catch(const std::logic_error&){closed=true;}
    Check(closed && !inventory.OutstandingAllocations() && !addition.OutstandingTexts(),"closed collection rejects access and native storage balanced");
    std::cout<<"PASS original learned recipes="<<recipes.Records().size()<<" categories="<<expected[0].size()<<'/'<<expected[1].size()<<'/'<<expected[2].size()<<"; actual SDK giAddPrescription,duplicates,hints,error cleanup,save/load,reset,bounds\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
