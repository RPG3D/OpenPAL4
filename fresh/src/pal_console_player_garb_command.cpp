#include "pal4/pal_console_player_garb_command.h"

#include <array>

namespace pal4 {

void PalConsolePlayerGarbCommand::ApplyToAll(
    IPalConsolePlayerGarbRuntime& runtime) {
    constexpr std::array<std::string_view, 3> models{
        "101", "103", "106"};
    for (std::uint32_t index = 0u; index < models.size(); ++index) {
        const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                                  index);
        const auto buffer = player + 0xa18u;
        if (index < 2u) {
            runtime.AssignModelText(buffer, models[index]);
        } else if (runtime.AllocateModelText(
                       buffer, static_cast<std::uint32_t>(models[index].size()),
                       1u)) {
            runtime.WriteModelText(buffer, models[index]);
        }

        if (!runtime.NeedsModelReload(player)) continue;
        const auto old_state = runtime.ReadDword(player + 0x1f0u);
        runtime.ScriptOnRemove(runtime.ArenaManager(), player);
        runtime.ProcessAndCleanup(player);
        if (old_state != 0u) {
            runtime.SetStateFlag(player, old_state);
            runtime.ScriptOnAdd(runtime.ArenaManager(), player);
        }
        const auto manager = runtime.PlayerManager();
        const auto active_index = runtime.ReadDword(manager + 0x2c50u);
        if (player == manager + 2836u * active_index)
            runtime.SetActor(runtime.ActorControl(), player);
    }
}

}  // namespace pal4
