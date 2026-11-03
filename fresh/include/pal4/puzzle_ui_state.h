#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

class IPuzzleUiRuntime {
public:
    virtual ~IPuzzleUiRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void SetUIFlag(std::uint32_t manager, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::optional<std::uint32_t> ReadIntegerAttribute(
        std::uint32_t config, std::string_view key) = 0;
    virtual void DispatchEvent(std::uint32_t manager,
                               std::uint32_t descriptor,
                               std::uint32_t first,
                               std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateUI(
        std::uint32_t manager, double delta) = 0;
};

class PuzzleUiState final {
public:
    static constexpr std::uint32_t kShowPuzzleEvent = 0x8b12d8u;
    static constexpr std::uint32_t kShowJigsawEvent = 0x8b1238u;

    // IDA 0x58fd30/0x58fdf0, virtual slots 0x849424/0x84942c.
    [[nodiscard]] static std::uint32_t Enter(
        std::uint32_t unused, std::uint32_t config,
        IPuzzleUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        float delta, std::uint32_t unused1,
        std::uint32_t unused2, IPuzzleUiRuntime& runtime);
};

}  // namespace pal4
