#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_game_database.h"
#include "pal4/game_data_parameter_field_names.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
unsigned Address(const void* p){return reinterpret_cast<unsigned>(p);}
}
int main(int argc,char** argv)try{
    Check(argc==2,"provide original game directory");pal4::NativeGameDatabase db;Check(db.Load(argv[1]),"original database");
    const auto& table=db.Tables().at("Prescription");unsigned texts{},floats{},material_slots{};std::array<unsigned,3> kinds{};
    for(unsigned pass=0;pass<2u;++pass){pal4::NativePrescriptionRecordCatalog catalog(db);
        Check(catalog.Records().size()==table.rows.size() && catalog.Fields().size()==64u,"complete original Prescription rows and64 fields");
        std::set<unsigned> keys;std::array<unsigned,328> coverage{};
        for(const auto& field:catalog.Fields())for(unsigned n=0;n<(field.kind==pal4::GameDataFieldKind::text?16u:4u);++n)
            Check(++coverage[field.offset+n]==1u,"actual Prescription text/scalars do not overlap");
        for(const auto count:coverage)Check(count==1u,"actual Prescription descriptors cover every328-byte record byte");
        const auto expected=[&](std::size_t row,std::string_view name)->const pal4::NativeGameDbValue&{
            const auto c=std::find_if(table.fields.begin(),table.fields.end(),[&](const auto& f){return f.name==name;});Check(c!=table.fields.end(),"original Prescription expected field");return table.rows[row][c-table.fields.begin()];};
        for(std::size_t row=0;row<catalog.Records().size();++row){const auto& record=catalog.Records()[row];Check(keys.insert(record[0]).second,"unique original Prescription IDs");
            Check(catalog.Address(record[0])==Address(&record),"original signed table lookup maps each ID to its owned328-byte record");
            Check(record[25]<3u,"original Prescription category0..2");if(!pass)++kinds[record[25]];
            for(const auto& field:catalog.Fields()){const auto& value=expected(row,field.name);const auto word=record[field.offset/4u];
                if(field.kind==pal4::GameDataFieldKind::text){const auto pointer=record[field.offset/4u+1u];const auto& text=std::get<std::string>(value);
                    Check(catalog.Text(pointer)==text && record[field.offset/4u+2u]==text.size() && record[field.offset/4u+3u]==text.size(),"original legacy Prescription text/length/capacity");
                    Check(!word,"original text leading byte/allocator field initialized");if(pointer){Check(*(reinterpret_cast<const unsigned char*>(pointer)-1u)==0xfeu && !reinterpret_cast<const char*>(pointer)[text.size()],"owned nonshareable legacy text header and terminator");++texts;}
                }else if(field.kind==pal4::GameDataFieldKind::hit_extra){Check(word==std::bit_cast<unsigned>(std::get<float>(value)),"Prescription extra-hit/reaction floats retain original bits");++floats;}
                else Check(word==std::get<unsigned>(value),"all original Prescription scalar fields match database");
            }
            // Independent IDA-backed offsets used by SmithWindow and crafting,
            // independent of the production descriptor iteration above.
            Check(catalog.Text(record[2])==std::get<std::string>(expected(row,"PRA_NAME")) &&
                catalog.Text(record[6])==std::get<std::string>(expected(row,"PRA_DESCRIPTION")) &&
                catalog.Text(record[10])==std::get<std::string>(expected(row,"PRA_PROPERTY")),"three critical original name/description/property descriptor offsets");
            Check(record[0x68u/4u]==std::get<unsigned>(expected(row,"PRA_PRODUCT_ID")) && record[0x74u/4u]==std::get<unsigned>(expected(row,"PRA_SKILL_ID")),"original crafting output and enchant Skill offsets");
            for(unsigned i=0;i<4u;++i){const auto name="P_MAT_"+std::to_string(i+1u);
                Check(record[0x98u/4u+i]==std::get<unsigned>(expected(row,name)) && record[0xa8u/4u+i]==std::get<unsigned>(expected(row,name+"_NUMBER")),"all four original crafting material ID/count offsets");if(!pass && record[0xa8u/4u+i])++material_slots;}
        }
        Check(!catalog.Address(0xfefefefeu) && catalog.Text(0u).empty(),"missing signed key and empty text");
        bool rejected{};try{(void)catalog.Text(0xfefefefeu);}catch(const std::invalid_argument&){rejected=true;}Check(rejected,"foreign legacy text pointer rejected");
    }
    Check(kinds[0] && kinds[1] && kinds[2] && texts && floats && material_slots,"all original crafting modes and owned field kinds exercised");
    Check(pal4::GameDataParameterFieldNames::NameAt(0x8eef90u)=="PRA_NAME" && pal4::GameDataParameterFieldNames::NameAt(0x8eede0u+48u)=="P_MAT_4" &&
        !pal4::GameDataParameterFieldNames::NameAt(0x8eef91u),"shared field-name lookup preserves original16-byte stride and boundaries");
    std::cout<<"PASS: original Prescription records="<<table.rows.size()<<" modes="<<kinds[0]<<'/'<<kinds[1]<<'/'<<kinds[2]
        <<" material-slots="<<material_slots<<" text-checks="<<texts<<" float-checks="<<floats<<", two catalog lifetimes and exact328-byte schema\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
