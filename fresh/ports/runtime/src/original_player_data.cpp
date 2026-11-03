#include "pal4/portable/original_player_data.h"
#include <bit>
#include <sstream>
#include <stdexcept>

namespace pal4::portable {
namespace {
void Require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void Word(std::ostream& out,std::uint32_t value){for(unsigned i=0;i<4;++i)out.put(static_cast<char>(value>>(i*8)));}
void Text(std::ostream& out,const std::string& value){Require(value.size()<=65536,"original player text exceeds limit");Word(out,static_cast<std::uint32_t>(value.size()));out.write(value.data(),value.size());}
void Value(std::ostream& out,const RoleField& field,const RoleValue& value){
    if(field.type==1)Word(out,std::get<std::uint32_t>(value));
    else if(field.type==2)Text(out,std::get<std::string>(value));
    else if(field.type==3)Word(out,std::bit_cast<std::uint32_t>(std::get<float>(value)));
    else throw std::runtime_error("invalid original player field type");
}
std::size_t Index(const OriginalTable& table,std::string_view name,unsigned type){
    for(std::size_t i=0;i<table.fields.size();++i)if(table.fields[i].name==name && table.fields[i].type==type)return i;
    throw std::runtime_error("missing original player database field "+std::string(name));
}
const OriginalTable& Table(const OriginalDatabaseCatalogs& db,const char* name){
    const auto it=db.player_tables.find(name);if(it==db.player_tables.end())throw std::runtime_error(std::string("missing original database table ")+name);return it->second;
}
void Schema(std::ostream& out,const OriginalTable& table){for(const auto& field:table.fields){Text(out,field.name);Word(out,field.type);}}
void Row(std::ostream& out,const OriginalTable& table,const std::vector<RoleValue>& row){Require(row.size()==table.fields.size(),"invalid original player row");for(std::size_t i=0;i<row.size();++i)Value(out,table.fields[i],row[i]);}
}
OriginalPlayerData ReadOriginalPlayerData(const OriginalDatabaseCatalogs& db){
    const auto& roles=Table(db,"Role");const auto& upgrades=Table(db,"UpgradeData");const auto& equip=Table(db,"Equip");const auto& relation=Table(db,"EquipRelation");
    Require(roles.fields.size()==50 && upgrades.fields.size()==39 && equip.fields.size()==58,"invalid original player schema size");
    std::ostringstream role_bytes(std::ios::binary);role_bytes.write("P4RL",4);Word(role_bytes,1);Word(role_bytes,4);Word(role_bytes,50);
    const auto role_id=Index(roles,"ROLE_ID",1);
    for(std::uint32_t i=0;i<4;++i){const std::vector<RoleValue>* match=nullptr;for(const auto& row:roles.rows){Require(row.size()==roles.fields.size(),"invalid original Role row");if(std::get<std::uint32_t>(row[role_id])==1001+i){Require(!match,"duplicate original Role ID");match=&row;}}Require(match,"missing original Role ID");Word(role_bytes,i);for(std::size_t j=0;j<roles.fields.size();++j){Text(role_bytes,roles.fields[j].name);Word(role_bytes,roles.fields[j].type);Value(role_bytes,roles.fields[j],(*match)[j]);}}
    std::ostringstream upgrade_bytes(std::ios::binary);upgrade_bytes.write("P4UP",4);Word(upgrade_bytes,1);Word(upgrade_bytes,static_cast<std::uint32_t>(upgrades.rows.size()));Word(upgrade_bytes,39);Schema(upgrade_bytes,upgrades);for(const auto& row:upgrades.rows)Row(upgrade_bytes,upgrades,row);
    std::array<std::uint32_t,7> classes{};std::array<bool,7> found{};const auto type=Index(relation,"TYPE",1),category=Index(relation,"CLASS",1);
    for(const auto& row:relation.rows){Require(row.size()==relation.fields.size(),"invalid EquipRelation row");const auto t=std::get<std::uint32_t>(row[type]),c=std::get<std::uint32_t>(row[category]);Require(t<7 && c<5 && !found[t],"invalid EquipRelation");classes[t]=c;found[t]=true;}
    for(bool present:found)Require(present,"incomplete EquipRelation");
    std::ostringstream equipment_bytes(std::ios::binary);equipment_bytes.write("P4EQ",4);Word(equipment_bytes,1);Word(equipment_bytes,static_cast<std::uint32_t>(equip.rows.size()));Word(equipment_bytes,58);for(auto c:classes)Word(equipment_bytes,c);Schema(equipment_bytes,equip);for(const auto& row:equip.rows)Row(equipment_bytes,equip,row);
    std::istringstream role_input(role_bytes.str(),std::ios::binary),upgrade_input(upgrade_bytes.str(),std::ios::binary),equipment_input(equipment_bytes.str(),std::ios::binary);
    return {ReadRoles(role_input),ReadUpgrades(upgrade_input),ReadEquipment(equipment_input)};
}
}
