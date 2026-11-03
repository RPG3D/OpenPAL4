#include "pal4/script_player_skill_acquisition.h"

namespace pal4 {
namespace {

struct TextOwner {
    IScriptPlayerSkillAcquisitionRuntime& runtime;
    bool active = false;

    ~TextOwner() {
        if (active) (void)runtime.CleanupText();
    }
    std::uint8_t Close() noexcept {
        active = false;
        return runtime.CleanupText();
    }
};

}  // namespace

std::uint8_t ScriptPlayerSkillAcquisition::Add(
    std::int32_t player_index, std::uint32_t skill_id,
    std::uint8_t show_hint,
    IScriptPlayerSkillAcquisitionRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                              player_index);
    if (player == 0u) return 0u;
    const auto window = runtime.CreateHintWindow(runtime.UiFrameManager());
    if (window == 0u) return 0u;
    if (runtime.HasSkill(runtime.GameDatabase() + 0xcu, skill_id) == 0u)
        return 0u;

    runtime.InsertSkill(player + 0x938u, skill_id);
    const auto record = runtime.FindSkill(runtime.GameDatabase() + 0xcu,
                                          skill_id);
    if (show_hint == 0u || record == 0u) return show_hint;

    TextOwner text{runtime};
    runtime.InitializeText();
    text.active = true;
    const auto skill_name = runtime.SkillName(record);
    const auto player_name = runtime.PlayerName(player);
    const auto format = runtime.GameString(runtime.GameDatabase() + 0x34u,
                                           0x118u);
    runtime.FormatText(format, player_name, skill_name);
    runtime.ShowHint(window, runtime.HintText(), 0.5f, 0.15f, 1.0f,
                     0u, 1u);
    runtime.PlaySound(runtime.AudioManager(), show_hint,
                      "gamedata\\PALSound\\WA17.wav", 1u);
    return text.Close();
}

}  // namespace pal4
