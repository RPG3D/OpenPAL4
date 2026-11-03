#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct SlowMotionTaskState final {
    std::array<std::uint32_t, 20u> words{};
};
static_assert(sizeof(SlowMotionTaskState) == 0x50u);

class ISlowMotionTaskRuntime {
public:
    virtual ~ISlowMotionTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t owner, std::string_view class_name) = 0;
    [[nodiscard]] virtual std::int32_t ReadOwnerId(
        std::uint32_t owner, std::string_view key,
        std::int32_t fallback) = 0;
    virtual void ApplyScaleTime(std::uint32_t facet, std::int32_t id,
                                float scale_time) = 0;
};

class ISlowMotionConfigRuntime {
public:
    virtual ~ISlowMotionConfigRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t config, std::string_view key, float fallback) = 0;
};

class SlowMotionTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84b318u;
    [[nodiscard]] static std::uint8_t Start(
        SlowMotionTaskState& state, ISlowMotionTaskRuntime& runtime);
    [[nodiscard]] static bool LoadConfig(
        SlowMotionTaskState& state, std::uint32_t config,
        ISlowMotionConfigRuntime& runtime);
    [[nodiscard]] static std::uint32_t Reset(SlowMotionTaskState& state);
};

}  // namespace pal4
