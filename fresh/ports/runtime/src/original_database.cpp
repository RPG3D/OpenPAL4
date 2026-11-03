#include "pal4/portable/game_path.h"
#include "pal4/portable/original_database.h"
#include <array>
#include "pal4/portable/gbk.h"
#include "pal4/portable/package.h"
#include "pal4/floor_sound_lookup.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <set>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
class Reader {
public:
    explicit Reader(std::span<const std::uint8_t> data):data_(data){}
    std::uint32_t Word(){Require(data_.size()-pos_>=4,"truncated original database");std::uint32_t n=0;for(unsigned i=0;i<4;++i)n|=std::uint32_t(data_[pos_++])<<(8*i);return n;}
    std::uint32_t Count(std::uint32_t max){const auto n=Word();Require(n<=max,"original database count exceeds limit");return n;}
    std::string Text(){const auto n=Count(1024*1024);Require(n<=data_.size()-pos_,"truncated original database text");std::string s(reinterpret_cast<const char*>(data_.data()+pos_),n);pos_+=n;const auto zero=s.find('\0');if(zero!=s.npos)s.resize(zero);return s;}
    void Marker(std::uint32_t value){Require(Word()==value,"invalid original database marker");}
    bool End()const{return pos_==data_.size();}
private:std::span<const std::uint8_t> data_;std::size_t pos_{};
};
struct Field {std::string name;std::uint32_t type;};
}
OriginalDatabaseCatalogs ReadOriginalDatabase(std::span<const std::uint8_t> bytes){
    Reader r(bytes);Require(r.Text()=="GAME_DB_FLAG","invalid original database signature");r.Marker(20060228);
    std::set<std::string> names;OriginalDatabaseCatalogs result;
    for(auto count=r.Count(4096);count--;){
        const auto name=r.Text();Require(names.insert(name).second,"duplicate database property");
        r.Marker(20060303);(void)r.Text();std::set<std::uint32_t> topic_ids;
        for(auto n=r.Count(100000);n--;){
            const auto id=r.Word();auto title=r.Text();
            if(name=="UI"){
                Require(topic_ids.insert(id).second,"duplicate help topic ID");
                result.help_topics.push_back({id,DecodeGbk(title)});
            }
        }
        r.Marker(20060304);
    }
    names.clear();
    for(auto tables=r.Count(4096);tables--;){
        const auto name=r.Text();Require(names.insert(name).second,"duplicate database table");r.Marker(20060225);Require(r.Text()==name,"database table name mismatch");(void)r.Text();(void)r.Word();
        const auto field_count=r.Count(4096),rows=r.Count(100000);std::vector<Field> fields;fields.reserve(field_count);
        for(unsigned i=0;i<field_count;++i){auto field=r.Text();const auto kind=r.Word();Require(kind>=1 && kind<=3,"invalid database field type");(void)r.Text();(void)r.Text();fields.push_back({std::move(field),kind});}
        const bool properties=name=="Property",tutorial=name=="Tutorial",strings=name=="GameString",scenes=name=="Scene",combat=name=="CombatScene",introductions=name=="Introduction",formulas=name=="CombatFormulaConfig";
        const bool player_table=name=="Role" || name=="UpgradeData" || name=="Equip" || name=="EquipRelation";
        const bool help=name=="HelpTips";
        OriginalTable selected;
        const bool capture_table=player_table || formulas;
        if(capture_table){Require(rows<=10000,"selected database table row count exceeds limit");for(const auto& field:fields)selected.fields.push_back({field.name,field.type,{}});selected.rows.reserve(rows);}
        if(formulas)Require(fields.size()>=2 && fields[0].type==1u && fields[1].type==3u,"invalid CombatFormulaConfig key/float schema");
        auto find=[&](const char* field,unsigned type){for(std::size_t i=0;i<fields.size();++i)if(fields[i].name==field && fields[i].type==type)return i;throw std::runtime_error("original database schema mismatch");};
        std::size_t id=0,category=0,value=0,scene_world=0,scene_section=0,scene_title=0,scene_type=0,scene_earth=0,combat_world=0,combat_floor=0,combat_area=0,introduction_name=0,introduction_text=0;
        if(properties || tutorial || strings){Require(rows>0 && rows<=4096,"invalid selected database row count");id=find("ID",1);value=find(properties?"PA_NAME":"STRING",2);if(properties)category=find("TYPE",1);}
        if(scenes){Require(rows>0 && rows<=4096,"invalid scene database row count");id=find("ID",1);scene_world=find("SCENE",2);scene_section=find("SECTION",2);scene_title=find("NAME",2);scene_type=find("TYPE",1);scene_earth=find("EARTHBALL",1);}
        if(combat){Require(field_count==4 && rows<=4096,"invalid combat scene schema");id=find("ID",1);combat_world=find("WORLD",2);combat_floor=find("FLOOR",1);combat_area=find("COMBAT_SCENE",2);}
        if(introductions){Require(rows<=4096,"invalid Introduction row count");Require(field_count==3,"invalid Introduction schema");id=find("ID",1);introduction_name=find("NAME",2);introduction_text=find("INTRODUCTION",2);}
        std::array<std::size_t,6> property_text{};
        if(properties){
            constexpr const char* names[]{"PA_DESCRIPTION","PA_PROPERTY","PA_ICON","PA_MODEL","PA_TEXTURE","PA_ATTACHED_EFFECT"};
            for(unsigned i=0;i<property_text.size();++i)property_text[i]=find(names[i],2);
        }
        std::size_t help_topic=0,help_title=0,help_text=0,help_progress=0;
        if(help){
            Require(field_count==5 && rows<=4096,"invalid HelpTips schema/count");
            id=find("ID",1);help_topic=find("WINDOW",1);help_title=find("CLASS",2);
            help_text=find("TIPS",2);help_progress=find("QUEST_ID",1);
        }
        for(unsigned row=0;row<rows;++row){r.Marker(20060223);std::uint32_t key=0,kind=0;std::string text,intro_name;
            OriginalHelpTip tip;std::array<std::string,6> property_values;
            OriginalSceneDefinition scene;OriginalCombatArea combat_row;std::vector<RoleValue> player_row;if(capture_table)player_row.reserve(fields.size());
            for(std::size_t i=0;i<fields.size();++i){if(fields[i].type==2){auto data=r.Text();if(properties){for(unsigned n=0;n<property_text.size();++n)if(i==property_text[n])property_values[n]=DecodeGbk(data);}if(help){if(i==help_title)tip.title=DecodeGbk(data);if(i==help_text)tip.text=DecodeGbk(data);}if(capture_table)player_row.emplace_back(DecodeGbk(data));if((properties||tutorial||strings) && i==value)text=std::move(data);else if(introductions && i==introduction_name)intro_name=DecodeGbk(data);else if(introductions && i==introduction_text)text=DecodeGbk(data);else if(scenes && i==scene_world)scene.world=std::move(data);else if(scenes && i==scene_section)scene.scene=std::move(data);else if(scenes && i==scene_title)scene.title=DecodeGbk(data);else if(combat && i==combat_world)combat_row.world=std::move(data);else if(combat && i==combat_area)combat_row.area=std::move(data);}else {auto data=r.Word();if(help){if(i==id)tip.id=data;if(i==help_topic)tip.topic_id=data;if(i==help_progress)tip.unlock_progress=std::bit_cast<std::int32_t>(data);}if(fields[i].type==3)Require(std::isfinite(std::bit_cast<float>(data)),"non-finite database value");if(capture_table){if(fields[i].type==3)player_row.emplace_back(std::bit_cast<float>(data));else player_row.emplace_back(data);}if((properties||tutorial||strings||scenes||combat||introductions) && i==id)key=data;if(properties && i==category)kind=data;if(scenes && i==scene_type)scene.type=data;if(scenes && i==scene_earth)scene.earthball=data;if(combat && i==combat_floor)combat_row.floor=std::bit_cast<std::int32_t>(data);}}
            r.Marker(20060224);
            if(help){
                Require(std::none_of(result.help_tips.begin(),result.help_tips.end(),[&](const auto& entry){return entry.id==tip.id;}),"duplicate help tip ID");
                result.help_tips.push_back(std::move(tip));
            }
            if(capture_table)selected.rows.push_back(std::move(player_row));
            if(properties){Require(key && kind<=4 && !text.empty() && text.size()<=1024,"invalid original property");Require(result.properties.emplace(key,PropertyDefinition{key,kind,DecodeGbk(text),std::move(property_values[0]),std::move(property_values[1]),std::move(property_values[2]),std::move(property_values[3]),std::move(property_values[4]),std::move(property_values[5])}).second,"duplicate property ID");}
            if(tutorial || strings){Require(!text.empty() && text.size()<=256*1024,"invalid original text");auto& catalog=tutorial?result.tutorials:result.game_strings;Require(catalog.emplace(key,DecodeGbk(text)).second,"duplicate text ID");}
            if(introductions){Require(key!=0 && !intro_name.empty() && !text.empty(),"invalid original Introduction record");Require(std::none_of(result.introductions.begin(),result.introductions.end(),[&](const auto& record){return record.id==key;}),"duplicate Introduction ID");result.introductions.push_back({key,std::move(intro_name),std::move(text)});}
            if(scenes){scene.id=key;result.scenes.push_back(std::move(scene));}
            if(combat){combat_row.id=key;auto& records=result.combat_areas;auto found=std::find_if(records.begin(),records.end(),[&](const auto& item){return item.id==key;});if(found==records.end())records.push_back(std::move(combat_row));else *found=std::move(combat_row);}
        }
        r.Marker(20060226);
        if(player_table)result.player_tables.emplace(name,std::move(selected));
        if(formulas)result.combat_formula_config=std::move(selected);
    }
    Require(r.End(),"trailing original database data");Require(!result.properties.empty() && !result.tutorials.empty() && !result.game_strings.empty(),"required original database table missing");return result;
}
OriginalDatabaseCatalogs ReadOriginalDatabasePackage(const std::filesystem::path& game_data){
    std::ifstream input(ResolveOriginalPath(game_data/"database.cpk"),std::ios::binary);Require(bool(input),"cannot open original database.cpk");PackageReader package(input);const auto bytes=package.Read("Pal4db.db");return ReadOriginalDatabase(bytes);
}
std::string ResolveCombatArea(const std::vector<OriginalCombatArea>& areas,std::string_view world,std::int32_t current,std::int32_t fallback){
    auto equal=[](std::string_view a,std::string_view b){if(a.size()!=b.size())return false;for(std::size_t i=0;i<a.size();++i){auto lower=[](unsigned char c){return c>='A' && c<='Z'?c+('a'-'A'):c;};if(lower(a[i])!=lower(b[i]))return false;}return true;};
    for(const auto candidate:pal4::FloorSoundLookup::Candidates(current,fallback)){
        const auto floor=static_cast<std::int32_t>(static_cast<double>(pal4::FloorSoundLookup::ScaleInput(candidate)));
        for(const auto& area:areas)if(area.floor==floor && equal(area.world,world))return area.area;
    }
    return {};
}
}
