#pragma once

#include "pal4/system_setting_stats.h"

#include <memory>

namespace pal4 {
class NativeGameDatabase;
class NativeDialogSettingsSnapshot;
class NativePalGameIVTextureStorage;
class NativeInventorySession;

// Borrows the loaded database, settings, game and inventory. No cached gameplay
// counters or audio levels: each refresh observes the current owners.
class NativeSystemSettingStatsData final : public ISystemSettingStatsDataRuntime {
public:
    NativeSystemSettingStatsData(const NativeGameDatabase& database,
        NativeDialogSettingsSnapshot& settings, NativePalGameIVTextureStorage& game,
        NativeInventorySession& inventory);
    ~NativeSystemSettingStatsData() override;
    NativeSystemSettingStatsData(const NativeSystemSettingStatsData&) = delete;
    NativeSystemSettingStatsData& operator=(const NativeSystemSettingStatsData&) = delete;
    std::uint32_t ReadWord(std::uint32_t address) override;
    float ReadFloat(std::uint32_t address) override;
    std::uint32_t GlobalData() override;
    double CalculateStatValue(std::uint32_t global_data) override;
    float UsedSaveCount() override;
    std::int32_t AvailableSaveCount() override;
    void InitializeInventory() override;
    double InventoryRatioOne() override;
    double InventoryRatioTwo() override;
    double NormalizedStat(std::uint32_t global_data) override;
    std::uint32_t Game() override;
    std::int32_t GameEntry(std::uint32_t game, std::uint32_t index) override;
    std::uint32_t AudioManager() override;
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}  // namespace pal4
