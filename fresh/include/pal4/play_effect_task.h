#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PlayEffectTaskState final {
    std::array<std::uint32_t, 20u> words{};
};
static_assert(sizeof(PlayEffectTaskState) == 0x50u);

// Base lifecycle and EffectSystem facet calls for IDA 0x61fd00..0x61fe20.
class IPlayEffectTaskRuntime {
public:
    virtual ~IPlayEffectTaskRuntime() = default;
    virtual void ConstructBase(PlayEffectTaskState& state) = 0;
    virtual void DestroyBase(PlayEffectTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t object, std::string_view class_name) = 0;
    virtual void SetLoop(std::uint32_t facet, std::uint8_t loop) = 0;
};

// XML attribute conversion remains a native runtime boundary.
class IPlayEffectConfigRuntime {
public:
    virtual ~IPlayEffectConfigRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t config, std::string_view key,
        std::int32_t default_value) = 0;
};

class PlayEffectTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84ad20u;

    // IDA 0x61fd00/0x61fd40/0x61fd50.
    static PlayEffectTaskState& Construct(PlayEffectTaskState& state,
                                          IPlayEffectTaskRuntime& runtime);
    static PlayEffectTaskState& Destroy(PlayEffectTaskState& state,
                                        IPlayEffectTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    // IDA 0x61fd60/0x61fdf0/0x61fe20.
    [[nodiscard]] static bool LoadConfig(PlayEffectTaskState& state,
                                         std::uint32_t config,
                                         IPlayEffectConfigRuntime& runtime);
    [[nodiscard]] static std::uint8_t Start(PlayEffectTaskState& state,
                                            IPlayEffectTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Reset(PlayEffectTaskState& state);
};

}  // namespace pal4
