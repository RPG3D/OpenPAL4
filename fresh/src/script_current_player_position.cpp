#include "pal4/script_current_player_position.h"

namespace pal4 {

double ScriptCurrentPlayerPosition::Axis(
    std::uint32_t offset, IScriptCurrentPlayerPositionRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto player = manager +
        runtime.ReadWord(manager + 0x2c50u) * 0xb14u;
    return player == 0u ? 0.0 :
        static_cast<double>(runtime.ReadFloat(player + offset));
}

double ScriptCurrentPlayerPosition::X(
    IScriptCurrentPlayerPositionRuntime& runtime) {
    return Axis(0xacu, runtime);
}

double ScriptCurrentPlayerPosition::Y(
    IScriptCurrentPlayerPositionRuntime& runtime) {
    return Axis(0xb0u, runtime);
}

double ScriptCurrentPlayerPosition::Z(
    IScriptCurrentPlayerPositionRuntime& runtime) {
    return Axis(0xb4u, runtime);
}

}  // namespace pal4
