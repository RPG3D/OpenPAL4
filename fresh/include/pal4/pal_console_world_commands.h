#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalConsoleWorldRuntime {
public:
    virtual ~IPalConsoleWorldRuntime() = default;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::int32_t SelectWeatherEffect(
        std::uint32_t arena, std::int32_t effect) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateWeatherEffects(
        std::uint32_t arena) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
};

class PalConsoleWorldCommands final {
public:
    // IDA 0x5bcde0 / 0x5bcf20 / 0x5bcf90.
    [[nodiscard]] static std::int32_t SetMazeVariable(
        const char* command, IPalConsoleWorldRuntime& runtime);
    [[nodiscard]] static std::int32_t OpenWeather(
        const char* command, IPalConsoleWorldRuntime& runtime);
    [[nodiscard]] static std::uint8_t CloseWeather(
        IPalConsoleWorldRuntime& runtime);
};

}  // namespace pal4
