#include "pal4/world_map_visibility.h"

namespace pal4 {

void WorldMapVisibility::Update(
    const std::uint32_t frame, const std::uint8_t visible,
    const std::uint32_t parent, IWorldMapVisibilityRuntime& runtime) {
    runtime.SetParent(frame, visible, parent);
    const auto manager = runtime.FrameManager();
    const auto cursor = runtime.ReadWord(kDefaultCursor);
    runtime.SetCursor(manager + 0x114u, cursor);
    const auto root = runtime.ReadWord(frame + 0x24u);
    if (visible == 0u) {
        runtime.SetModal(root, false);
        runtime.WriteWord(frame + 0x270u, 0xffffffffu);
        return;
    }

    runtime.InitializeButtons(frame);
    runtime.SetModal(root, true);
    const auto highlight = runtime.ReadWord(frame + 0x238u);
    runtime.SetVisible(highlight, false);
    if (runtime.ReadWord(runtime.ArenaManager() + 0x30u) == 0u) {
        return;
    }

    const auto arena_name = runtime.ReadWord(runtime.ArenaManager() + 8u);
    const auto name = runtime.CopyArenaName(
        arena_name, static_cast<std::uint8_t>(parent));
    try {
        for (std::uint32_t index = 0u; index < kLocationCount; ++index) {
            const auto record = runtime.ReadWord(frame + 0x98u + index * 0x10u);
            if (!runtime.NamesEqualIgnoreCase(name, record)) continue;
            runtime.SetVisible(highlight, true);
            const auto location = runtime.ReadWord(frame + 0x40u + index * 4u);
            runtime.SetPosition(highlight, runtime.GetPosition(location));
            runtime.WriteWord(frame + 0x26cu, index);
            break;
        }
        runtime.RefreshMouse(frame + 0x28u);
    } catch (...) {
        runtime.ReleaseArenaName(name);
        throw;
    }
    runtime.ReleaseArenaName(name);
    runtime.WriteWord(frame + 0x270u, 0xffffffffu);
}

}  // namespace pal4
