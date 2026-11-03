#pragma once

#include "pal4/rtk_prt_std_atomic_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IRtkPrtStdAtomicPropertiesRuntime {
public:
    virtual ~IRtkPrtStdAtomicPropertiesRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateFrame(std::uint32_t resource) = 0;
};

class RtkPrtStdAtomicPropertiesRuntimeStub final
    : public IRtkPrtStdAtomicPropertiesRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteFloat(std::uint32_t, float) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint8_t UpdateFrame(std::uint32_t) override { return 0u; }
};

class RtkPrtStdAtomicProperties final {
public:
    // IDA 0x614360 / 0x614390 / 0x6143c0.
    [[nodiscard]] static std::uint8_t GetRotation(
        const RtkPrtStdAtomicObjectState& state,
        std::array<std::uint32_t, 3>& output,
        IRtkPrtStdAtomicPropertiesRuntime& runtime);
    [[nodiscard]] static std::uint8_t GetScale(
        const RtkPrtStdAtomicObjectState& state,
        std::array<std::uint32_t, 3>& output,
        IRtkPrtStdAtomicPropertiesRuntime& runtime);
    [[nodiscard]] static std::uint8_t GetPosition(
        const RtkPrtStdAtomicObjectState& state,
        std::array<std::uint32_t, 3>& output,
        IRtkPrtStdAtomicPropertiesRuntime& runtime);
    // IDA 0x6143f0 / 0x614480 / 0x614510.
    [[nodiscard]] static std::uint8_t SetPosition(
        const RtkPrtStdAtomicObjectState& state,
        std::array<float, 3> values, bool relative,
        IRtkPrtStdAtomicPropertiesRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetRotation(
        const RtkPrtStdAtomicObjectState& state,
        std::array<float, 3> values, bool relative,
        IRtkPrtStdAtomicPropertiesRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetScale(
        const RtkPrtStdAtomicObjectState& state,
        std::array<float, 3> values, bool relative,
        IRtkPrtStdAtomicPropertiesRuntime& runtime);
    // IDA 0x6145a0.
    [[nodiscard]] static std::uint8_t SetResourceMode(
        const RtkPrtStdAtomicObjectState& state, std::uint8_t value,
        IRtkPrtStdAtomicPropertiesRuntime& runtime);
    // IDA 0x6145c0 / 0x6146e0.
    [[nodiscard]] static std::uint8_t GetColor(
        const RtkPrtStdAtomicObjectState& state,
        const std::array<std::uint8_t*, 4>& output,
        IRtkPrtStdAtomicPropertiesRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetColor(
        const RtkPrtStdAtomicObjectState& state,
        const std::array<std::uint8_t, 4>& color,
        IRtkPrtStdAtomicPropertiesRuntime& runtime);
};

}  // namespace pal4
