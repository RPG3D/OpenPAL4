#include "pal4/minimap_scene_ui_extended_update.h"

#include <vector>

namespace pal4 {

std::uint8_t MinimapSceneUiExtendedUpdate::Update(
    MinimapSceneUiState& state, const std::uint32_t owner,
    IMinimapSceneUiExtendedRuntime& runtime) {
    state.WriteWord(0xccu, 0u);
    const auto script = runtime.ReadWord(runtime.ArenaManager() + 12u);
    const auto path = runtime.ReadWord(runtime.ArenaManager() + 8u);
    runtime.LoadScriptData(runtime.MinimapManager(), path, script);
    (void)runtime.MinimapManager();
    runtime.InitializeDialogSystem();

    const auto variable_name = runtime.ReadWord(runtime.ArenaManager() + 224u);
    const auto variable_path = runtime.ReadWord(runtime.ArenaManager() + 8u);
    const auto variable = runtime.QueryTriggerVariable(
        variable_path, variable_name);
    std::uint32_t key = 0u;
    std::uint32_t secondary = 0u;
    bool key_live = false;
    bool secondary_live = false;
    bool variable_live = true;
    try {
        key = runtime.CopyFloorAttribute(variable.primary);
        key_live = true;
        secondary = runtime.CopyFloorAttribute(variable.secondary);
        secondary_live = true;

        const auto table = runtime.ItemPropertyTable();
        const auto collection = table + 92u;
        std::vector<std::uint32_t> matches;
        if (runtime.ItemPropertyCount(collection) != 0u) {
            std::uint32_t index = 0u;
            do {
                const auto row = runtime.ReadWord(table + 96u) + 60u * index;
                if (runtime.MatchesFloorAttribute(key, row)) {
                    matches.push_back(row);
                }
                ++index;
            } while (index < runtime.ItemPropertyCount(collection));
        }
        key_live = false;
        runtime.ReleaseFloorAttribute(key);

        if (!matches.empty()) {
            const auto selected = matches.front();
            state.WriteWord(0xccu, selected);
            auto text = runtime.ReadWord(selected + 40u);
            if (text == 0u) text = runtime.EmptyString();
            runtime.SetWindowText(state.ReadWord(0x58u),
                                  runtime.ConvertToUtf8(text));

            const auto mode = runtime.ReadWord(selected + 52u);
            if (mode <= 2u) {
                const auto minimap = runtime.MinimapManager();
                const std::uint32_t minimap_mode = *MinimapModeForSceneType(mode);
                runtime.WriteWord(minimap + 80u, minimap_mode);
                runtime.SetFlagAndUpdate(owner,
                                         static_cast<std::uint8_t>(mode == 1u));
            }
            runtime.UpdateVisibilityAndMinimap(owner);
            runtime.UpdateVisibilityBasedOnState(owner);
        } else {
            runtime.WriteWord(runtime.MinimapManager() + 80u, 2u);
        }

        const auto [screen_width, screen_height] = runtime.ConfigSize();
        float x = 2.0f;
        float margin = 28.0f;
        float size = 173.0f;
        if (screen_width != 800) {
            const auto factor = static_cast<double>(screen_width) * 0.00125;
            x = static_cast<float>(factor + factor);
            margin = static_cast<float>(28.0 * factor);
            size = static_cast<float>(173.0 * factor);
        }
        const auto minimap = runtime.MinimapManager();
        const auto pixel_size = runtime.FloatToLong(size);
        const auto pixel_y = runtime.FloatToLong(
            static_cast<double>(screen_height) - size - margin);
        const auto pixel_x = runtime.FloatToLong(x);
        runtime.SetupMinimapTexture(minimap, pixel_x, pixel_y,
                                    pixel_size, pixel_size);

        secondary_live = false;
        runtime.ReleaseFloorAttribute(secondary);
        variable_live = false;
        runtime.ReleaseTriggerVariable(variable);
    } catch (...) {
        if (key_live) runtime.ReleaseFloorAttribute(key);
        if (secondary_live) runtime.ReleaseFloorAttribute(secondary);
        if (variable_live) runtime.ReleaseTriggerVariable(variable);
        throw;
    }
    return 1u;
}

}  // namespace pal4
