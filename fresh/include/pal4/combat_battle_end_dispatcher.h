#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatBattleEndRuntime {
public:
    virtual ~ICombatBattleEndRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystemInstance() = 0;
    [[nodiscard]] virtual std::uint32_t GameDataInstance() = 0;
    [[nodiscard]] virtual std::uint32_t GameEntry(
        std::uint32_t game, std::uint32_t index) = 0;
    virtual void SetGameEntry(std::uint32_t game, std::uint32_t index,
                              std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CombatConsole() = 0;
    virtual void UnsubscribeConsole(std::uint32_t console) = 0;
    [[nodiscard]] virtual bool IsCombatInactive(
        std::uint32_t system) = 0;

    [[nodiscard]] virtual std::uint32_t PrepareReport() = 0;
    [[nodiscard]] virtual std::uint32_t CopyReport(
        std::uint32_t report) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessRewards(
        std::uint32_t report) = 0;
    virtual void ShowEnding(std::uint32_t system,
                            std::uint32_t prize) = 0;
    virtual void DestroyPrize(std::uint32_t prize) = 0;
    virtual void DestroyCopiedReport(std::uint32_t report) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyReport(
        std::uint32_t report) = 0;

    virtual void ShowDefeatBanner(std::string_view imageset,
                                  std::string_view image, float scale,
                                  float x, float y) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScheduleClose(
        std::uint32_t event_owner, std::uint32_t key,
        float delay, std::uint32_t callback_object) = 0;
    virtual void EnqueueClose(std::uint32_t queue,
                              std::uint32_t key,
                              std::uint32_t callback_object) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupQueuedEvent() = 0;
};

class CombatBattleEndDispatcher final {
public:
    static constexpr std::uint32_t kEndCallback = 0x576440u;
    static constexpr std::uint32_t kCallbackVtable = 0x8480ecu;
    static constexpr std::uint32_t kCloseEventKey = 0x8fa3b0u;

    // IDA 0x57a920.
    [[nodiscard]] static std::uint32_t Handle(
        std::uint32_t event, ICombatBattleEndRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t CreateEndTask(
        std::uint32_t system, std::uint32_t result,
        ICombatBattleEndRuntime& runtime);
    [[nodiscard]] static std::uint32_t ShowRewards(
        std::uint32_t system, std::uint32_t result,
        ICombatBattleEndRuntime& runtime);
};

}  // namespace pal4
