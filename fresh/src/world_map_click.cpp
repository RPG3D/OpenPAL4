#include "pal4/world_map_click.h"
#include "pal4/world_map_fly_start.h"

namespace pal4 {
namespace {

void ShowTrigger(const std::uint32_t trigger,
                 const std::uint32_t mode,
                 const std::uint32_t callback,
                 IWorldMapClickRuntime& runtime) {
    const auto database = runtime.GameDatabase();
    auto text = runtime.TriggerText(database + 0x34u, trigger);
    if (text == 0u) {
        text = runtime.EmptyText();
    }
    const auto manager = runtime.FrameManager();
    const auto dialog = runtime.CreateCommonDialog(manager);
    runtime.ShowDialog(dialog, text, WorldMapClick::kCenterBits,
                       WorldMapClick::kCenterBits, mode, callback);
}

}  // namespace

std::uint8_t WorldMapClick::OnLocation(
    const std::uint32_t frame, const std::uint32_t event,
    IWorldMapClickRuntime& runtime) {
    if (static_cast<std::int32_t>(runtime.ReadWord(frame + 0x270u)) >= 0) {
        return 0u;
    }
    if (!runtime.IsVisible(runtime.ReadWord(frame + 0x238u), false)) {
        return 1u;
    }

    const auto source = runtime.ReadWord(event + 8u);
    const auto index = runtime.ReadWord(source + 0x118u);
    if (runtime.ReadWord(frame + 0x1e4u + index * 4u) == 2u) {
        ShowTrigger(246u, 4u, 0u, runtime);
        return 1u;
    }

    runtime.WriteWord(frame + 0x270u, index);
    const auto callback = runtime.Allocate(24u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, kOnDialogResult);
        runtime.WriteWord(callback + 12u, 0u);
        runtime.WriteWord(callback + 16u, frame);
        runtime.WriteWord(callback, kCallbackVtable);
    }
    try {
        ShowTrigger(index + 287u, 3u, callback, runtime);
    } catch (...) {
        if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
    return 1u;
}

std::uint8_t WorldMapClick::OnDialogResult(
    const std::uint32_t frame, const std::uint32_t result,
    IWorldMapDialogResultRuntime& runtime) {
    if (result == 1u) {
        runtime.StartFly(frame);
    } else {
        runtime.WriteWord(frame + 0x270u, 0xffffffffu);
    }
    return 1u;
}

std::uint8_t WorldMapClick::OnDialogResult(
    const std::uint32_t frame, const std::uint32_t result,
    IWorldMapFlyStartRuntime& runtime, IRenderWareVector2& renderware) {
    if (result == 1u) {
        WorldMapFlyStart::Start(frame, runtime, renderware);
    } else {
        runtime.WriteWord(frame + 0x270u, 0xffffffffu);
    }
    return 1u;
}

}  // namespace pal4
