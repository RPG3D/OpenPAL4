#include "pal4/portable/object_memory.h"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <vector>
namespace pal4::portable {
struct ObjectMemory::Impl {
    ObjectMemoryLimits limits;std::uint32_t next;std::size_t bytes{};
    std::map<std::uint32_t,std::vector<std::uint8_t>> blocks;
    struct PathRuntime final:ID3dFacePathRuntime {
        ObjectMemory& memory;explicit PathRuntime(ObjectMemory& m):memory(m){}
        std::uint32_t Allocate(std::uint32_t n) override{return memory.Allocate(n);}
        void Free(std::uint32_t a) override{(void)memory.Free(a);}
        std::uint32_t ReadWord(std::uint32_t a) override{return memory.ReadDword(a);}
        void WriteWord(std::uint32_t a,std::uint32_t v) override{memory.WriteDword(a,v);}
    } path;
    Impl(ObjectMemory& owner,ObjectMemoryLimits value):limits(value),next(value.first),path(owner){
        if(!limits.first || limits.first%4 || limits.limit%4 || limits.first>=limits.limit || !limits.bytes || !limits.allocations)
            throw std::invalid_argument("invalid object memory limits");
    }
    auto Find(std::uint32_t a,std::size_t n) const {
        auto i=blocks.upper_bound(a);
        if(i==blocks.begin())throw std::invalid_argument("unknown object memory address");
        --i;const auto offset=std::size_t(a-i->first);
        if(n>i->second.size() || offset>i->second.size()-n)throw std::invalid_argument("object memory access crosses allocation");
        return i;
    }
};
ObjectMemory::ObjectMemory(ObjectMemoryLimits limits):impl_(std::make_unique<Impl>(*this,limits)){}
ObjectMemory::~ObjectMemory()=default;
std::uint32_t ObjectMemory::Allocate(std::uint32_t requested){
    auto& s=*impl_;const auto size=std::max(requested,1u);
    const auto end=(std::uint64_t(s.next)+size+3u)&~std::uint64_t(3u);
    if(s.blocks.size()>=s.limits.allocations || size>s.limits.bytes-s.bytes || end>s.limits.limit)
        throw std::length_error("object memory allocation limit");
    const auto address=s.next;s.blocks.emplace(address,std::vector<std::uint8_t>(size));
    s.next=static_cast<std::uint32_t>(end);s.bytes+=size;return address;
}
std::uint32_t ObjectMemory::Free(std::uint32_t a){
    if(!a)return 0;
    auto& s=*impl_;const auto i=s.blocks.find(a);
    if(i==s.blocks.end())throw std::invalid_argument("foreign or released object allocation");
    s.bytes-=i->second.size();s.blocks.erase(i);return 1;
}
std::uint32_t ObjectMemory::ReadDword(std::uint32_t a){
    if(a%4)throw std::invalid_argument("unaligned object word read");
    const auto i=impl_->Find(a,4);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::uint32_t(i->second[a-i->first+n])<<(n*8);return v;
}
void ObjectMemory::WriteDword(std::uint32_t a,std::uint32_t v){
    if(a%4)throw std::invalid_argument("unaligned object word write");
    (void)impl_->Find(a,4);auto i=impl_->blocks.upper_bound(a);--i;
    for(unsigned n=0;n<4;++n)i->second[a-i->first+n]=std::uint8_t(v>>(n*8));
}
std::uint8_t ObjectMemory::ReadByte(std::uint32_t a) const {const auto i=impl_->Find(a,1);return i->second[a-i->first];}
void ObjectMemory::WriteByte(std::uint32_t a,std::uint8_t v){(void)impl_->Find(a,1);auto i=impl_->blocks.upper_bound(a);--i;i->second[a-i->first]=v;}
std::size_t ObjectMemory::AllocationCount() const{return impl_->blocks.size();}
std::size_t ObjectMemory::AllocatedBytes() const{return impl_->bytes;}
ID3dFacePathRuntime& ObjectMemory::Path(){return impl_->path;}
}
