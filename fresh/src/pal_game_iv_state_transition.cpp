#include "pal4/pal_game_iv_state_transition.h"

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 13> kStateNames = {
    "PALIV_LOGO", "PALIV_SCENE", "PALIV_SYSTEM", "PALIV_COMBAT",
    "PALIV_TRADE", "PALIV_FOUNDRY", "PALIV_LOADING", "PALIV_GAME_OVER",
    "PALIV_MINIGAME", "PALIV_GAME_SAVE", "PALIV_MODAL_FRAME",
    "PALIV_WORLD_MAP", "PALIV_BINK",
};

std::string_view NameAt(const PalGameIVStateLabels& labels,
                        std::uint32_t id) {
    return id < labels.names.size() ? std::string_view(labels.names[id])
                                    : std::string_view{};
}

}  // namespace

bool PalGameIVStateTransition::Initialize(
    std::uint32_t game, std::uint32_t next_id,
    std::uint32_t argument, PalGameIVStateLabels& labels,
    IPalGameIVStateRuntime& runtime) {
    if ((labels.guard & 1u) == 0u) {
        labels.guard |= 1u;
        for (std::size_t index = 0; index < kStateNames.size(); ++index) {
            labels.markers[index] = static_cast<std::uint8_t>(next_id);
            labels.error_flags[index] = 0u;
            labels.names[index] = kStateNames[index];
        }
        runtime.RegisterAtExit(0x5b67a0u);
    }

    const auto entry = runtime.FindEntry(game, next_id);
    if (entry == 0u) return false;
    const auto current = runtime.ReadWord(game + 0x18cu);
    if (current != 0u) runtime.Leave(current, next_id);
    const auto previous_id = current != 0u ? runtime.StateId(current) : 13u;
    const auto ui = runtime.UiFrameManager();
    runtime.SetCursor(ui + 0x114u, "default.cur");
    if (current != 0u) {
        const auto old_id = runtime.StateId(current);
        runtime.LogExchange(NameAt(labels, old_id),
                            NameAt(labels, next_id), 0x1a1u);
    }
    if (!runtime.Enter(entry, previous_id, argument)) return false;
    runtime.WriteWord(game + 0x18cu, entry);
    const auto time = runtime.TimeGetTime();
    runtime.WriteGameStateWord(0x574u, time);
    runtime.WriteGameStateWord(0x56cu, time);
    return true;
}

void PalGameIVStateTransition::Cleanup(PalGameIVStateLabels& labels) {
    for (std::size_t index = labels.names.size(); index != 0u; --index)
        labels.names[index - 1u].clear();
}

}  // namespace pal4
