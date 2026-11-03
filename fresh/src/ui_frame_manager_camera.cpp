#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::CopyActiveCameraRaster(
    const std::uint32_t manager, IUiFrameManagerCameraRuntime& runtime,
    IRenderWareImageApi& renderware) {
    const auto subsystem = runtime.InitializeCameraSubsystem();
    const auto camera = runtime.ActiveCameraObject(subsystem);
    if (camera == 0u) return 0u;
    const auto raster = runtime.ReadWord(camera + 0x60u);
    if (raster == 0u) return camera;

    auto image = runtime.ReadWord(manager + 0x148u);
    if (image == 0u) {
        const auto height = runtime.ReadWord(raster + 0x10u);
        const auto width = runtime.ReadWord(raster + 0x0cu);
        image = renderware.Create(width, height, 32u);
        runtime.WriteWord(manager + 0x148u, image);
        (void)renderware.AllocatePixels(image);
    }

    image = runtime.ReadWord(manager + 0x148u);
    const auto width = runtime.ReadWord(raster + 0x0cu);
    const auto image_width = runtime.ReadWord(image + 4u);
    bool resize = image_width != width;
    if (!resize) {
        const auto image_height = runtime.ReadWord(image + 8u);
        const auto raster_height = runtime.ReadWord(raster + 0x10u);
        resize = image_height != raster_height;
    }
    if (resize) {
        renderware.Destroy(image);
        const auto height = runtime.ReadWord(raster + 0x10u);
        const auto new_width = runtime.ReadWord(raster + 0x0cu);
        image = renderware.Create(new_width, height, 32u);
        runtime.WriteWord(manager + 0x148u, image);
        (void)renderware.AllocatePixels(image);
    }
    return renderware.SetFromRaster(
        runtime.ReadWord(manager + 0x148u), raster);
}

}  // namespace pal4
