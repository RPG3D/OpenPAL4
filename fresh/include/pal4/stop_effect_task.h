#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct StopEffectTaskState final {
    std::array<std::uint32_t, 23u> words{};
};
static_assert(sizeof(StopEffectTaskState) == 0x5cu);

class IStopEffectTaskRuntime {
public:
    virtual ~IStopEffectTaskRuntime() = default;
    virtual void ConstructBase(StopEffectTaskState& state) = 0;
    virtual void DestroyBase(StopEffectTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t owner, std::string_view class_name) = 0;
    [[nodiscard]] virtual float RemainingEffectTime(std::uint32_t facet) = 0;
    virtual void StopEffect(std::uint32_t facet) = 0;
    [[nodiscard]] virtual bool BaseDurationComplete(
        const StopEffectTaskState& state) = 0;
};

class IStopEffectConfigRuntime {
public:
    virtual ~IStopEffectConfigRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t config, std::string_view key,
        std::int32_t fallback) = 0;
};

class StopEffectTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84afa8u;
    static StopEffectTaskState& Construct(StopEffectTaskState& state,
                                          IStopEffectTaskRuntime& runtime);
    static StopEffectTaskState& Destroy(StopEffectTaskState& state,
                                        IStopEffectTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    [[nodiscard]] static bool LoadConfig(
        StopEffectTaskState& state, std::uint32_t config,
        IStopEffectConfigRuntime& runtime);
    [[nodiscard]] static bool IsDurationComplete(
        const StopEffectTaskState& state, IStopEffectTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Start(
        StopEffectTaskState& state, IStopEffectTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        StopEffectTaskState& state, float elapsed,
        IStopEffectTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Stop(
        StopEffectTaskState& state, IStopEffectTaskRuntime& runtime);
};

}  // namespace pal4
