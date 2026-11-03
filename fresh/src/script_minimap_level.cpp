#include "pal4/script_minimap_level.h"

namespace pal4 {

void ScriptMinimapLevel::Set(
    const std::uint32_t level, IScriptMinimapLevelRuntime& runtime) {
    runtime.SetTexture(runtime.MinimapManager(), level);
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    const auto player = manager + index * 0xb14u;
    if (player == 0u) return;
    runtime.UpdateGrid(runtime.MinimapManager(),
                       player + 0xacu, player + 0xa0u);
}

}  // namespace pal4
