#pragma once

#include <cstdint>

#include "pal4/base_object_lifecycle.h"

namespace pal4 {

class IBaseObjectAnimationSelectionRuntime {
public:
    virtual ~IBaseObjectAnimationSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CurrentAnimationName(
        BaseObjectState&) { return 0u; }
    [[nodiscard]] virtual std::int32_t CompareIgnoreCase(
        std::uint32_t, std::uint32_t) { return 0; }
    [[nodiscard]] virtual std::uint32_t SetCurrentAnimation(
        BaseObjectState&, std::uint32_t) { return 0u; }
    [[nodiscard]] virtual std::uint32_t SetCurrentAnimationWithFlag(
        BaseObjectState&, std::uint32_t, std::uint32_t) { return 0u; }
};

class BaseObjectAnimationSelection final {
public:
    // IDA 0x443ff0.
    [[nodiscard]] static std::uint32_t CompareAndSet(
        BaseObjectState& state, std::uint32_t requested_name,
        IBaseObjectAnimationSelectionRuntime& runtime);
    // IDA 0x444080.
    [[nodiscard]] static std::uint32_t CompareAndSetWithFlag(
        BaseObjectState& state, std::uint32_t requested_name,
        std::uint32_t flag,
        IBaseObjectAnimationSelectionRuntime& runtime);
};

}  // namespace pal4
