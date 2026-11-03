#pragma once

#include "pal4/base_object_lifecycle.h"
#include "pal4/game_object_embedded_animation.h"
#include "pal4/game_resource_manager.h"
#include "pal4/collision_actor.h"
#include "pal4/string_buffer.h"

#include <functional>
#include <stdexcept>
#include <utility>

namespace pal4 {

// 绑定原基础对象成员；地址解析器返回稳定的32位逻辑身份，不截断宿主指针。
// 所有依赖必须比适配器及其已构造成员存活更久；调用方负责生命周期。
class BaseObjectMemberRuntime : public IBaseObjectRuntime {
public:
    using AddressResolver=std::function<std::uint32_t(BaseObjectState&)>;
    BaseObjectMemberRuntime(IGameConfigMemory& memory,
                                  IRwDictSchemaRuntime& schema,
                                  IGameObjectEmbeddedAnimationRuntime& animation,
                                  IStringBufferStorageRuntime& text,
                                  ID3dFacePathRuntime& path, AddressResolver address)
        : memory_(memory), schema_(schema), animation_(animation),
          text_(text), path_(path), address_(std::move(address)) {
        if (!address_) throw std::invalid_argument("base object address resolver is missing");
    }

    void ConstructResource(BaseObjectState& state, std::uint32_t offset) override;
    void DestroyResource(BaseObjectState& state, std::uint32_t offset) override;
    void InitializeAnimation(BaseObjectState& state, std::uint32_t offset) override;
    void CleanupAnimation(BaseObjectState& state, std::uint32_t offset) override;
    void ConstructEffect(BaseObjectState& state, std::uint32_t offset) override;
    void ResetEffect(BaseObjectState& state, std::uint32_t offset) override;
    void ConstructText(BaseObjectState& state, std::uint32_t offset) override;
    void CleanupText(BaseObjectState& state, std::uint32_t offset) override;
    void ConstructCharacter(BaseObjectState& state, std::uint32_t offset) override;
    void DestroyCharacter(BaseObjectState& state, std::uint32_t offset) override;

private:
    std::uint32_t Address(BaseObjectState&,std::uint32_t offset) const;
    IGameConfigMemory& memory_;
    IRwDictSchemaRuntime& schema_;
    IGameObjectEmbeddedAnimationRuntime& animation_;
    IStringBufferStorageRuntime& text_;
    ID3dFacePathRuntime& path_;
    AddressResolver address_;
};

}  // namespace pal4
