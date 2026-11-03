#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct ChangeColorTaskState final {
    std::array<std::uint32_t, 46u> words{};
};
static_assert(sizeof(ChangeColorTaskState) == 0xb8u);

class IChangeColorTaskRuntime {
public:
    virtual ~IChangeColorTaskRuntime() = default;
    virtual void ConstructBase(ChangeColorTaskState& state) = 0;
    virtual void DestroyBase(ChangeColorTaskState& state) = 0;
};

class IChangeColorConfigRuntime {
public:
    virtual ~IChangeColorConfigRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t config, std::string_view key,
        std::int32_t default_value) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t config, std::string_view key,
        float default_value) = 0;
};

class IChangeColorProgressRuntime {
public:
    virtual ~IChangeColorProgressRuntime() = default;
    [[nodiscard]] virtual bool DurationComplete(
        const ChangeColorTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveColorFacet(
        std::uint32_t object, std::string_view class_name) = 0;
    virtual void SetColorFacetEnabled(std::uint32_t facet,
                                      std::uint8_t enabled) = 0;
    virtual void ReadColor(std::uint32_t facet,
                           std::array<std::uint8_t, 4u>& channels) = 0;
    [[nodiscard]] virtual std::int32_t RandomInt() = 0;
};

class IChangeColorAnimationRuntime {
public:
    virtual ~IChangeColorAnimationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t WriteColor(
        std::uint32_t facet, const std::array<std::uint8_t, 4u>& channels) = 0;
    [[nodiscard]] virtual std::int32_t RandomInt() = 0;
};

class ChangeColorTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84abe0u;

    // IDA 0x61cdd0/0x61ce40/0x61ce50.
    static ChangeColorTaskState& Construct(ChangeColorTaskState& state,
                                           IChangeColorTaskRuntime& runtime);
    static ChangeColorTaskState& Destroy(ChangeColorTaskState& state,
                                         IChangeColorTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    // IDA 0x61ce60.
    [[nodiscard]] static bool LoadConfig(ChangeColorTaskState& state,
                                         std::uint32_t config,
                                         IChangeColorConfigRuntime& runtime);
    // IDA 0x61d110/0x61d130.
    [[nodiscard]] static bool IsComplete(
        const ChangeColorTaskState& state,
        IChangeColorProgressRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeProgress(
        ChangeColorTaskState& state, IChangeColorProgressRuntime& runtime);
    // IDA 0x61d2f0/0x61d8c0.
    [[nodiscard]] static std::uint32_t UpdateAnimation(
        ChangeColorTaskState& state, float elapsed,
        IChangeColorAnimationRuntime& runtime);
    [[nodiscard]] static std::uint8_t FinishAnimation(
        ChangeColorTaskState& state, IChangeColorAnimationRuntime& runtime);
};

}  // namespace pal4
