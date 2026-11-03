#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

class IPalConsolePlayerLevelRuntime {
public:
    virtual ~IPalConsolePlayerLevelRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::int32_t PlayerId(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t id) = 0;
    virtual void SetComponentValue(std::uint32_t component,
                                   std::uint32_t category,
                                   std::int32_t level) = 0;
    [[nodiscard]] virtual std::string PlayerName(std::uint32_t player) = 0;
    virtual void InitializeFormatBuffer() = 0;
    [[nodiscard]] virtual std::int32_t CleanupFormatBuffer() = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void UpdateLevelEvent(std::uint32_t component,
                                  std::int32_t level,
                                  std::uint32_t game_data) = 0;
    [[nodiscard]] virtual std::vector<std::string> BuildNewMatchingRecords(
        std::uint32_t collection, std::int32_t level,
        std::uint32_t game_data) = 0;
    virtual void ReleaseMatchingRecords() = 0;
};

class PalConsolePlayerLevelCommand final {
public:
    // IDA 0x5bbac0, command "setPlayerLevel".
    [[nodiscard]] static std::int32_t Execute(
        const char* command, IPalConsolePlayerLevelRuntime& runtime);
};

}  // namespace pal4
