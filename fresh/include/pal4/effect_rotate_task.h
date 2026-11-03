#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct EffectRotateTaskState final {
    std::array<std::uint32_t, 46u> words{};
};
static_assert(sizeof(EffectRotateTaskState) == 0xb8u);

class IEffectRotateTaskRuntime {
public:
    virtual ~IEffectRotateTaskRuntime() = default;
    virtual void ConstructBase(EffectRotateTaskState& state) = 0;
    virtual void DestroyBase(EffectRotateTaskState& state) = 0;
    virtual void Free(EffectRotateTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t owner, std::string_view class_name) = 0;
    virtual void ReadRotation(std::uint32_t facet,
                              std::array<float, 3u>& rotation) = 0;
    virtual void ApplyRotation(std::uint32_t facet,
                               const std::array<float, 3u>& rotation,
                               bool relative) = 0;
    [[nodiscard]] virtual bool BaseDurationComplete(
        const EffectRotateTaskState& state) = 0;
    [[nodiscard]] virtual std::int32_t RandomInt() = 0;
    virtual void SetCameraLockedLookAt(std::uint32_t facet, bool locked) = 0;
};

class IEffectRotateConfigRuntime {
public:
    virtual ~IEffectRotateConfigRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t config, std::string_view key,
        std::int32_t default_value) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t config, std::string_view key, float default_value) = 0;
};

// Distinct from the action-system RotateTask at IDA 0x598380.
class EffectRotateTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84ae20u;

    // IDA 0x621490..0x6214e0.
    static EffectRotateTaskState& Construct(
        EffectRotateTaskState& state, IEffectRotateTaskRuntime& runtime);
    static EffectRotateTaskState& Destroy(
        EffectRotateTaskState& state, IEffectRotateTaskRuntime& runtime);
    static EffectRotateTaskState& Delete(
        EffectRotateTaskState& state, std::uint8_t flags,
        IEffectRotateTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    // IDA 0x6214f0..0x621c80.
    [[nodiscard]] static bool LoadConfig(
        EffectRotateTaskState& state, std::uint32_t config,
        IEffectRotateConfigRuntime& runtime);
    [[nodiscard]] static bool IsDurationComplete(
        const EffectRotateTaskState& state,
        IEffectRotateTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Start(
        EffectRotateTaskState& state, IEffectRotateTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        EffectRotateTaskState& state, float elapsed,
        IEffectRotateTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Stop(
        EffectRotateTaskState& state, IEffectRotateTaskRuntime& runtime);
};

}  // namespace pal4
