#include "pal4/native_crafting_strengthening.h"
#include "pal4/native_inventory_equipment_mutation.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_skill_numeric.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>

namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
unsigned Word(unsigned a){unsigned v;std::memcpy(&v,reinterpret_cast<const void*>(a),4);return v;}
// Test oracle uses the original x87 truncating conversion instruction. The
// production bridge uses the shared public host numeric conversion instead.
std::int64_t X87Integer(double value){
    unsigned short original{},truncate{};std::int64_t result{};
    __asm { fnstcw original }
    truncate=original|0x0c00u;
    __asm { fldcw truncate
        fld value
        fistp result
        fldcw original }
    return result;
}
float X87Float(float base,float modifier,bool percentage,bool spill){
    float result{},delta{};const double scale=0.01;
    if(!percentage){__asm { fld base
        fadd modifier
        fstp result }}
    else {
        __asm { fld base
            fmul modifier
            fmul scale }
        if(spill){__asm { fstp delta
            fld base
            fadd delta
            fstp result }}
        else {__asm { fadd base
            fstp result }}
    }
    return result;
}
std::array<unsigned,36> Expected(std::array<unsigned,36> words,const pal4::GameDataParameterRecord& recipe){
    const bool percentage=recipe[81]!=0u;
    for(unsigned word=1;word<36;++word){
        const auto base=words[word];unsigned modifier_offset{};bool floating{},spill{};
        if(word<=8u)modifier_offset=0xb8u+(word-1u)*4u;
        else if(word<=12u){modifier_offset=0x134u+(word-9u)*4u;floating=true;spill=word==12u;}
        else if(word<=18u)modifier_offset=0xecu+(word-13u)*4u;
        else if(word<=24u){modifier_offset=0x11cu+(word-19u)*4u;floating=true;}
        else if(word<=30u){modifier_offset=0x104u+(word-25u)*4u;floating=true;spill=word==30u;}
        else modifier_offset=0xd8u+(word-31u)*4u;
        const auto modifier=recipe[modifier_offset/4u];
        if(floating)words[word]=std::bit_cast<unsigned>(X87Float(std::bit_cast<float>(base),std::bit_cast<float>(modifier),percentage,spill));
        else if(!percentage)words[word]=base+modifier;
        else {
            const bool subtract=(word>=13u && word<=17u) || (word>=31u && word<=34u);
            const auto product=std::bit_cast<int>(base*modifier);
            const auto delta=static_cast<unsigned>(X87Integer(static_cast<double>(product)*(subtract?-0.01:0.01)));
            words[word]=subtract?base-delta:base+delta;
        }
    }
    return words;
}
}
int main(int argc,char** argv)try{
    Check(argc==2,"original root required");
    for(const double value:{0.0,-0.0,1.9,-1.9,2147483648.25,4294967297.5,-4294967297.5,1e40,-1e40,
        std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})
        Check(pal4::TruncateSkillFloat(value)==X87Integer(value),"shared double conversion matches original x87 truncation including64-bit invalid result");
    // Compare the public host stream against the directly observed original
    // constants as a boundary test, not a private runtime replacement. The
    // manifest leaves seed/rand archive identity pending.
    for(unsigned seed:{0u,1u,12345u,0xffffffffu}){std::srand(seed);auto state=seed;
        for(unsigned i=0;i<12u;++i){state=state*214013u+2531011u;Check(std::rand()==static_cast<int>((state>>16u)&0x7fffu),"public CRT rand sequence matches directly observed original body");}}
    std::array<unsigned,100> seeds{};
    for(unsigned seed=1u;seed<100000u;++seed){std::srand(seed);const auto roll=std::rand()%100;if(!seeds[roll])seeds[roll]=seed;}
    for(const auto seed:seeds)Check(seed!=0u,"real CRT seeds cover all chance boundaries");
    pal4::NativeGameDatabase db;Check(db.Load(argv[1]),"original database");pal4::NativePrescriptionRecordCatalog recipes(db);
    auto alternate_table=db.Tables().at("Prescription");
    const auto flag=std::find_if(recipes.Fields().begin(),recipes.Fields().end(),[](const auto& f){return f.offset==0x144u;});Check(flag!=recipes.Fields().end(),"original percentage flag descriptor");
    const auto flag_column=std::find_if(alternate_table.fields.begin(),alternate_table.fields.end(),[&](const auto& f){return f.name==flag->name;});Check(flag_column!=alternate_table.fields.end(),"percentage flag source column");
    for(auto& row:alternate_table.rows){auto& v=row[flag_column-alternate_table.fields.begin()];v=std::uint32_t(std::get<unsigned>(v)?0u:1u);}
    pal4::NativePrescriptionRecordCatalog alternate(alternate_table);
    pal4::NativeInventorySession inventory;inventory.BindDatabase(db);(void)inventory.Start();inventory.BindItemObserver([](unsigned){});
    pal4::NativeInventoryEquipmentMutation mutate(inventory);
    std::array<unsigned,7> equipment{};const auto& equips=db.Tables().at("Equip");
    const auto f=std::find_if(equips.fields.begin(),equips.fields.end(),[](const auto& f){return f.name=="ID";});Check(f!=equips.fields.end(),"Equip ID");
    for(const auto& row:equips.rows){const auto id=std::get<unsigned>(row[f-equips.fields.begin()]);const auto def=inventory.EquipmentDefinitionAddress(id);
        const auto type=Word(def+0x94u);if(type<7u && !equipment[type])equipment[type]=id;}
    for(const auto id:equipment)Check(id!=0u,"seven actual equipment types");
    unsigned next_seed{},clocks{},sounds{};std::string last_sound;
    pal4::NativeCraftingStrengthening strengthening(recipes,inventory,[&](std::string_view name,unsigned mode){
        Check(mode==1u,"strengthening original sound mode");last_sound=name;++sounds;
    },[&](){++clocks;return next_seed;});
    unsigned cases{},successes{},failures{},flat{},percentage{},rows{},original_flat{},original_percentage{};
    for(const auto* source:{&recipes,&alternate}){
    pal4::NativeCraftingStrengthening variant(*source,inventory,[&](std::string_view name,unsigned mode){Check(mode==1u,"variant sound mode");last_sound=name;++sounds;},[&](){++clocks;return next_seed;});
    for(const auto& recipe:source->Records())if(recipe[25]==1u){++rows;
        if(source==&recipes){if(recipe[81])++original_percentage;else ++original_flat;}
        if(recipe[81])++percentage;else ++flat;
        for(unsigned type=0;type<7;++type)for(unsigned mode=0;mode<2;++mode)for(const int difference:{0,3,-1,-2,-3,-4,-5}){
            const auto chance=difference>=0?100:11+difference<7?6:11+difference;
            for(unsigned outcome=0;outcome<(chance==100?1u:2u);++outcome){
                inventory.Reset();(void)inventory.UpdateCounter(1000);
                const auto entry=inventory.ProcessEquipment(inventory.Address()+0x7cu,equipment[type]),seq=Word(entry+0x190u);
                const auto other=inventory.ProcessEquipment(inventory.Address()+0x7cu,equipment[type]);std::array<unsigned,101> other_image{};std::memcpy(other_image.data(),reinterpret_cast<const void*>(other),404u);
                std::array<unsigned,36> before{};before[0]=Word(entry);
                for(unsigned word=1;word<36;++word){const bool fp=(word>=9u && word<=12u) || (word>=19u && word<=30u);
                    before[word]=fp?std::bit_cast<unsigned>(0.6175009608268738f+static_cast<float>(word)*0.125f):12345u+word;
                    mutate.WriteScalar(seq,word*4u,before[word]);}
                mutate.WriteScalar(seq,0x184u,recipe[0x6cu/4u]+static_cast<unsigned>(difference));
                std::map<unsigned,unsigned> materials;
                for(unsigned n=0;n<4;++n)if(static_cast<int>(recipe[0xa8u/4u+n])>0)materials.emplace(recipe[0x98u/4u+n],recipe[0xa8u/4u+n]);
                for(const auto [id,count]:materials){Check(inventory.PropertyAddress(id)!=0u,"original strengthening needs direct items");(void)inventory.AddItem(id,static_cast<int>(count+2u));}
                const auto order=inventory.RecentEquipmentSequences();const auto allocations=inventory.OutstandingAllocations();
                const auto roll=outcome?chance:chance-1;next_seed=seeds[roll];std::srand(next_seed);Check(std::rand()%100==roll,"chosen real seed");const auto second_random=std::rand();
                const auto prior_clocks=clocks,prior_sounds=sounds;
                const auto result=variant.Apply(recipe[0],seq,mode!=0u);
                Check(result==(outcome?-1:0) && clocks==prior_clocks+1u && sounds==prior_sounds+1u && std::rand()==second_random,"real reseed/one draw,exact boundary outcome and one clock/sound");
                Check(last_sound==(outcome?"wa09a.wav":"wa09.wav"),"original success/failure sound");
                Check(Word(entry+0x184u)==static_cast<unsigned>(difference),"potential deducted on both random outcomes");
                const auto expected=outcome?before:Expected(before,recipe);std::array<unsigned,36> actual{};std::memcpy(actual.data(),reinterpret_cast<const void*>(entry),144u);
                if(actual!=expected){for(unsigned word=0;word<36;++word)if(actual[word]!=expected[word])std::cerr<<"recipe="<<recipe[0]<<" word="<<word<<" actual="<<actual[word]<<" expected="<<expected[word]<<'\n';throw std::runtime_error("all35 native stat fields match independent x87 oracle");}
                Check(inventory.ReadDword(inventory.Address()+0x134u)==(mode?1000u:920u),"both random outcomes obey original fee flag");
                for(const auto [id,count]:materials)Check(inventory.ItemCount(id)==2u,"exact material consumption on both outcomes");
                Check(inventory.RecentEquipmentSequences()==order && inventory.OutstandingAllocations()==allocations && !std::memcmp(other_image.data(),reinterpret_cast<const void*>(other),404u),"strengthening preserves identity,other copy,text ownership and collection");
                ++cases;if(outcome)++failures;else ++successes;
            }
        }
    }}
    const auto calls=clocks;bool bad_recipe{},bad_target{},closed{};
    try{(void)strengthening.Apply(0xfefefefeu,1u,true);}catch(const std::out_of_range&){bad_recipe=true;}
    try{(void)strengthening.Apply(recipes.Records()[0][0],0xfefefefeu,true);}catch(const std::out_of_range&){bad_target=true;}
    inventory.Close();try{(void)strengthening.Apply(recipes.Records()[0][0],1u,true);}catch(const std::logic_error&){closed=true;}
    Check(bad_recipe && bad_target && closed && clocks==calls && !inventory.OutstandingAllocations(),"invalid requests rejected before random state and native lifecycle balanced");
    Check(rows==88u && original_flat+original_percentage==44u && flat==44u && percentage==44u && successes && failures,"all44 original recipes plus explicit toggled-mode fixtures cover both stat modes");
    std::cout<<"PASS strengthening original-rows="<<original_flat+original_percentage<<" original-flat="<<original_flat<<" original-percentage="<<original_percentage<<" alternate-mode-rows=44 cases="<<cases<<" success="<<successes<<" failure="<<failures<<"; real CRT threshold seeds,35 stats against x87 oracle,exact materials/potential/money/lifecycle\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
