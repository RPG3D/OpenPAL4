#pragma once

#include <cstdint>
#include <utility>

#include "pal4/base_object_motion.h"

namespace pal4 {

class IBaseObjectPositionQueryRuntime {
public:
    virtual ~IBaseObjectPositionQueryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t MaterialCount(std::uint32_t) {
        return 0u;
    }
    [[nodiscard]] virtual std::pair<std::uint32_t, std::uint32_t>
    ProcessClumpCollision(const BaseObjectVector3&, std::uint32_t) {
        return {};
    }
};

class BaseObjectPositionQuery final {
public:
    // IDA 0x444230.
    [[nodiscard]] static std::uint32_t GetPositionAndData(
        const BaseObjectState& state, const BaseObjectVector3& position,
        std::uint32_t* output,
        IBaseObjectPositionQueryRuntime& runtime);
};

}  // namespace pal4
