#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalConsolePlayerStatsRuntime {
public:
    virtual ~IPalConsolePlayerStatsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::int32_t PlayerId(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t id) = 0;
    [[nodiscard]] virtual std::int32_t SetComponentValue(
        std::uint32_t component, std::uint32_t category,
        std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
};

class PalConsolePlayerStatsCommands final {
public:
    // IDA 0x5bc150 / 0x5bc230 / 0x5bc310.
    [[nodiscard]] static std::int32_t SetHp(
        const char* command, IPalConsolePlayerStatsRuntime& runtime);
    [[nodiscard]] static std::int32_t SetMp(
        const char* command, IPalConsolePlayerStatsRuntime& runtime);
    [[nodiscard]] static std::int32_t SetRage(
        const char* command, IPalConsolePlayerStatsRuntime& runtime);
};

}  // namespace pal4
