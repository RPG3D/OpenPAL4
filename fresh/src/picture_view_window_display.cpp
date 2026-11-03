#include "pal4/picture_view_window_display.h"

namespace pal4 {

void PictureViewWindowDisplay::Show(
    PictureViewWindowState& state, std::uint32_t record,
    IPictureViewWindowDisplayRuntime& runtime) {
    runtime.ProcessPictureScript();
    if (runtime.HasImageset(kImageset)) runtime.DestroyImageset(kImageset);

    std::string file = "gamedata\\ui\\picture\\";
    file += runtime.RecordImageName(record);
    runtime.CreateImageset(kImageset, file, "");
    runtime.SetImagesetAutoScaling(kImageset, false);
    runtime.SetImage(state.Read(0x4cu), kImageset, kImage);

    const auto begin = state.Read(0x54u);
    const auto count = begin == 0u ? 0u :
        static_cast<std::uint32_t>(
            static_cast<std::int32_t>(state.Read(0x58u) - begin) >> 2);
    for (std::uint32_t i = 0u; i < count; ++i) {
        if (runtime.ReadWord(begin + i * 4u) == record) state.Write(0x60u, i);
    }

    const auto right = state.Read(0x48u);
    const auto left = state.Read(0x44u);
    runtime.SetEnabled(right, true);
    runtime.SetEnabled(left, true);
    const auto index = state.Read(0x60u);
    if (index >= count - 1u) runtime.SetEnabled(right, false);
    if (static_cast<std::int32_t>(index) <= 0)
        runtime.SetEnabled(left, false);
    runtime.CleanupPictureScripts();
}

}  // namespace pal4
