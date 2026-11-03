#pragma once

#include <cstdint>

#include "pal4/base_object_weapon.h"

namespace pal4 {

class IBaseObjectAttachedEffectRuntime {
public:
    virtual ~IBaseObjectAttachedEffectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t) { return 0u; }
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(std::uint32_t) {
        return 0u;
    }
    [[nodiscard]] virtual bool EffectNeedsUpdate(BaseObjectState&,
                                                  std::uint32_t) {
        return false;
    }
    virtual void UpdateEffect(BaseObjectState&, std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t EffectSingleton() { return 0u; }
    [[nodiscard]] virtual BaseObjectEffectCreation CreateEffectObject(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) { return {}; }
    virtual void ResetTemporary(BaseObjectEffectHandle&) {}
    virtual void SetVisibility(BaseObjectState&, std::uint32_t,
                               std::uint32_t) {}
    virtual void SetRotation(BaseObjectState&, std::uint32_t,
                             float, float, float, std::uint8_t) {}
    virtual void BindFrame(BaseObjectState&, std::uint32_t,
                           std::uint32_t) {}
    virtual void RefreshEffect(BaseObjectState&, std::uint32_t) {}
};

class BaseObjectAttachedEffect final {
public:
    // IDA 0x443920.
    [[nodiscard]] static std::uint32_t Setup(
        BaseObjectState& state, std::uint32_t effect_name,
        std::uint32_t mode, IBaseObjectAttachedEffectRuntime& runtime);
    // IDA 0x4439d0.
    [[nodiscard]] static std::uint32_t Update(
        BaseObjectState& state, IBaseObjectAttachedEffectRuntime& runtime);
    // IDA 0x4439f0.
    [[nodiscard]] static std::uint32_t SetupRotated(
        BaseObjectState& state, std::uint32_t effect_name,
        std::uint32_t mode, IBaseObjectAttachedEffectRuntime& runtime);
    // IDA 0x443ac0.
    [[nodiscard]] static std::uint32_t UpdateRotated(
        BaseObjectState& state, IBaseObjectAttachedEffectRuntime& runtime);
};

}  // namespace pal4
