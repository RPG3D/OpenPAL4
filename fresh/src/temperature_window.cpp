#include "pal4/temperature_window.h"

#include <string>

namespace pal4 {

std::uint32_t TemperatureWindow::Construct(
    const std::uint32_t object, const std::uint32_t layout_name,
    ITemperatureWindowRuntime& runtime) {
    runtime.LoadLayout(object, layout_name);
    runtime.WriteWord(object, kVtable);
    (void)SetImages(object, runtime);
    (void)SetPosition(object, 0.0f, runtime);
    return object;
}

std::uint32_t TemperatureWindow::SetImages(
    const std::uint32_t object, ITemperatureWindowRuntime& runtime) {
    const std::string base(runtime.WindowName(object));
    try {
        runtime.WriteWord(object + 0x2cu,
                          runtime.GetWindow(object, base + "ImgDial"));
        runtime.WriteWord(object + 0x28u,
                          runtime.GetWindow(object, base + "ImgPointer"));
        runtime.WriteWord(object + 0x30u, 0x436b0000u);
        runtime.WriteWord(object + 0x34u, 0x42040000u);
        runtime.SetEnabled(runtime.ReadWord(object + 0x24u), false);
    } catch (...) {
        (void)runtime.ReleaseNameBuffer();
        throw;
    }
    return runtime.ReleaseNameBuffer();
}

std::uint32_t TemperatureWindow::SetPosition(
    const std::uint32_t object, const float position,
    ITemperatureWindowRuntime& runtime) {
    const auto scale = runtime.ReadFloat(object + 0x30u);
    const auto offset = runtime.ReadFloat(object + 0x34u);
    const auto x = position * scale + offset;
    return runtime.SetXPosition(runtime.ReadWord(object + 0x28u), 1u, x);
}

}  // namespace pal4
