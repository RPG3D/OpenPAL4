#include "pal4/loading_progress_updater.h"

namespace pal4 {

void LoadingProgressUpdater::Update(const float progress,
                                    ILoadingProgressRuntime& runtime) {
    if (!runtime.HandleVideoModeChange()) return;

    const auto window = runtime.CreateLoadingWindow(
        runtime.GetUiFrameManager());
    if (window != 0u) {
        runtime.UpdateLoadingWindow(window, progress);
        runtime.UpdateUi(runtime.GetUiFrameManager(),
                         0.1000000014901161);

        const auto game = runtime.GetGameController();
        const auto state = runtime.ReadWord(game + 0x18cu);
        if (state != 0u && runtime.GetStateType(state) == 6u) {
            const auto camera_system = runtime.InitializeCameraSubsystem();
            const auto camera_slot = runtime.GetActiveCameraSlot(camera_system);
            runtime.ClearCamera(runtime.ReadWord(camera_slot), 0x8f6048u, 3u);
            runtime.BeginCameraUpdate(runtime.ReadWord(camera_slot));
            runtime.SetRenderState(20u, 2u);
            runtime.SetRenderState(6u, 1u);
            runtime.SetRenderState(8u, 1u);
            runtime.RenderState(state, camera_slot);
            runtime.EndCameraUpdate(runtime.ReadWord(camera_slot));
            runtime.ShowGameRaster(runtime.ReadWord(camera_slot));
        }
    }

    runtime.HandleDeviceLoss();
}

}  // namespace pal4
