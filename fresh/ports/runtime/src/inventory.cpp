#include "pal4/portable/inventory.h"
#include "pal4/inventory_subsystem.h"
#include "pal4/portable/gbk.h"
#include "pal4/inventory_item_changes.h"
#include <algorithm>
#include <array>
#include <istream>
#include <ostream>
#include <set>
#include <stdexcept>
#include <vector>
namespace pal4::portable {
namespace {
void Require(bool ok,const char* error){if(!ok)throw std::runtime_error(error);}
std::uint32_t Word(std::istream& in){unsigned char b[4]{};Require(bool(in.read(reinterpret_cast<char*>(b),4)),"truncated property catalog");return b[0]|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);}
}
PropertyCatalog ReadPropertyCatalog(std::istream& in){
    Require(Word(in)==0x49503450 && Word(in)==1,"unsupported property catalog");
    const auto size=Word(in);Require(size>0 && size<=4096,"invalid property count");PropertyCatalog result;
    for(std::uint32_t i=0;i<size;++i){PropertyDefinition p;p.id=Word(in);p.category=Word(in);const auto n=Word(in);
        Require(p.id && p.category<=4 && n>0 && n<=1024,"invalid property definition");
        std::string raw(n,'\0');Require(bool(in.read(raw.data(),n)) && raw.find('\0')==raw.npos,"invalid property name");p.name=DecodeGbk(raw);
        Require(result.emplace(p.id,p).second,"duplicate property ID");
    }
    Require(in.peek()==std::char_traits<char>::eof(),"trailing property catalog bytes");return result;
}
struct Inventory::Impl final:pal4::IInventoryItemChangesRuntime {
    const PropertyCatalog& catalog;std::vector<InventoryRecord> records;std::vector<std::uint32_t> recent;std::set<std::uint32_t> seen;std::uint32_t sequence{1};
    std::array<std::uint32_t,5> featured{};
    explicit Impl(const PropertyCatalog& c):catalog(c){for(const auto& [id,p]:c)Require(id && p.id==id && p.category<=4,"invalid inventory definition");}
    auto FindRecord(std::uint32_t id){return std::find_if(records.begin(),records.end(),[&](const auto& r){return r.id==id;});}
    InventoryRecord& Record(std::uint32_t id){const auto it=FindRecord(id);Require(it!=records.end(),"inventory record is missing");return *it;}
    std::uint32_t Definition(std::uint32_t id) override{return catalog.contains(id)?id:0;}
    std::uint32_t Slot(std::uint32_t id) override{return catalog.at(id).category+1;}
    void Observe(std::uint32_t id) override{seen.insert(id);}
    std::uint32_t Find(std::uint32_t slot,std::uint32_t id) override{const auto it=FindRecord(id);return it!=records.end() && it->category+1==slot?id:0;}
    std::uint32_t NextSequence() override{return sequence;}
    void NextSequence(std::uint32_t value) override{sequence=value;}
    std::uint32_t Append(std::uint32_t slot,std::uint32_t id,std::uint32_t key,std::uint32_t count) override{Require(catalog.at(id).category+1==slot && FindRecord(id)==records.end(),"inventory append mismatch");records.push_back({id,slot-1,key,count});return id;}
    std::uint32_t Count(std::uint32_t id) override{return Record(id).count;}
    void Count(std::uint32_t id,std::uint32_t value) override{Record(id).count=value;}
    std::uint32_t Sequence(std::uint32_t id) override{return Record(id).sequence;}
    void RemoveRecent(std::uint32_t key,bool trim) override{const auto index=pal4::InventoryRecentEraseIndex(recent.size(),[&](auto i){return recent[i];},key,trim);if(index)recent.erase(recent.begin()+static_cast<std::ptrdiff_t>(*index));}
    void AppendRecent(std::uint32_t key) override{recent.push_back(key);}
    void Erase(std::uint32_t slot,std::uint32_t key) override{const auto it=std::find_if(records.begin(),records.end(),[&](const auto& r){return r.category+1==slot && r.sequence==key;});Require(it!=records.end(),"inventory erase mismatch");records.erase(it);}
};
Inventory::Inventory(const PropertyCatalog& c):impl_(std::make_unique<Impl>(c)){}
Inventory::~Inventory()=default;
std::int32_t Inventory::Money()const{return money_;}
std::int32_t Inventory::AdjustMoney(std::int32_t delta){return money_=pal4::InventorySubsystem::UpdatedCounter(static_cast<std::uint32_t>(money_),delta);}
void Inventory::Add(std::uint32_t id,std::int32_t count){(void)pal4::InventoryItemChanges::Add(id,count,*impl_);}
void Inventory::Remove(std::uint32_t id,std::int32_t count){(void)pal4::InventoryItemChanges::Remove(id,count,*impl_);}
std::uint32_t Inventory::Count(std::uint32_t id) const{const auto it=impl_->FindRecord(id);return it==impl_->records.end()?0:it->count;}
bool Inventory::Seen(std::uint32_t id) const{return impl_->seen.contains(id);}
std::span<const InventoryRecord> Inventory::Records() const{return impl_->records;}
std::span<const std::uint32_t> Inventory::Recent() const{return impl_->recent;}
std::optional<InventoryRecord> Inventory::FindSequence(std::uint32_t sequence,std::uint32_t category)const{
    // Original FindBySequence treats every category >4 as an all-category scan.
    // Scan categories in original slot order, including zero-count records.
    for(unsigned slot=0;slot<5;++slot){
        if(category<=4 && category!=slot)continue;
        for(const auto& record:impl_->records)if(record.category==slot && record.sequence==sequence)return record;
    }
    return std::nullopt;
}
std::size_t Inventory::CategoryCount(std::uint32_t category)const{
    if(category>4)throw std::out_of_range("inventory UI category");
    return static_cast<std::size_t>(std::count_if(impl_->records.begin(),impl_->records.end(),[&](const auto& r){return r.category==category;}));
}
std::optional<InventoryRecord> Inventory::AtReverse(std::uint32_t category,std::size_t index)const{
    if(category>4)throw std::out_of_range("inventory UI category");
    for(auto i=impl_->records.rbegin();i!=impl_->records.rend();++i)if(i->category==category){if(index==0)return *i;--index;}
    return std::nullopt;
}
std::optional<InventoryRecord> Inventory::Featured(std::uint32_t category)const{
    return FindSequence(impl_->featured.at(category),category);
}
void Inventory::SetFeatured(std::uint32_t category,std::uint32_t sequence){impl_->featured.at(category)=sequence;}
void Inventory::WriteReport(std::ostream& out) const{
    out<<"{\"money\":"<<money_<<",\"next_sequence\":"<<impl_->sequence<<",\"items\":[";bool first=true;
    for(const auto& r:impl_->records){if(!first)out<<',';first=false;out<<"{\"id\":"<<r.id<<",\"category\":"<<r.category<<",\"sequence\":"<<r.sequence<<",\"count\":"<<r.count<<'}';}
    out<<"],\"recent\":[";first=true;for(auto key:impl_->recent){if(!first)out<<',';first=false;out<<key;}out<<"],\"seen\":[";first=true;for(auto id:impl_->seen){if(!first)out<<',';first=false;out<<id;}out<<"]}";
}
}
