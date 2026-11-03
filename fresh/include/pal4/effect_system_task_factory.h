#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

enum class EffectSystemTaskKind : std::uint8_t {
    Showobj,
    Hideobj,
    Scale,
    Position,
    Rotate,
    Revolute,
    ChangeColor,
    PlaySound3D,
    StopSound3D,
    UVRotate,
    UVTranslate,
    PlayEffect,
    PauseEffect,
    RewindEffect,
    StopEffect,
    Track,
    StartCamera,
    StopCamera,
    RumpShahdow,
    Decress,
    MoveToTarget,
    Wobble,
    Message,
    SlowMotion,
};

class IEffectSystemTaskFactoryRuntime {
public:
    virtual ~IEffectSystemTaskFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructTask(
        EffectSystemTaskKind kind, std::uint32_t object) = 0;
    virtual void ConstructCommonTask(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class EffectSystemTaskFactory final {
public:
    static constexpr std::array<std::uint32_t, 24u> kSizes{
        0x4cu, 0x4cu, 0x9cu, 0xe4u, 0xb8u, 0x88u,
        0xb8u, 0x50u, 0x4cu, 0x60u, 0x5cu, 0x50u,
        0x4cu, 0x4cu, 0x5cu, 0x90u, 0x4cu, 0x4cu,
        0xb8u, 0x54u, 0x90u, 0x54u, 0x5cu, 0x50u};

    // IDA 0x640960..0x6412e0: fixed-size allocation and construction.
    [[nodiscard]] static std::uint32_t Create(
        EffectSystemTaskKind kind,
        IEffectSystemTaskFactoryRuntime& runtime,
        std::uint8_t native_stack_byte = 0u);
};

}  // namespace pal4
