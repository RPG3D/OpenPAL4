#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct ScaleTaskState final {
    std::array<std::uint32_t, 39u> words{};
};
static_assert(sizeof(ScaleTaskState) == 0x9cu);

class IScaleTaskRuntime {
public:
    virtual ~IScaleTaskRuntime() = default;
    virtual void ConstructBase(ScaleTaskState& state) = 0;
    virtual void DestroyBase(ScaleTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t owner, std::string_view class_name) = 0;
    virtual void ReadScale(std::uint32_t facet, std::array<float, 3u>& scale) = 0;
    virtual void SetScale(std::uint32_t facet,
                          const std::array<float, 3u>& scale,
                          std::uint32_t mode) = 0;
    [[nodiscard]] virtual bool BaseDurationComplete(
        const ScaleTaskState& state) = 0;
    [[nodiscard]] virtual std::int32_t RandomInt() = 0;
};

class IScaleTaskConfigRuntime {
public:
    virtual ~IScaleTaskConfigRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t config, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t config, std::string_view key, float fallback) = 0;
};

class ScaleTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84aea8u;

    static ScaleTaskState& Construct(ScaleTaskState& state,
                                     IScaleTaskRuntime& runtime);
    static ScaleTaskState& Destroy(ScaleTaskState& state,
                                   IScaleTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    [[nodiscard]] static bool LoadConfig(ScaleTaskState& state,
                                         std::uint32_t config,
                                         IScaleTaskConfigRuntime& runtime);
    [[nodiscard]] static bool IsDurationComplete(
        const ScaleTaskState& state, IScaleTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Start(ScaleTaskState& state,
                                             IScaleTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(ScaleTaskState& state,
                                              float elapsed,
                                              IScaleTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Stop(ScaleTaskState& state,
                                            IScaleTaskRuntime& runtime);
};

}  // namespace pal4
