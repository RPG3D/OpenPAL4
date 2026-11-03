#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::ConvertUiFrameCoordinates(
    const std::uint32_t manager, const std::uint32_t output,
    const std::uint32_t input,
    IUiFrameManagerCoordinatesRuntime& runtime) {
    runtime.TransformMouse(runtime.ReadWord(manager + 0x198u),
                           output, input);
    return output;
}

std::uint32_t UiFrameManagerSingleton::TransformCeguiCoordinates(
    const std::uint32_t manager, const std::uint32_t output,
    const std::uint32_t input,
    IUiFrameManagerCoordinatesRuntime& runtime) {
    runtime.TransformCegui(runtime.ReadWord(manager + 0x198u),
                           output, input);
    return output;
}

}  // namespace pal4
