#include "monster_component_source.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace pal4::portable::detail {
namespace {void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}}
struct MonsterComponentSource::Impl {
    ObjectMemory& storage;const SceneMonster& record;std::vector<std::uint32_t> owned;
    std::uint32_t source{},document{},cache{},child{},values{};std::array<std::uint32_t,20> image{};bool closed{};
    std::uint32_t Allocate(std::uint32_t bytes){const auto id=storage.Allocate(bytes);try{owned.push_back(id);}catch(...){(void)storage.Free(id);throw;}return id;}
    Impl(ObjectMemory& heap,const SceneMonster& input):storage(heap),record(input){
        Require(!record.name.empty() && !record.resource.empty() && record.fields[4]==record.values.size() && record.values.size()<=std::numeric_limits<std::uint32_t>::max()/4,"invalid monster component source record");
        for(float value:record.position)Require(std::isfinite(value),"invalid monster source position");
        for(float value:record.rotation)Require(std::isfinite(value),"invalid monster source rotation");
        try{
            source=Allocate(0x50);document=Allocate(60);cache=Allocate(60);
            if(!record.behavior.Root().children.empty())child=Allocate(60);
            if(!record.values.empty()){values=Allocate(static_cast<std::uint32_t>(record.values.size()*4));for(std::size_t i=0;i<record.values.size();++i)storage.WriteDword(values+static_cast<std::uint32_t>(i)*4,record.values[i]);}
            for(unsigned a=0;a<3;++a){image[6+a]=std::bit_cast<std::uint32_t>(record.position[a]);image[9+a]=std::bit_cast<std::uint32_t>(record.rotation[a]);}
            for(unsigned i=0;i<record.fields.size();++i)image[12+i]=record.fields[i];
            image[17]=values;image[18]=document;image[19]=cache;
            for(unsigned i=0;i<image.size();++i)storage.WriteDword(source+i*4,image[i]);
        }catch(...){Release();throw;}
    }
    void Validate() const {
        Require(!closed,"closed monster component source");
        for(unsigned i=0;i<image.size();++i)Require(storage.ReadDword(source+i*4)==image[i],"monster component source image was modified");
        for(std::size_t i=0;i<record.values.size();++i)Require(storage.ReadDword(values+static_cast<std::uint32_t>(i)*4)==record.values[i],"monster component source values were modified");
    }
    void Release() noexcept {for(auto it=owned.rbegin();it!=owned.rend();++it){try{(void)storage.Free(*it);}catch(...){}}owned.clear();}
    ~Impl(){Release();}
};
MonsterComponentSource::MonsterComponentSource(ObjectMemory& memory,const SceneMonster& record):impl_(std::make_unique<Impl>(memory,record)){}
MonsterComponentSource::~MonsterComponentSource()=default;
std::uint32_t MonsterComponentSource::Token() const{impl_->Validate();return impl_->source;}
std::uint32_t MonsterComponentSource::ChildToken() const{impl_->Validate();return impl_->child;}
std::uint32_t MonsterComponentSource::Document(std::uint32_t source) const{impl_->Validate();Require(source==impl_->source,"foreign monster component source");return impl_->document;}
std::uint32_t MonsterComponentSource::FirstChild(std::uint32_t document,std::uint32_t index) const{impl_->Validate();Require(document==impl_->document && index==0,"foreign monster component document or child");return impl_->child;}
const SceneMonster& MonsterComponentSource::Record() const{impl_->Validate();return impl_->record;}
void MonsterComponentSource::Close(){if(impl_->closed){return;}while(!impl_->owned.empty()){(void)impl_->storage.Free(impl_->owned.back());impl_->owned.pop_back();}impl_->closed=true;}
}
