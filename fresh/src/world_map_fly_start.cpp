#include "pal4/world_map_fly_start.h"
#include "pal4/renderware_vector2.h"

namespace pal4 {
namespace {

std::string_view ImageForDirection(const float x, const float y) {
    if (x < 0.0f) return y < 0.0f ? "flya" : "flyc";
    return y < 0.0f ? "flyb" : "flyd";
}

}  // namespace

void WorldMapFlyStart::Start(const std::uint32_t frame,
                             IWorldMapFlyStartRuntime& runtime,
                             IRenderWareVector2& renderware) {
    const auto raw_index = runtime.ReadWord(frame + 0x270u);
    if (static_cast<std::int32_t>(raw_index) < 0) return;

    for (std::uint32_t i = 0u; i < 21u; ++i) {
        const auto window = runtime.ReadWord(frame + 0x40u + i * 4u);
        if (runtime.IsVisible(window, false)) {
            runtime.SetEnabled(runtime.ReadWord(frame + 0x40u + i * 4u),
                               false);
        }
    }

    const auto target = runtime.ReadWord(frame + 0x40u + raw_index * 4u);
    const auto [target_x, target_y] = runtime.GetPosition(target);
    const auto marker = runtime.ReadWord(frame + 0x238u);
    const auto [start_x, start_y] = runtime.GetPosition(marker);
    const auto delta_x = target_x - start_x;
    const auto delta_y = target_y - start_y;
    const auto length = renderware.Length(delta_x, delta_y);
    runtime.WriteFloat(frame + 0x23cu, static_cast<float>(length));
    runtime.WriteFloat(frame + 0x248u, start_x);
    runtime.WriteFloat(frame + 0x24cu, start_y);
    const auto [unit_x, unit_y] = renderware.Normalize(delta_x, delta_y);
    runtime.WriteFloat(frame + 0x240u, unit_x * 0.1f);
    runtime.WriteWord(frame + 0x254u, 0u);
    runtime.WriteWord(frame + 0x250u, 0u);
    runtime.WriteFloat(frame + 0x244u, unit_y * 0.1f);

    const auto name = runtime.CopyLocationName(
        frame + 0x94u + raw_index * 0x10u);
    try {
        const auto interval = runtime.ReadWord(frame + 0x268u);
        const auto manager = runtime.FrameManager();
        const auto queue = runtime.ReadWord(manager + 0x1a0u);
        const auto storage = runtime.Allocate(32u);
        std::uint32_t callback = 0u;
        if (storage != 0u) {
            try {
                callback = runtime.ConstructCallback(
                    storage, frame, kFlyCallback, name);
            } catch (...) {
                runtime.Free(storage);
                throw;
            }
        }
        const auto entry = runtime.ConstructTimerEntry(
            frame + 0x258u, interval, callback);
        try {
            runtime.AddTimerEntry(queue, entry);
        } catch (...) {
            runtime.DestroyTimerEntry(entry);
            throw;
        }
        runtime.DestroyTimerEntry(entry);
    } catch (...) {
        runtime.ReleaseLocationName(name);
        throw;
    }
    runtime.ReleaseLocationName(name);

    const auto image = runtime.SequenceImage(ImageForDirection(
        runtime.ReadFloat(frame + 0x240u),
        runtime.ReadFloat(frame + 0x244u)));
    runtime.WriteWord(marker + 0x7d4u, image);
}

}  // namespace pal4
