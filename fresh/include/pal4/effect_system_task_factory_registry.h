#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct EffectSystemTaskFactoryDescriptor {
    std::string_view name;
    std::uint32_t creator;
    bool via_event_helper;
};

class IEffectSystemTaskFactoryRegistryRuntime {
public:
    virtual ~IEffectSystemTaskFactoryRegistryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t RegisterEffectSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void ClearRegistry(std::uint32_t tree) = 0;
    [[nodiscard]] virtual bool HasFactory(
        std::uint32_t tree, std::string_view name) = 0;
    virtual void InsertFactory(
        std::uint32_t tree, std::string_view name,
        std::uint32_t creator) = 0;
    virtual void ProcessEvent(
        std::uint32_t tree, std::string_view name,
        std::uint32_t creator) = 0;
    [[nodiscard]] virtual std::uint8_t ReleaseName(
        std::string_view name) = 0;
};

class EffectSystemTaskFactoryRegistry final {
public:
    static constexpr std::array<EffectSystemTaskFactoryDescriptor, 24u>
        kFactories{{
            {"ShowobjTask", 0x640960u, false},
            {"HideobjTask", 0x6409c0u, false},
            {"ScaleTask", 0x640a20u, false},
            {"PositionTask", 0x640a80u, false},
            {"RotateTask", 0x640ae0u, false},
            {"RevoluteTask", 0x640b40u, false},
            {"ChangeColorTask", 0x640ba0u, false},
            {"PlaySound3DTask", 0x640c00u, false},
            {"StopSound3DTask", 0x640c60u, false},
            {"UVRotateTask", 0x640fc0u, false},
            {"UVTranslateTask", 0x641020u, false},
            {"PlayEffectTask", 0x640cc0u, true},
            {"PauseEffectTask", 0x640d80u, true},
            {"RewindEffectTask", 0x640de0u, true},
            {"StopEffectTask", 0x640d20u, true},
            {"TrackTask", 0x640e40u, true},
            {"StartCameraTask", 0x640ea0u, true},
            {"StopCameraTask", 0x640f00u, true},
            {"RumpShahdowTask", 0x640f60u, true},
            {"DecressTask", 0x641080u, true},
            {"MoveToTargetTask", 0x6410e0u, true},
            {"WobbleTask", 0x641140u, true},
            {"MessageTask", 0x6411a0u, true},
            {"SlowMotionTask", 0x6412e0u, true},
        }};

    // IDA 0x63fd20: register the 24 effect task creators in source order.
    [[nodiscard]] static std::uint8_t Register(
        IEffectSystemTaskFactoryRegistryRuntime& runtime);
};

}  // namespace pal4
