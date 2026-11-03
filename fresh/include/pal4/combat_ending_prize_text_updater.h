#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class ICombatEndingResultTextRuntime {
public:
    virtual ~ICombatEndingResultTextRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t NextTreeNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::optional<std::string> PrimaryName(
        std::uint32_t key) = 0;
    [[nodiscard]] virtual std::optional<std::string> SecondaryName(
        std::uint32_t key) = 0;
    [[nodiscard]] virtual std::string GameString(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string FormatEntry(
        std::string_view format, std::string_view name,
        std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string ToUtf8(std::string_view text) = 0;
    virtual void SetFormatting(std::uint32_t window,
                               std::uint32_t horizontal,
                               std::uint32_t vertical) = 0;
    virtual void SetText(std::uint32_t window, std::string_view text) = 0;
    [[nodiscard]] virtual float GetTextHeight(std::uint32_t window) = 0;
    [[nodiscard]] virtual float GetHeight(std::uint32_t window,
                                          std::uint32_t metric) = 0;
    virtual void SetHeight(std::uint32_t window, std::uint32_t metric,
                           float height) = 0;
    [[nodiscard]] virtual int CleanupFormatBuffer() = 0;
};

class CombatEndingPrizeTextUpdater final {
public:
    // IDA ui_updateScoreText at 0x546770.
    [[nodiscard]] static int Update(std::uint32_t dialog,
                                    ICombatEndingResultTextRuntime& runtime);
};

class CombatEndingPropertyTextUpdater final {
public:
    // IDA ui_updateScoreText_part2 at 0x546d40.
    [[nodiscard]] static int Update(std::uint32_t dialog,
                                    ICombatEndingResultTextRuntime& runtime);
};

}  // namespace pal4
