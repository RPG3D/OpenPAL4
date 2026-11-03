#include "pal4/portable/original_database.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
using namespace pal4::portable;
void Check(bool value){if(!value)throw std::runtime_error("original database test failed");}
void Word(std::string& out,unsigned n){for(unsigned i=0;i<4;++i)out+=char(n>>(8*i));}
void Text(std::string& out,std::string_view text){Word(out,static_cast<unsigned>(text.size()));out+=text;}
void Table(std::string& out,std::string_view name,bool property){
    Text(out,name);Word(out,20060225);Text(out,name);Text(out,"");Word(out,0);Word(out,property?9:2);Word(out,1);
    auto field=[&](std::string_view n,unsigned type){Text(out,n);Word(out,type);Text(out,"");Text(out,"");};
    field("ID",1);if(property)field("TYPE",1);field(property?"PA_NAME":"STRING",2);
    if(property)for(const auto* name:{"PA_TEXTURE","PA_DESCRIPTION","PA_ATTACHED_EFFECT","PA_MODEL","PA_PROPERTY","PA_ICON"})field(name,2);
    Word(out,20060223);Word(out,7);if(property)Word(out,4);Text(out,"test");
    if(property)for(const auto* value:{"texture","\xd0\xc2\xca\xd6","effect","model","<dc0>property</dc0>","icon"})Text(out,value);
    Word(out,20060224);Word(out,20060226);
}
void CombatTable(std::string& out){
    Text(out,"CombatScene");Word(out,20060225);Text(out,"CombatScene");Text(out,"");Word(out,0);Word(out,4);Word(out,3);
    auto field=[&](std::string_view name,unsigned type){Text(out,name);Word(out,type);Text(out,"");Text(out,"");};
    field("FLOOR",1);field("COMBAT_SCENE",2);field("ID",1);field("WORLD",2);
    auto row=[&](unsigned floor,std::string_view area,unsigned id){Word(out,20060223);Word(out,floor);Text(out,area);Word(out,id);Text(out,"M01");Word(out,20060224);};
    row(10,"old",1);row(10,"second",2);row(10,"replacement",1);Word(out,20060226);
}
void IntroductionTable(std::string& out){
    Text(out,"Introduction");Word(out,20060225);Text(out,"Introduction");Text(out,"");Word(out,0);Word(out,3);Word(out,1);
    auto field=[&](std::string_view name,unsigned type){Text(out,name);Word(out,type);Text(out,"");Text(out,"");};
    field("ID",1);field("NAME",2);field("INTRODUCTION",2);
    Word(out,20060223);Word(out,1);Text(out,"Name");Text(out,"Body");Word(out,20060224);Word(out,20060226);
}
void HelpTable(std::string& out){
    Text(out,"HelpTips");Word(out,20060225);Text(out,"HelpTips");Text(out,"");Word(out,0);Word(out,5);Word(out,2);
    auto field=[&](std::string_view name,unsigned type){Text(out,name);Word(out,type);Text(out,"");Text(out,"");};
    // Reordered schema proves name/type binding rather than x86 struct offsets.
    field("TIPS",2);field("QUEST_ID",1);field("WINDOW",1);field("ID",1);field("CLASS",2);
    for(unsigned id:{255u,256u}){
        Word(out,20060223);Text(out,"<dc0>Help\nBody</dc0>");Word(out,0xffffffff);Word(out,7);Word(out,id);Text(out,"\xd0\xc2\xca\xd6");Word(out,20060224);
    }
    Word(out,20060226);
}
int main(int argc,char** argv){try{
    if(argc==2){const auto db=ReadOriginalDatabasePackage(argv[1]);
        Check(db.help_topics.size()==11 && db.help_tips.size()==85);
        Check(db.help_topics.front().id==0 && db.help_topics.front().title=="新手上路");
        Check(db.help_tips.front().id==1 && db.help_tips.front().title=="储存游戏进度" && db.help_tips.front().text.find("<image set=maimai0 name=sy_xitong/>")!=std::string::npos);
        auto hash=[](std::string_view text){std::uint64_t value=14695981039346656037ull;for(unsigned char byte:text){value^=byte;value*=1099511628211ull;}return value;};
        for(const auto& topic:db.help_topics)std::cout<<"HELP_TOPIC "<<topic.id<<' '<<hash(topic.title)<<'\n';
        for(const auto& tip:db.help_tips)std::cout<<"HELP_TIP "<<tip.id<<' '<<tip.topic_id<<' '<<tip.unlock_progress<<' '<<hash(tip.title)<<' '<<hash(tip.text)<<'\n';
        Check(db.combat_areas.size()==58);Check(ResolveCombatArea(db.combat_areas,"m01",25,0)=="M01");Check(db.introductions.size()==18);Check(db.introductions.front().id==1 && db.introductions.front().name=="仙剑足迹" && !db.introductions.front().text.empty());std::cout<<"original combat areas="<<db.combat_areas.size()<<" introductions="<<db.introductions.size()<<'\n';return 0;}
    if(argc==5){const auto db=ReadOriginalDatabasePackage(argv[1]);std::ifstream in(argv[2],std::ios::binary);const auto expected=ReadPropertyCatalog(in);Check(db.properties.size()==expected.size());for(const auto& [id,p]:expected){const auto& actual=db.properties.at(id);Check(p.category==actual.category && p.name==actual.name);}
        std::ifstream tutorials(argv[3],std::ios::binary);const auto tutorial=ReadTextCatalog(tutorials,"P4TU");Check(db.tutorials==tutorial);
        std::ifstream strings(argv[4],std::ios::binary);const auto text=ReadTextCatalog(strings,"P4GS");for(const auto& [id,value]:text)Check(db.game_strings.at(id)==value);
        std::cout<<"original package properties="<<db.properties.size()<<" tutorials="<<db.tutorials.size()<<" strings="<<db.game_strings.size()<<'\n';return 0;}
    std::string data;Text(data,"GAME_DB_FLAG");Word(data,20060228);Word(data,1);
    Text(data,"UI");Word(data,20060303);Text(data,"UI");Word(data,2);
    Word(data,7);Text(data,"\xd0\xc2\xca\xd6");Word(data,0);Text(data,"second");Word(data,20060304);
    Word(data,6);Table(data,"Property",true);Table(data,"Tutorial",false);Table(data,"GameString",false);CombatTable(data);IntroductionTable(data);HelpTable(data);
    auto parse=[](const std::string& s){return ReadOriginalDatabase(std::span(reinterpret_cast<const std::uint8_t*>(s.data()),s.size()));};
    const auto db=parse(data);
    const auto& property=db.properties.at(7);
    Check(property.description=="新手" && property.property=="<dc0>property</dc0>" && property.model=="model" && property.texture=="texture" && property.icon=="icon" && property.attached_effect=="effect");
    auto wrong_property=data;wrong_property.replace(wrong_property.find("PA_DESCRIPTION"),14,"PB_DESCRIPTION");
    bool property_rejected=false;try{(void)parse(wrong_property);}catch(const std::runtime_error&){property_rejected=true;}Check(property_rejected);
    Check(db.properties.at(7).category==4 && db.tutorials.at(7)=="test" && db.game_strings.at(7)=="test");Check(db.introductions.size()==1 && db.introductions[0].id==1 && db.introductions[0].name=="Name" && db.introductions[0].text=="Body");
    Check(db.combat_areas.size()==2 && ResolveCombatArea(db.combat_areas,"m01",25,0)=="replacement");
    Check(db.help_topics.size()==2 && db.help_topics[0].id==7 && db.help_topics[0].title=="新手" && db.help_topics[1].id==0);
    Check(db.help_tips.size()==2 && db.help_tips[0].id==255 && db.help_tips[1].id==256);
    Check(db.help_tips[0].topic_id==7 && db.help_tips[0].title=="新手" && db.help_tips[0].unlock_progress==-1 && db.help_tips[0].text=="<dc0>Help\nBody</dc0>");
    auto wrong_schema=data;wrong_schema.replace(wrong_schema.find("QUEST_ID"),8,"BROKENID");
    bool schema_rejected=false;try{(void)parse(wrong_schema);}catch(const std::runtime_error&){schema_rejected=true;}Check(schema_rejected);
    const std::vector<OriginalCombatArea> areas{{1,10,"M01","adjacent"},{2,0,"M01","fallback"},{3,10,"M01","later"}};
    Check(ResolveCombatArea(areas,"m01",25,0)=="adjacent");
    Check(ResolveCombatArea(areas,"M01",200,0)=="fallback");
    Check(ResolveCombatArea(areas,"other",25,0).empty());
    for(std::size_t n=0;n<data.size();++n){bool failed=false;try{(void)parse(data.substr(0,n));}catch(const std::exception&){failed=true;}Check(failed);}
    bool failed=false;try{(void)parse(data+"x");}catch(const std::exception&){failed=true;}Check(failed);
    std::cout<<"original database contracts passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
