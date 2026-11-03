#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalConsoleGlobalDataRuntime {
public:
    virtual ~IPalConsoleGlobalDataRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    virtual void InitializeInventoryGameData(std::uint32_t inventory) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void ResetDialogManager(std::uint32_t global_data) = 0;
    virtual void SaveDialogSettings(std::uint32_t global_data) = 0;
    virtual void InitializeFormatBuffer() = 0;
    [[nodiscard]] virtual std::int32_t CleanupFormatBuffer() = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
};

class PalConsoleGlobalDataCommands final {
public:
    // IDA 0x5bc3f0 / 0x5bc420 / 0x5bc470 / 0x5bc4e0.
    [[nodiscard]] static std::int32_t SetGodMode(
        IPalConsoleGlobalDataRuntime& runtime);
    [[nodiscard]] static std::int32_t OpenMovieFlag(
        IPalConsoleGlobalDataRuntime& runtime);
    [[nodiscard]] static std::int32_t ResetGlobalData(
        IPalConsoleGlobalDataRuntime& runtime);
    [[nodiscard]] static std::int32_t SetRoundTimes(
        const char* command, IPalConsoleGlobalDataRuntime& runtime);
};

}  // namespace pal4
