#pragma once

#include <cstdint>

#include "pal4/base_object_lifecycle.h"

namespace pal4 {

class IBaseObjectEffectListRuntime {
public:
    virtual ~IBaseObjectEffectListRuntime() = default;
    virtual void AddOrReplaceComponent(BaseObjectState&, std::uint32_t,
                                       std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t QueryComponent(
        BaseObjectState&, std::uint32_t) { return 0u; }
    virtual void RemoveById(BaseObjectState&, std::uint32_t,
                            std::uint32_t) {}
};

class BaseObjectEffectList final {
public:
    // IDA 0x444280 and 0x4442a0.
    [[nodiscard]] static std::uint32_t UpdateFirst(
        BaseObjectState& state, IBaseObjectEffectListRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateFirstResolved(
        BaseObjectState& state, IBaseObjectEffectListRuntime& runtime);
    // IDA 0x4442d0 and 0x4442f0.
    [[nodiscard]] static std::uint32_t UpdateSecond(
        BaseObjectState& state, IBaseObjectEffectListRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateSecondResolved(
        BaseObjectState& state, IBaseObjectEffectListRuntime& runtime);
};

}  // namespace pal4
