#pragma once

#include <cstdint>

namespace pal4 {

// Borrowed live game/settings services; no widget or text-buffer ownership.
class ISystemSettingStatsDataRuntime {
public:
    virtual ~ISystemSettingStatsDataRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    [[nodiscard]] virtual double CalculateStatValue(
        std::uint32_t global_data) = 0;
    [[nodiscard]] virtual float UsedSaveCount() = 0;
    [[nodiscard]] virtual std::int32_t AvailableSaveCount() = 0;
    virtual void InitializeInventory() = 0;
    [[nodiscard]] virtual double InventoryRatioOne() = 0;
    [[nodiscard]] virtual double InventoryRatioTwo() = 0;
    [[nodiscard]] virtual double NormalizedStat(
        std::uint32_t global_data) = 0;
    [[nodiscard]] virtual std::uint32_t Game() = 0;
    [[nodiscard]] virtual std::int32_t GameEntry(
        std::uint32_t game, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
};

class ISystemSettingStatsRuntime : public ISystemSettingStatsDataRuntime {
public:
    virtual void SetFormattedStat(std::uint32_t window,
                                  std::uint32_t trigger_id,
                                  double value) = 0;
    virtual void SetFormattedInteger(std::uint32_t window,
                                  std::uint32_t trigger_id,
                                  std::int32_t value) = 0;
    virtual void SetSelected(std::uint32_t button, bool selected) = 0;
    virtual void SetSliderValue(std::uint32_t slider, float value) = 0;
    [[nodiscard]] virtual std::uint32_t EndTextBuffer() = 0;
};

class SystemSettingStats final {
public:
    [[nodiscard]] static std::uint32_t Refresh(
        std::uint32_t frame, ISystemSettingStatsRuntime& runtime);
};

}  // namespace pal4
