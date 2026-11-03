#pragma once

#include <cstdint>
#include <optional>

namespace pal4 {

class IEffectSystemRadiusRuntime {
public:
    virtual ~IEffectSystemRadiusRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual void CleanupEffects(std::uint32_t auxiliary) = 0;
};

class EffectSystemRadius final {
public:
    // IDA 0x63d360: clear effects, enable the auxiliary system and square time.
    [[nodiscard]] static std::uint8_t SetTimeAndFlag(
        std::uint32_t auxiliary, float radius,
        IEffectSystemRadiusRuntime& runtime);
    // IDA 0x63d380: update only for positive input while enabled.
    // The inactive path has undefined high EAX bits in the binary.
    [[nodiscard]] static std::optional<std::uint32_t> SetSquaredValue(
        std::uint32_t auxiliary, float radius,
        IEffectSystemRadiusRuntime& runtime);
};

}  // namespace pal4
