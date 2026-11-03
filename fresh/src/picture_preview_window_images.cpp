#include "pal4/picture_preview_window_images.h"

namespace pal4 {

std::uint32_t PicturePreviewWindowImages::Update(
    PicturePreviewWindowState& state,
    IPicturePreviewWindowImagesRuntime& runtime) {
    for (std::uint32_t i = 0u; i < 10u; ++i) {
        const auto image = state.Read(0x2cu + i * 4u);
        const auto index = 100u + i;
        if (!runtime.HasTriggerTask(index)) {
            runtime.SetImagePointer(image, state.Read(0x60u));
            runtime.SetEnabled(image, false);
            continue;
        }
        const auto task = runtime.TriggerTaskAddress(index);
        runtime.SetVisible(image, true);
        runtime.SetEnabled(image, true);
        const auto raw = runtime.TriggerTaskImageName(task);
        const auto utf8 = runtime.ConvertToUtf8(raw);
        const auto nul = utf8.find('\0');
        runtime.SetImage(image, kImageset,
                         std::string_view(utf8.data(),
                                          nul == std::string::npos ?
                                              utf8.size() : nul));
        runtime.WriteWord(image + 0x118u, runtime.TriggerTaskId(task));
    }
    return 10u;
}

}  // namespace pal4
