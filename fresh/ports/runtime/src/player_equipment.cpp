#include "pal4/portable/player_equipment.h"
#include "pal4/script_player_equipment.h"
#include "pal4/game_data_race_stats.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iomanip>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <unordered_set>

namespace pal4::portable {
namespace {
void Require(bool valid,const char* error) {if(!valid)throw std::runtime_error(error);}
std::uint32_t Word(std::istream& input) {
    unsigned char b[4]{};Require(bool(input.read(reinterpret_cast<char*>(b),4)),"truncated equipment cache");
    return b[0]|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);
}
std::string Text(std::istream& input,std::uint32_t limit) {
    const auto size=Word(input);Require(size<=limit,"equipment text exceeds limit");
    std::string text(size,'\0');Require(bool(input.read(text.data(),size)),"truncated equipment text");
    Require(text.find('\0')==std::string::npos,"equipment embedded NUL");
    for(std::size_t i=0;i<text.size();) {
        const auto c=static_cast<unsigned char>(text[i++]);if(c<128)continue;
        unsigned count{};std::uint32_t point{},minimum{};
        if(c>=0xc2 && c<=0xdf){count=1;point=c&31;minimum=0x80;}
        else if(c>=0xe0 && c<=0xef){count=2;point=c&15;minimum=0x800;}
        else if(c>=0xf0 && c<=0xf4){count=3;point=c&7;minimum=0x10000;}
        else throw std::runtime_error("invalid equipment UTF-8");
        Require(text.size()-i>=count,"truncated equipment UTF-8");
        while(count--){const auto next=static_cast<unsigned char>(text[i++]);Require((next&0xc0)==0x80,"invalid equipment UTF-8 continuation");point=(point<<6)|(next&63);}
        Require(point>=minimum && point<=0x10ffff && !(point>=0xd800 && point<=0xdfff),"invalid equipment Unicode point");
    }
    return text;
}
const RoleValue& Field(const EquipmentDefinition& record,std::string_view name) {
    for(const auto& field:record.fields)if(field.name==name)return field.value;
    throw std::runtime_error("missing equipment field "+std::string(name));
}
// 与原400字节Equip记录的前144字节一致；这里全部为数值。
pal4::GameDataRaceStats Encode(const PlayerStats& stats) {
    pal4::GameDataRaceStats result;(void)result.Reset();auto& words=result.Data();
    std::copy(stats.core.begin(),stats.core.end(),words.begin()+1);
    for(std::size_t i=0;i<4;++i)words[9+i]=std::bit_cast<std::uint32_t>(stats.rates[i]);
    for(std::size_t i=0;i<6;++i){words[13+i]=stats.attack[i];words[19+i]=std::bit_cast<std::uint32_t>(stats.react[i]);words[25+i]=std::bit_cast<std::uint32_t>(stats.extract[i]);}
    std::copy(stats.elements.begin(),stats.elements.end(),words.begin()+31);return result;
}
PlayerStats Decode(const pal4::GameDataRaceStats& data) {
    PlayerStats stats;const auto& words=data.Data();
    std::copy_n(words.begin()+1,8,stats.core.begin());
    for(std::size_t i=0;i<4;++i)stats.rates[i]=std::bit_cast<float>(words[9+i]);
    for(std::size_t i=0;i<6;++i){stats.attack[i]=words[13+i];stats.react[i]=std::bit_cast<float>(words[19+i]);stats.extract[i]=std::bit_cast<float>(words[25+i]);}
    std::copy_n(words.begin()+31,5,stats.elements.begin());return stats;
}
std::int32_t Clamp(std::int32_t value,std::uint32_t maximum) {
    const auto cap=std::bit_cast<std::int32_t>(maximum);return value>cap?cap:(value<0?0:value);
}
}
EquipmentCatalog ReadEquipment(std::istream& input) {
    Require(Word(input)==0x51453450 && Word(input)==1,"unsupported equipment cache");
    const auto count=Word(input);Require(count>0 && count<=10000 && Word(input)==58,"invalid equipment counts");
    EquipmentCatalog result;for(auto& category:result.classes){category=Word(input);Require(category<5,"invalid equipment relation");}
    std::vector<RoleField> schema(58);std::unordered_set<std::string> names;
    for(auto& field:schema){field.name=Text(input,64);field.type=Word(input);
        Require(!field.name.empty() && field.name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_")==std::string::npos && names.insert(field.name).second,"invalid equipment schema");
        Require(field.type>=1 && field.type<=3,"invalid equipment field type");}
    constexpr std::array core={"CA_MAX_HP","CA_ADDITIONAL_RAGE","CA_MAX_MP","CA_PHYSICAL","CA_TOUGHNESS","CA_SPEED","CA_LUCKY","CA_WILL"};
    constexpr std::array rates={"CA_ADDITIONAL_CRITICAL","CA_FEND_OFF","CA_ADDITIONAL_HITTING","CA_PAY_FOR_SPARE"};
    constexpr std::array channels={"PHYSICAL","WATER","FIRE","THUNDER","AIR","EARTH"};
    constexpr std::array elements={"EFE_WATER","EFE_FIRE","EFE_THUNDER","EFE_AIR","EFE_EARTH"};
    for(std::uint32_t index=0;index<count;++index) {
        EquipmentDefinition record;record.fields=schema;
        for(auto& field:record.fields) {
            if(field.type==2)field.value=Text(input,65536);
            else if(field.type==1)field.value=Word(input);
            else {const auto value=std::bit_cast<float>(Word(input));Require(std::isfinite(value),"non-finite equipment value");field.value=value;}
        }
        const auto number=[&](std::string_view name){return std::get<std::uint32_t>(Field(record,name));};
        const auto real=[&](std::string_view name){return std::get<float>(Field(record,name));};
        record.id=number("ID");record.type=number("TYPE");record.skill=number("EA_SKILL_ID");
        Require(record.id>0 && record.type<7,"invalid equipment ID/type");
        for(std::size_t i=0;i<core.size();++i)record.stats.core[i]=number(core[i]);
        for(std::size_t i=0;i<rates.size();++i)record.stats.rates[i]=real(rates[i]);
        for(std::size_t i=0;i<channels.size();++i){const auto prefix=std::string("CA_ATTACK_")+channels[i];record.stats.attack[i]=number(prefix+"_ADDITIONAL");record.stats.react[i]=real(prefix+"_REACT");record.stats.extract[i]=real(prefix+"_EXTRACT");}
        for(std::size_t i=0;i<elements.size();++i)record.stats.elements[i]=number(elements[i]);
        Require(result.definitions.emplace(record.id,std::move(record)).second,"duplicate equipment ID");
    }
    Require(input.peek()==std::char_traits<char>::eof() && !input.bad(),"trailing equipment bytes");return result;
}
struct PlayerEquipment::Impl final:pal4::IScriptPlayerEquipmentRuntime {
    static constexpr std::uint32_t manager=0x10000,database=0x20000,inventory=0x30000,records=0x100000;
    PlayerRoster& players;const EquipmentCatalog& catalog;
    std::vector<EquipmentInstance> instances;
    std::vector<std::uint32_t> recent;
    std::unordered_multimap<std::uint32_t,std::uint32_t> acquired; // 原全局装备获取索引。
    std::uint32_t calls{},refreshes{};
    Impl(PlayerRoster& p,const EquipmentCatalog& c):players(p),catalog(c){}
    static std::uint32_t Player(std::uint32_t i){return manager+0x1000u+i*0x1000u;}
    PlayerSlot& Slot(std::uint32_t base) {
        Require(base>=Player(0) && base<=Player(3) && (base-Player(0))%0x1000==0,"invalid equipment player handle");
        return players.slots_[(base-Player(0))/0x1000];
    }
    const EquipmentInstance& Instance(std::uint32_t token) const {
        Require(token>=records && (token-records)%0x1000==0 && (token-records)/0x1000<instances.size(),"invalid equipment instance handle");
        return instances[(token-records)/0x1000];
    }
    const EquipmentDefinition& Definition(const EquipmentInstance& instance) const{return instance.snapshot;}
    std::uint32_t PlayerManager() override{return manager;}
    std::uint32_t PlayerByIndex(std::uint32_t owner,std::int32_t index) override {
        Require(owner==manager,"invalid equipment manager");return index>=0 && index<4?Player(static_cast<std::uint32_t>(index)):0;
    }
    std::uint8_t IsEquipment(std::uint32_t table,std::uint32_t id) override {
        Require(table==database+0x58,"invalid equipment table");return catalog.definitions.contains(id);
    }
    std::uint32_t ProcessInventoryCollection(std::uint32_t collection,std::uint32_t id) override {
        Require(collection==inventory+0x7c,"invalid equipment collection");
        const auto& definition=catalog.definitions.at(id);
        // 非零技能需要接入技能批处理，不能当作只有数值的装备放行。
        Require(!definition.skill,"equipment skill processing is not bound");
        Require(instances.size()<100000,"equipment instance limit exceeded");
        const auto sequence=static_cast<std::uint32_t>(instances.size()+1);
        instances.push_back({sequence,id,definition});acquired.emplace(id,sequence);
        if(recent.size()==6)recent.erase(recent.begin());
        recent.push_back(sequence);
        return records+(sequence-1)*0x1000;
    }
    pal4::InventoryItemSlotsState ReadEquipmentSlots(std::uint32_t address) override {return {Slot(address-0x920).equipment};}
    void WriteWord(std::uint32_t address,std::uint32_t value) override {
        for(unsigned i=0;i<4;++i)if(address>=Player(i)+0x920 && address<Player(i)+0x938 && (address-Player(i)-0x920)%4==0){Slot(Player(i)).equipment[(address-Player(i)-0x920)/4]=value;return;}
        throw std::logic_error("unbound equipment slot write");
    }
    std::uint8_t RefreshEquipmentState(std::uint32_t script) override {
        auto& p=Slot(script-0x654).progress;const auto& slots=Slot(script-0x654).equipment;
        // 43F4B0: 基础属性复制后按六槽顺序叠加，保留并重新钳制HP/MP/怒气。
        auto effective=Encode(p.base);
        for(const auto sequence:slots)if(sequence) {
            const auto found=std::find_if(instances.begin(),instances.end(),[&](auto item){return item.sequence==sequence;});
            Require(found!=instances.end(),"equipment slot references missing instance");
            const auto& definition=Definition(*found);Require(!definition.skill,"equipment skill processing is not bound");
            (void)effective.AddFrom(Encode(definition.stats),1,2);
        }
        p.effective=Decode(effective);p.hp=Clamp(p.hp,p.effective.core[0]);p.mp=Clamp(p.mp,p.effective.core[2]);p.rage=Clamp(p.rage,100);p.dead=p.hp<=0;
        ++refreshes;return static_cast<std::uint8_t>(p.effective.core[2]);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        const auto offset=(address-records)%0x1000;const auto token=address-offset;const auto& instance=Instance(token);
        if(offset==0x94)return Definition(instance).type;
        if(offset==0x190)return instance.sequence;
        throw std::logic_error("unbound equipment record read");
    }
    std::uint32_t GetGameDatabase() override{return database;}
    std::uint32_t ProcessGameDatabaseArray(std::uint32_t table,std::uint32_t type) override {
        Require(table==database+0x58 && type<catalog.classes.size(),"invalid equipment type lookup");return catalog.classes[type];
    }
    std::uint32_t GetInventory() override{return inventory;}
    std::uint32_t FindInventoryEntry(std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("unbound inventory entry lookup");}
    std::uint32_t ProcessEntry(std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("unbound inventory entry processing");}
    void InsertEntry(std::uint32_t,std::uint32_t) override{throw std::logic_error("unbound inventory result insertion");}
};
PlayerEquipment::PlayerEquipment(PlayerRoster& players,const EquipmentCatalog& catalog):impl_(std::make_unique<Impl>(players,catalog)){}
PlayerEquipment::~PlayerEquipment()=default;
void PlayerEquipment::Add(std::int32_t player,std::uint32_t equipment) {
    Require(impl_->players.Prepared(),"equipment requires prepared players");++impl_->calls;
    (void)pal4::ScriptPlayerEquipment::Add(player,equipment,*impl_);
}
const std::vector<EquipmentInstance>& PlayerEquipment::Instances() const{return impl_->instances;}
void PlayerEquipment::WriteReport(std::ostream& output) const {
    output<<"{\"calls\":"<<impl_->calls<<",\"refreshes\":"<<impl_->refreshes<<",\"instances\":[";
    for(std::size_t i=0;i<impl_->instances.size();++i){if(i)output<<',';const auto& item=impl_->instances[i];output<<"{\"sequence\":"<<item.sequence<<",\"definition\":"<<item.definition<<'}';}
    output<<"],\"recent\":[";for(std::size_t i=0;i<impl_->recent.size();++i){if(i)output<<',';output<<impl_->recent[i];}
    output<<"],\"players\":";WritePlayerReport(output,impl_->players);output<<'}';
}
}
