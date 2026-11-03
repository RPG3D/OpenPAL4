#include "pal4/system_setting_stats.h"

namespace pal4 {
namespace {

void SetStat(const std::uint32_t frame, const std::uint32_t index,
             const double value, ISystemSettingStatsRuntime& runtime) {
    const auto window = runtime.ReadWord(frame + 440u + 4u * index);
    runtime.SetFormattedStat(window, 156u + index, value);
}

void SetInteger(const std::uint32_t frame, const std::uint32_t index,
                const std::int32_t value, ISystemSettingStatsRuntime& runtime) {
    const auto window = runtime.ReadWord(frame + 440u + 4u * index);
    runtime.SetFormattedInteger(window, 156u + index, value);
}

}  // namespace

std::uint32_t SystemSettingStats::Refresh(
    const std::uint32_t frame, ISystemSettingStatsRuntime& runtime) {
    try {
        SetStat(frame, 0u,
                runtime.CalculateStatValue(runtime.GlobalData()), runtime);
        const auto used = runtime.UsedSaveCount();
        const auto available = runtime.AvailableSaveCount();
        SetStat(frame, 1u, static_cast<float>(
                    static_cast<double>(used) / available * 100.0), runtime);
        runtime.InitializeInventory();
        SetStat(frame, 2u, static_cast<float>(runtime.InventoryRatioOne() * 100.0), runtime);
        runtime.InitializeInventory();
        SetStat(frame, 3u, static_cast<float>(runtime.InventoryRatioTwo() * 100.0), runtime);
        SetStat(frame, 4u,
                static_cast<float>(runtime.NormalizedStat(runtime.GlobalData()) * 100.0),
                runtime);

        const auto first = static_cast<float>(runtime.GameEntry(runtime.Game(), 0u));
        const auto second = static_cast<double>(runtime.GameEntry(runtime.Game(), 1u));
        const auto total = static_cast<double>(first) + second;
        const auto first_percent = total == 0.0 ? 0.0f :
            static_cast<float>(static_cast<double>(first) / total * 100.0);
        const auto second_percent = total == 0.0 ? 0.0f :
            static_cast<float>(second / total * 100.0);
        SetStat(frame, 5u, first_percent, runtime);
        SetStat(frame, 6u, second_percent, runtime);
        for (std::uint32_t index = 0; index < 5u; ++index) {
            const auto value = runtime.GameEntry(runtime.Game(), index + 2u);
            SetInteger(frame, index + 7u, value, runtime);
        }

        constexpr std::uint32_t groups[][2] = {
            {200u, 3u}, {212u, 2u}, {220u, 3u}, {232u, 2u}};
        for (const auto& group : groups) {
            for (std::uint32_t index = 0; index < group[1]; ++index) {
                runtime.SetSelected(runtime.ReadWord(frame + group[0] +
                                                     4u * index), false);
            }
        }
        auto setting = runtime.ReadWord(runtime.GlobalData() + 4u);
        if (setting < 3u) {
            runtime.SetSelected(runtime.ReadWord(frame + 200u +
                                                 4u * setting), true);
        }
        setting = runtime.ReadWord(runtime.GlobalData() + 12u);
        runtime.SetSelected(runtime.ReadWord(frame + 212u +
                                             (setting != 0u ? 4u : 0u)), true);
        setting = runtime.ReadWord(runtime.GlobalData());
        const auto auto_index = setting == 2u ? 2u : setting == 1u ? 0u : 1u;
        runtime.SetSelected(runtime.ReadWord(frame + 220u +
                                             4u * auto_index), true);
        setting = runtime.ReadWord(runtime.GlobalData() + 8u);
        runtime.SetSelected(runtime.ReadWord(frame + 232u +
                                             (setting != 0u ? 0u : 4u)), true);

        runtime.SetSliderValue(runtime.ReadWord(frame + 496u),
                               1.0f - runtime.ReadFloat(runtime.AudioManager()));
        runtime.SetSliderValue(runtime.ReadWord(frame + 500u),
                               1.0f - runtime.ReadFloat(runtime.AudioManager() + 4u));
    } catch (...) {
        (void)runtime.EndTextBuffer();
        throw;
    }
    return runtime.EndTextBuffer();
}

}  // namespace pal4
