#include "pal4/portable/borrowed_actor_base_members.h"
#include <stdexcept>
#include <map>
namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}
[[noreturn]] void DynamicResource(){throw std::logic_error("borrowed actor base does not own dynamic resource data");}
void EmptyRecords(const GameObjectEmbeddedAnimationState& state){Require(!state.words[21] && !state.words[22] && !state.words[23],"borrowed actor animation script records were not returned");}
// 原构造的集合为空；实际动作/事件由ActorPlayback的标准容器拥有。
// 只处理原空成员生命周期，拒绝将播放器局部token当作存储地址释放。
struct AnimationLifetime final:IGameObjectEmbeddedAnimationRuntime {
    void InitializeAnimationData(GameObjectEmbeddedAnimationState& state) override {state.words[21]=state.words[22]=state.words[23]=0;}
    void CleanupAnimationData(GameObjectEmbeddedAnimationState& state) override {EmptyRecords(state);}
    void CleanupAnimationDataRange(GameObjectEmbeddedAnimationState& state) override {EmptyRecords(state);}
    std::uint32_t AnimationArrayCount(std::uint32_t) override {DynamicResource();}
    void DestructAnimationArray(std::uint32_t,std::uint32_t,std::uint32_t) override {DynamicResource();}
    std::uint32_t FreeMemory(std::uint32_t address) override {Require(!address,"borrowed actor animation allocation was not returned");return 0;}
    void FreeWrappedMemory(std::uint32_t) override {DynamicResource();}
    std::uint32_t FindHierarchy(std::uint32_t) override {DynamicResource();}
    void ProcessAnimationData(GameObjectEmbeddedAnimationState&,std::uint32_t,std::int32_t) override {DynamicResource();}
};
struct EmptySchema final:IRwDictSchemaRuntime {
    std::uint32_t SetCurrentDict(std::uint32_t dict) override {Require(!dict,"borrowed actor UV dictionary is not empty");return 0;}
    std::uint32_t ReadWord(std::uint32_t) override {DynamicResource();}
    std::uint32_t ListBegin(std::uint32_t) override {DynamicResource();}
    std::uint32_t ListEnd(std::uint32_t) override {DynamicResource();}
    void AdvanceUVAnim(std::uint32_t,float) override {DynamicResource();}
    void ApplyUVAnim(std::uint32_t) override {DynamicResource();}
    void DestroyList(std::uint32_t) override {DynamicResource();}
    void DestroyDict(std::uint32_t) override {DynamicResource();}
    std::uint32_t OpenStream(std::uint32_t,std::uint32_t,const char*) override {DynamicResource();}
    bool FindChunk(std::uint32_t,std::uint32_t) override {DynamicResource();}
    std::uint32_t ReadDict(std::uint32_t) override {DynamicResource();}
    void CloseStream(std::uint32_t) override {DynamicResource();}
    std::uint32_t CreateList(std::uint32_t,std::uint32_t) override {DynamicResource();}
    using Callback=std::uint32_t(*)(std::uint32_t,std::uint32_t,IRwDictSchemaRuntime&);
    std::uint32_t ForAllAtomics(std::uint32_t,Callback,std::uint32_t) override {DynamicResource();}
    std::uint32_t ForAllWorldMaterials(std::uint32_t,Callback,std::uint32_t) override {DynamicResource();}
    std::uint32_t ForAllGeometryMaterials(std::uint32_t,Callback,std::uint32_t) override {DynamicResource();}
    bool HasUVAnim(std::uint32_t) override {DynamicResource();}
    std::uint32_t ListAppend(std::uint32_t) override {DynamicResource();}
    void WriteWord(std::uint32_t,std::uint32_t) override {DynamicResource();}
};
}
struct BorrowedActorBaseMembers::Impl {
    BaseObjectMemberRuntime::AddressResolver address;AnimationLifetime animation;EmptySchema schema;BaseObjectMemberRuntime members;
    std::map<BaseObjectState*,std::uint32_t> live;
    Impl(ObjectMemory& memory,BaseObjectMemberRuntime::AddressResolver resolve):address(std::move(resolve)),members(memory,schema,animation,memory,memory.Path(),address){}
    void ValidateIdentity(BaseObjectState& state,std::uint32_t token){Require(token && token%4==0 && token<=0xffffffffu-BaseObjectState::kSize && address(state)==token,"invalid borrowed actor base identity");}
};
BorrowedActorBaseMembers::BorrowedActorBaseMembers(ObjectMemory& memory,BaseObjectMemberRuntime::AddressResolver address):impl_(std::make_unique<Impl>(memory,std::move(address))){}
BorrowedActorBaseMembers::~BorrowedActorBaseMembers()=default;
std::uint32_t BorrowedActorBaseMembers::Construct(BaseObjectState& state,std::uint32_t token){
    impl_->ValidateIdentity(state,token);Require(!impl_->live.contains(&state),"borrowed actor base is already constructed");
    for(const auto& entry:impl_->live)Require(token>=entry.second+BaseObjectState::kSize || entry.second>=token+BaseObjectState::kSize,"overlapping borrowed actor base identity");
    impl_->live.emplace(&state,token);
    try{return BaseObjectLifecycle::Construct(state,token,impl_->members);}catch(...){impl_->live.erase(&state);throw;}
}
void BorrowedActorBaseMembers::ValidateDestroy(BaseObjectState& state){
    const auto entry=impl_->live.find(&state);Require(entry!=impl_->live.end(),"borrowed actor base is not constructed");impl_->ValidateIdentity(state,entry->second);
    for(unsigned offset:{0u,0x218u,0x388u}){
        Require(!state.Read(offset+37*4) && !state.Read(offset+38*4),"borrowed actor UV resources were not returned");
        Require(!state.Read(offset+11*4) && !state.Read(offset+12*4) && !state.Read(offset+13*4),"borrowed actor shadow resources were not returned");
    }
    for(unsigned field:{1u,2u,3u,4u,5u,7u,8u,9u,10u,11u,21u,22u,23u,24u,25u,26u,27u,28u,29u,30u})
        Require(!state.Read(0x170+field*4),"borrowed actor animation data was not returned");
}
void BorrowedActorBaseMembers::Destroy(BaseObjectState& state){
    ValidateDestroy(state);BaseObjectLifecycle::Destroy(state,impl_->members);impl_->live.erase(&state);
}
}
