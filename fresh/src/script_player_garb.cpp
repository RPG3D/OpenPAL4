#include "pal4/script_player_garb.h"

#include <array>

namespace pal4 {

void ScriptPlayerGarb::ApplyKnownPlayer(
    std::uint32_t player, std::int32_t index, bool second,
    IScriptPlayerGarbRuntime& runtime) {
    constexpr std::array<std::string_view, 3> first{"101", "103", "106"};
    constexpr std::array<std::string_view, 3> second_models{"102", "104", "107"};
    const auto model = (second ? second_models : first)[index];
    const auto buffer = player + 0xa18u;
    if (index < 2) {
        runtime.AssignModelText(buffer, model);
    } else if (runtime.AllocateModelText(
                   buffer, static_cast<std::uint32_t>(model.size()), 1u)) {
        runtime.WriteModelText(buffer, model);
    }

    if (!runtime.NeedsModelReload(player)) return;
    const auto old_state = runtime.ReadDword(player + 0x1f0u);
    runtime.ScriptOnRemove(runtime.ArenaManager(), player);
    runtime.ProcessAndCleanup(player);
    if (old_state != 0u) {
        runtime.SetStateFlag(player, old_state);
        runtime.ScriptOnAdd(runtime.ArenaManager(), player);
    }
    const auto manager = runtime.PlayerManager();
    const auto active_index = runtime.ReadDword(manager + 0x2c50u);
    if (player == manager + 0xb14u * active_index)
        runtime.SetActor(runtime.ActorControl(), player);
}

void ScriptPlayerGarb::ApplySecondToAll(
    IScriptPlayerGarbRuntime& runtime) {
    for (std::int32_t index = 0; index < 3; ++index) {
        const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), index);
        ApplyKnownPlayer(player, index, true, runtime);
    }
}

void ScriptPlayerGarb::ApplySecond(
    std::int32_t index, IScriptPlayerGarbRuntime& runtime) {
    const auto initial = runtime.PlayerByIndex(runtime.PlayerManager(), index);
    if (index < 0 || index > 2) return;
    const auto player = index == 0 ? initial :
        runtime.PlayerByIndex(runtime.PlayerManager(), index);
    ApplyKnownPlayer(player, index, true, runtime);
}

void ScriptPlayerGarb::ApplyFirst(
    std::int32_t index, IScriptPlayerGarbRuntime& runtime) {
    (void)runtime.PlayerByIndex(runtime.PlayerManager(), index);
    if (index < 0 || index > 2) return;
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), index);
    ApplyKnownPlayer(player, index, false, runtime);
}

}  // namespace pal4
