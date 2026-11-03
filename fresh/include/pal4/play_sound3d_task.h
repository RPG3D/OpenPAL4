#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PlaySound3DTaskState final {
    std::array<std::uint32_t, 20u> words{};
};
static_assert(sizeof(PlaySound3DTaskState) == 0x50u);

// EffectSystem object and sound facet calls for IDA 0x620190..0x620250.
class IPlaySound3DTaskRuntime {
public:
    virtual ~IPlaySound3DTaskRuntime() = default;
    virtual void ConstructBase(PlaySound3DTaskState& state) = 0;
    virtual void DestroyBase(PlaySound3DTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t object, std::string_view class_name) = 0;
    virtual void StartSound(std::uint32_t facet,
                            std::uint8_t non_looping) = 0;
};

class IPlaySound3DConfigRuntime {
public:
    virtual ~IPlaySound3DConfigRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t config, std::string_view key,
        std::int32_t default_value) = 0;
};

class PlaySound3DTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84ad60u;

    // IDA 0x620190/0x6201d0/0x6201e0.
    static PlaySound3DTaskState& Construct(
        PlaySound3DTaskState& state, IPlaySound3DTaskRuntime& runtime);
    static PlaySound3DTaskState& Destroy(
        PlaySound3DTaskState& state, IPlaySound3DTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    // IDA 0x6201f0/0x620250.
    [[nodiscard]] static bool LoadConfig(
        PlaySound3DTaskState& state, std::uint32_t config,
        IPlaySound3DConfigRuntime& runtime);
    [[nodiscard]] static std::uint8_t Start(
        PlaySound3DTaskState& state, IPlaySound3DTaskRuntime& runtime);
};

}  // namespace pal4
