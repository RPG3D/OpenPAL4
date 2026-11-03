#include "pal4/screenshot_capture.h"

namespace pal4 {

std::uint32_t ScreenshotCapture::Capture(
    IScreenshotCaptureRuntime& runtime,
    IRenderWareImageApi& renderware) {
    const auto subsystem = runtime.InitializeCameraSubsystem();
    const auto camera = runtime.ActiveCameraObject(subsystem);
    const auto raster = runtime.ReadWord(camera + 0x60u);
    const auto height = runtime.ReadWord(raster + 0x10u);
    const auto width = runtime.ReadWord(raster + 0x0cu);
    const auto image = renderware.Create(width, height, 32u);
    if (image == 0u) return 0u;

    const auto pixels = renderware.AllocatePixels(image);
    if (pixels == 0u) return 0u;
    (void)renderware.SetFromRaster(pixels, raster);

    const auto time = runtime.CurrentTime();
    const auto local_time = runtime.LocalTime(time);
    const auto filename = runtime.InitializeFilename();
    try {
        const auto path = runtime.FormatFilename(
            filename, "snapshoot\\%d_%d_%d__%d_%d_%d.bmp",
            local_time.year_from_1900 + 1900,
            local_time.month_zero_based + 1, local_time.day,
            local_time.hour, local_time.minute, local_time.second);
        const auto original_directory = runtime.CurrentDirectory(0x100u);
        if (!runtime.SetCurrentDirectory("snapshoot"))
            runtime.CreateDirectory("snapshoot");
        (void)runtime.SetCurrentDirectory(original_directory);
        runtime.DeleteFile(path);
        (void)renderware.Write(pixels, path.c_str());
        renderware.Destroy(pixels);
    } catch (...) {
        (void)runtime.CleanupFilename(filename);
        throw;
    }
    return runtime.CleanupFilename(filename);
}

}  // namespace pal4
