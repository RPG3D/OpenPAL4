#include "pal4/portable/original_player_data.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool condition){if(!condition)throw std::runtime_error("original player data mismatch");}
bool Same(const PlayerStats& a,const PlayerStats& b){return a.core==b.core && a.rates==b.rates && a.attack==b.attack && a.react==b.react && a.extract==b.extract && a.elements==b.elements;}
int main(int argc,char** argv){try{
    bool rejected=false;try{(void)ReadOriginalPlayerData({});}catch(const std::exception&){rejected=true;}Check(rejected);
    if(argc!=5)return 0;
    auto database=ReadOriginalDatabasePackage(argv[1]);const auto actual=ReadOriginalPlayerData(database);
    std::ifstream r(argv[2],std::ios::binary),u(argv[3],std::ios::binary),e(argv[4],std::ios::binary);
    const auto roles=ReadRoles(r);const auto upgrades=ReadUpgrades(u);const auto equipment=ReadEquipment(e);
    for(std::size_t i=0;i<roles.size();++i)for(std::size_t j=0;j<roles[i].Fields().size();++j){const auto& a=actual.roles[i].Fields()[j];const auto& b=roles[i].Fields()[j];Check(a.name==b.name && a.type==b.type && a.value==b.value);}
    Check(actual.upgrades.size()==upgrades.size());for(std::size_t i=0;i<upgrades.size();++i){const auto& a=actual.upgrades[i];const auto& b=upgrades[i];Check(a.id==b.id && a.role_id==b.role_id && a.level==b.level && a.experience==b.experience && Same(a.stats,b.stats));}
    Check(actual.equipment.classes==equipment.classes && actual.equipment.definitions.size()==equipment.definitions.size());for(const auto& [id,b]:equipment.definitions){const auto& a=actual.equipment.definitions.at(id);Check(a.id==b.id && a.type==b.type && a.skill==b.skill && Same(a.stats,b.stats) && a.fields.size()==b.fields.size());for(std::size_t i=0;i<a.fields.size();++i)Check(a.fields[i].name==b.fields[i].name && a.fields[i].type==b.fields[i].type && a.fields[i].value==b.fields[i].value);}
    database.player_tables.at("EquipRelation").rows.clear();rejected=false;try{(void)ReadOriginalPlayerData(database);}catch(const std::exception&){rejected=true;}Check(rejected);
    std::cout<<"roles="<<roles.size()<<" upgrades="<<upgrades.size()<<" equipment="<<equipment.definitions.size()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
