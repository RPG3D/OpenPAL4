#pragma once

#include <cstdint>

namespace pal4 {

class IPlayerIndexedStatsRuntime {
public:
    virtual ~IPlayerIndexedStatsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class PlayerIndexedStats final {
public:
    // IDA 0x592410.
    [[nodiscard]] static std::uint32_t GetCurrent(
        std::uint32_t player, std::int32_t index,
        IPlayerIndexedStatsRuntime& runtime);
    // IDA 0x592430.
    [[nodiscard]] static std::uint32_t AddBase(
        std::uint32_t player, std::int32_t index, std::uint32_t delta,
        IPlayerIndexedStatsRuntime& runtime);
    // IDA 0x592460/0x592480.
    [[nodiscard]] static double GetAttribute(
        std::uint32_t player, std::int32_t index,
        IPlayerIndexedStatsRuntime& runtime);
    [[nodiscard]] static double SetAttribute(
        std::uint32_t player, std::int32_t index, float value,
        IPlayerIndexedStatsRuntime& runtime);
    // IDA 0x5924b0/0x5924d0.
    [[nodiscard]] static double GetSkill(
        std::uint32_t player, std::int32_t index,
        IPlayerIndexedStatsRuntime& runtime);
    [[nodiscard]] static double SetSkill(
        std::uint32_t player, std::int32_t index, float value,
        IPlayerIndexedStatsRuntime& runtime);
    // IDA 0x592500.
    [[nodiscard]] static double SetFloatResistance(
        std::uint32_t player, std::int32_t index, float value,
        IPlayerIndexedStatsRuntime& runtime);
    // IDA 0x592530/0x592550.
    [[nodiscard]] static std::uint32_t GetIntegerResistance(
        std::uint32_t player, std::int32_t index,
        IPlayerIndexedStatsRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetIntegerResistance(
        std::uint32_t player, std::int32_t index, std::uint32_t value,
        IPlayerIndexedStatsRuntime& runtime);
};

}  // namespace pal4
