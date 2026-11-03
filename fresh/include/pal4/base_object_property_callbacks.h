#pragma once

#include <array>
#include <cstdint>

#include "pal4/base_object_lifecycle.h"

namespace pal4 {

class IBaseObjectPropertyCallbacksRuntime {
public:
    virtual ~IBaseObjectPropertyCallbacksRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetProperty(
        BaseObjectState&, std::uint32_t) { return 0u; }
    virtual void RegisterCountCallback(BaseObjectState&, std::uint32_t) {}
    virtual void RegisterByIndexCallback(BaseObjectState&, std::uint32_t) {}
    virtual void RegisterByIdCallback(BaseObjectState&, std::uint32_t) {}
    virtual void RegisterActiveCallback(BaseObjectState&, std::uint32_t) {}
    [[nodiscard]] virtual std::array<std::uint32_t, 3> ConvertZeroToInt64() {
        return {};
    }
    [[nodiscard]] virtual std::uint32_t SetInitialProperty(
        BaseObjectState&, std::uint32_t, std::uint32_t,
        std::uint32_t, const std::array<std::uint32_t, 3>&,
        std::uint32_t, std::uint32_t) { return 0u; }
};

class BaseObjectPropertyCallbacks final {
public:
    // IDA 0x4441a0.
    [[nodiscard]] static std::uint32_t Initialize(
        BaseObjectState& state, std::uint32_t property_id,
        IBaseObjectPropertyCallbacksRuntime& runtime);
    // IDA 0x444220.
    [[nodiscard]] static std::uint32_t GetProperty(
        BaseObjectState& state,
        IBaseObjectPropertyCallbacksRuntime& runtime);
};

}  // namespace pal4
