#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ICombatEndingScoreRuntime {
public:
    virtual ~ICombatEndingScoreRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CopyTreeData(std::uint32_t destination,
                              std::uint32_t source) = 0;
    virtual void CopyPlayerData(std::uint32_t destination,
                                std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t IconImage(std::uint32_t slot) = 0;
    [[nodiscard]] virtual std::string GameString(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string ToUtf8(std::string_view text) = 0;
    [[nodiscard]] virtual std::string FormatMoney(std::string_view format,
                                                   std::int32_t value) = 0;
    virtual void SetImage(std::uint32_t window, std::uint32_t image) = 0;
    virtual void SetText(std::uint32_t window, std::string_view text) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual float GetHeight(std::uint32_t window) = 0;
    virtual void SetYPosition(std::uint32_t window, float y) = 0;
    virtual void SetWidth(std::uint32_t window, float width) = 0;
    [[nodiscard]] virtual std::uint32_t BeginActors() = 0;
    [[nodiscard]] virtual bool ActorsDone(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentActor(std::uint32_t iterator) = 0;
    virtual void AdvanceActor(std::uint32_t iterator) = 0;
    virtual void ReleaseActors(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::int32_t ActorBaseStat(
        std::uint32_t actor, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::int32_t ActorCurrentStat(
        std::uint32_t actor, std::uint32_t stat) = 0;
    virtual void UpdateScoreTextPart2(std::uint32_t dialog) = 0;
    virtual void UpdateScoreText(std::uint32_t dialog) = 0;
    virtual void ShowEndTaskScore(std::uint32_t dialog) = 0;
    [[nodiscard]] virtual int CleanupTextBuffer() = 0;
};

class CombatEndingScoreUpdater final {
public:
    // IDA ui_updateScore at 0x545ae0.
    [[nodiscard]] static int Update(std::uint32_t dialog,
                                    std::uint32_t result,
                                    ICombatEndingScoreRuntime& runtime);
};

}  // namespace pal4
