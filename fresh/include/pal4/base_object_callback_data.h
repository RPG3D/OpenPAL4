#pragma once

#include <array>
#include <cstdint>

#include "pal4/base_object_motion.h"

namespace pal4 {

struct BaseObjectCallbackRecord {
    static constexpr std::uint32_t kSize = 0x64cu;
    std::array<std::uint32_t, kSize / 4u> words{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const {
        return words[offset / 4u];
    }
    void Write(std::uint32_t offset, std::uint32_t value) {
        words[offset / 4u] = value;
    }
};

class IBaseObjectCollisionRuntime {
public:
    virtual ~IBaseObjectCollisionRuntime() = default;
    [[nodiscard]] virtual bool ProcessCollision(
        const BaseObjectVector3&, const BaseObjectVector3&,
        std::uint32_t) { return false; }
};

class BaseObjectCallbackData final {
public:
    // IDA 0x444320; returns the original target address.
    [[nodiscard]] static std::uint32_t SetProperty1544(
        BaseObjectState& state, std::uint32_t target_address,
        std::uint32_t property);
    // IDA 0x444330; returns the original target address.
    [[nodiscard]] static std::uint32_t SetPositionAndTransformData(
        BaseObjectCallbackRecord& record, std::uint32_t target_address,
        const BaseObjectVector3& first,
        const BaseObjectVector3& second);
    // IDA 0x444390.
    [[nodiscard]] static bool CheckCollision(
        const BaseObjectVector3& first,
        const BaseObjectVector3& second, std::uint32_t argument,
        IBaseObjectCollisionRuntime& runtime);
};

}  // namespace pal4
