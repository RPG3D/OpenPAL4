#include "pal4/portable/monster_encounter_storage.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace pal4::portable {
namespace {
std::size_t Visible(const MonsterBehaviorManagerState& state,std::uint32_t offset,std::uint32_t address,std::uint32_t count,std::uint32_t capacity,std::uint32_t stride){
    const auto begin=state.words[offset/4],end=state.words[offset/4+1],limit=state.words[offset/4+2];
    if(begin!=address || limit!=address+capacity*stride || end<begin || (end-begin)%stride || (end-begin)/stride>count)
        throw std::logic_error("monster encounter array ownership changed");
    return (end-begin)/stride;
}
}
MonsterEncounterStorage::MonsterEncounterStorage(MonsterBehaviorManagerState& state,ObjectMemory& memory):state_(state),memory_(memory){
    for(auto offset:{0x714u,0x724u})for(unsigned i=0;i<3;++i)if(state_.words[offset/4+i])throw std::logic_error("monster encounter array already owned");
}
MonsterEncounterStorage::~MonsterEncounterStorage(){try{Close();}catch(...) {}}
void MonsterEncounterStorage::Publish(std::uint32_t offset,std::uint32_t address,std::uint32_t count,std::uint32_t capacity,std::uint32_t stride){
    state_.words[offset/4]=address;state_.words[offset/4+1]=address+count*stride;state_.words[offset/4+2]=address+capacity*stride;
}
void MonsterEncounterStorage::SyncRecords(){if(closed_)throw std::logic_error("monster encounter storage closed");records_.resize(Visible(state_,0x714,records_address_,static_cast<std::uint32_t>(records_.size()),records_capacity_,12));}
void MonsterEncounterStorage::SyncNames(){
    if(closed_)throw std::logic_error("monster encounter storage closed");
    const auto count=Visible(state_,0x724,names_address_,static_cast<std::uint32_t>(names_.size()),names_capacity_,16);
    for(auto i=count;i<names_.size();++i)if(names_[i].token)(void)memory_.Free(names_[i].token);
    names_.resize(count);
}
void MonsterEncounterStorage::Reserve(std::uint32_t& address,std::uint32_t& capacity,std::uint32_t stride,std::uint32_t count){
    if(count<=capacity)return;
    const auto next=capacity?std::min<std::uint64_t>(std::uint64_t(capacity)*2,std::numeric_limits<std::uint32_t>::max()/stride):1;
    if(count>next)throw std::length_error("monster encounter array too large");
    const auto allocation=memory_.Allocate(static_cast<std::uint32_t>(next)*stride);
    try{for(std::uint32_t i=0;i<capacity*stride;i+=4)memory_.WriteDword(allocation+i,memory_.ReadDword(address+i));}
    catch(...){(void)memory_.Free(allocation);throw;}
    if(address)(void)memory_.Free(address);
    address=allocation;capacity=static_cast<std::uint32_t>(next);
}
void MonsterEncounterStorage::Append(const std::array<std::uint32_t,3>& record){
    SyncRecords();if(records_.size()>=std::numeric_limits<std::uint32_t>::max()/12)throw std::length_error("too many monster combat records");
    records_.push_back(record);
    try{Reserve(records_address_,records_capacity_,12,static_cast<std::uint32_t>(records_.size()));}
    catch(...){records_.pop_back();throw;}
    const auto address=records_address_+static_cast<std::uint32_t>(records_.size()-1)*12;
    for(unsigned i=0;i<3;++i)memory_.WriteDword(address+i*4,record[i]);
    Publish(0x714,records_address_,static_cast<std::uint32_t>(records_.size()),records_capacity_,12);
}
void MonsterEncounterStorage::AddName(std::string_view name){
    if(name.size()>=std::numeric_limits<std::uint32_t>::max())throw std::length_error("monster encounter name too long");
    const std::string copy(name);SyncNames();
    for(const auto& entry:names_)if(entry.token && Name(entry.token)==copy)return;
    if(names_.size()>=std::numeric_limits<std::uint32_t>::max()/16)throw std::length_error("too many monster encounter names");
    const auto token=memory_.Allocate(static_cast<std::uint32_t>(copy.size()+1));
    try{
        for(std::size_t i=0;i<copy.size();++i)memory_.WriteByte(token+static_cast<std::uint32_t>(i),static_cast<std::uint8_t>(copy[i]));
        names_.push_back({token,static_cast<std::uint32_t>(copy.size())});
        try{Reserve(names_address_,names_capacity_,16,static_cast<std::uint32_t>(names_.size()));}catch(...){names_.pop_back();throw;}
    }catch(...){(void)memory_.Free(token);throw;}
    const auto address=names_address_+static_cast<std::uint32_t>(names_.size()-1)*16;
    memory_.WriteDword(address,0);memory_.WriteDword(address+4,token);memory_.WriteDword(address+8,static_cast<std::uint32_t>(copy.size()));memory_.WriteDword(address+12,static_cast<std::uint32_t>(copy.size()));
    Publish(0x724,names_address_,static_cast<std::uint32_t>(names_.size()),names_capacity_,16);
}
bool MonsterEncounterStorage::ReleaseHistoryRecord(std::uint32_t record){
    if(closed_ || !names_address_ || record<names_address_ || (record-names_address_)%16)return false;
    const auto index=(record-names_address_)/16;if(index>=names_.size())return false;
    auto& entry=names_[index];if(entry.token)(void)memory_.Free(entry.token);entry={};
    memory_.WriteDword(record+4,0);memory_.WriteDword(record+8,0);memory_.WriteDword(record+12,0);return true;
}
std::string MonsterEncounterStorage::Name(std::uint32_t token) const{
    const auto found=std::find_if(names_.begin(),names_.end(),[&](const auto& entry){return entry.token && entry.token==token;});
    if(found==names_.end())throw std::invalid_argument("unknown monster encounter name");
    std::string result(found->length,'\0');for(std::uint32_t i=0;i<found->length;++i)result[i]=static_cast<char>(memory_.ReadByte(token+i));return result;
}
std::vector<std::array<std::uint32_t,3>> MonsterEncounterStorage::Records(){SyncRecords();return records_;}
std::vector<std::string> MonsterEncounterStorage::Names(){SyncNames();std::vector<std::string> result;for(const auto& entry:names_)if(entry.token)result.push_back(Name(entry.token));return result;}
std::uint32_t MonsterEncounterStorage::ReadWord(std::uint32_t address){
    SyncRecords();SyncNames();
    const auto in_range=[&](std::uint32_t begin,std::size_t count,std::uint32_t stride){return begin && address>=begin && std::uint64_t(address)+4<=std::uint64_t(begin)+count*stride && address%4==0;};
    if(!in_range(records_address_,records_.size(),12) && !in_range(names_address_,names_.size(),16))throw std::invalid_argument("foreign monster encounter word");
    return memory_.ReadDword(address);
}
void MonsterEncounterStorage::ConsumeNames(){
    SyncNames();for(std::size_t i=0;i<names_.size();++i)(void)ReleaseHistoryRecord(names_address_+static_cast<std::uint32_t>(i)*16);
    state_.words[0x728/4]=names_address_;
}
void MonsterEncounterStorage::Close(){
    if(closed_)return;
    SyncRecords();SyncNames();
    for(const auto& entry:names_)if(entry.token)(void)memory_.Free(entry.token);
    if(names_address_)(void)memory_.Free(names_address_);
    if(records_address_)(void)memory_.Free(records_address_);
    Publish(0x714,0,0,0,12);Publish(0x724,0,0,0,16);records_.clear();names_.clear();records_address_=records_capacity_=names_address_=names_capacity_=0;closed_=true;
}
}
