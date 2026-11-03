#include "pal4/base_object_member_runtime.h"

#include "pal4/dname_node_event_record.h"
#include "pal4/effect_handle.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace pal4 {
namespace {

template <typename T>
void LoadWords(T& member, const BaseObjectState& state,
               const std::uint32_t offset) {
    if (offset % 4u != 0u || offset > BaseObjectState::kSize - sizeof(member))
        throw std::invalid_argument("PAL4 base member offset");
    std::copy_n(state.words.begin() + offset / 4u,
                sizeof(member) / 4u, member.words.begin());
}

template <typename T>
void StoreWords(BaseObjectState& state, const std::uint32_t offset,
                const T& member) {
    std::copy(member.words.begin(), member.words.end(),
              state.words.begin() + offset / 4u);
}

class NameWriter final : public IDNameNodeRuntime {
public:
    explicit NameWriter(BaseObjectState& state) : state_(state) {}
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        state_.Write(address, value);
    }
private:
    BaseObjectState& state_;
};

// 字符串结构仍在调用方图像内，分配的数据交给实际存储服务。
class TextView final:public IStringBufferStorageRuntime {
public:
    TextView(BaseObjectState& state,std::uint32_t base,IStringBufferStorageRuntime& storage)
        :state_(state),base_(base),storage_(storage){}
    std::uint32_t Allocate(std::uint32_t size) override{return storage_.Allocate(size);}
    std::uint32_t Free(std::uint32_t address) override{return storage_.Free(address);}
    std::uint32_t ReadDword(std::uint32_t address) override {
        return InImage(address,4)?state_.Read(address-base_):storage_.ReadDword(address);
    }
    void WriteDword(std::uint32_t address,std::uint32_t value) override {
        if(InImage(address,4))state_.Write(address-base_,value);else storage_.WriteDword(address,value);
    }
    void WriteByte(std::uint32_t address,std::uint8_t value) override {
        if(!InImage(address,1)){storage_.WriteByte(address,value);return;}
        const auto offset=address-base_,aligned=offset&~3u,shift=(offset&3u)*8u;
        state_.Write(aligned,(state_.Read(aligned)&~(0xffu<<shift))|(std::uint32_t(value)<<shift));
    }
private:
    bool InImage(std::uint32_t address,std::uint32_t bytes) const {
        const bool inside=address>=base_ && address-base_<BaseObjectState::kSize;
        if(inside && (address-base_>BaseObjectState::kSize-bytes || (bytes==4 && address%4)))
            throw std::invalid_argument("invalid base object text image access");
        return inside;
    }
    BaseObjectState& state_;std::uint32_t base_;IStringBufferStorageRuntime& storage_;
};
}  // namespace

std::uint32_t BaseObjectMemberRuntime::Address(BaseObjectState& state,std::uint32_t offset) const {
    const auto base=address_(state);
    if(!base || base%4 || base>0xffffffffu-BaseObjectState::kSize || offset>=BaseObjectState::kSize)
        throw std::invalid_argument("invalid base object identity or member address");
    return base+offset;
}

void BaseObjectMemberRuntime::ConstructResource(
    BaseObjectState& state, const std::uint32_t offset) {
    GameResourceManager manager;
    LoadWords(manager, state, offset);
    (void)manager.Construct();
    StoreWords(state, offset, manager);
}

void BaseObjectMemberRuntime::DestroyResource(
    BaseObjectState& state, const std::uint32_t offset) {
    GameResourceManager manager;
    LoadWords(manager, state, offset);
    manager.Destruct(memory_, schema_);
    StoreWords(state, offset, manager);
}

void BaseObjectMemberRuntime::InitializeAnimation(
    BaseObjectState& state, const std::uint32_t offset) {
    GameObjectEmbeddedAnimationState animation;
    LoadWords(animation, state, offset);
    (void)GameObjectEmbeddedAnimation::InitializeState(
        animation, 0u, animation_);
    const auto manager_address = Address(state, offset + 80u);
    animation.words[20u] = (animation.words[20u] & 0xffffff00u) |
                           ((manager_address >> 8u) & 0xffu);
    animation.words[21u] = animation.words[22u] = animation.words[23u] = 0u;
    StoreWords(state, offset, animation);
}

void BaseObjectMemberRuntime::CleanupAnimation(
    BaseObjectState& state, const std::uint32_t offset) {
    GameObjectEmbeddedAnimationState animation;
    LoadWords(animation, state, offset);
    (void)GameObjectEmbeddedAnimation::CleanupState(animation, animation_);
    StoreWords(state, offset, animation);
}

void BaseObjectMemberRuntime::ConstructEffect(
    BaseObjectState& state, const std::uint32_t offset) {
    if (offset > BaseObjectState::kSize - 8u)
        throw std::invalid_argument("PAL4 base handle offset");
    NameWriter writer(state);
    (void)DNameNode::Construct(offset, writer);
}

void BaseObjectMemberRuntime::ResetEffect(
    BaseObjectState& state, const std::uint32_t offset) {
    if (offset > BaseObjectState::kSize - 8u)
        throw std::invalid_argument("PAL4 base handle offset");
    EffectSystemHandleWords handle{state.Read(offset), state.Read(offset + 4u)};
    EffectHandle::Reset(handle);
    state.Write(offset, handle.id);
    state.Write(offset + 4u, handle.system);
}

void BaseObjectMemberRuntime::ConstructText(
    BaseObjectState& state, const std::uint32_t offset) {
    if(offset%4 || offset>BaseObjectState::kSize-12)
        throw std::invalid_argument("invalid base object text member offset");
    TextView text(state,Address(state,0),text_);
    (void)StringBuffer::InitializeEmptyAt(Address(state, offset), text);
}
void BaseObjectMemberRuntime::CleanupText(
    BaseObjectState& state, const std::uint32_t offset) {
    if(offset%4 || offset>BaseObjectState::kSize-12)
        throw std::invalid_argument("invalid base object text member offset");
    TextView text(state,Address(state,0),text_);
    (void)StringBuffer::CleanupAt(Address(state, offset), text);
}
void BaseObjectMemberRuntime::ConstructCharacter(
    BaseObjectState& state, const std::uint32_t offset) {
    CollisionActorState character;
    LoadWords(character, state, offset);
    (void)CollisionActor::Construct(character, 0u, path_);
    StoreWords(state, offset, character);
}
void BaseObjectMemberRuntime::DestroyCharacter(
    BaseObjectState& state, const std::uint32_t offset) {
    CollisionActorState character;
    LoadWords(character, state, offset);
    (void)CollisionActor::Destroy(character, path_);
    StoreWords(state, offset, character);
}

}  // namespace pal4
