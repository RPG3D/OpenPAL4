#include "pal4/collection_camera_bootstrap.h"

namespace pal4 {

namespace {

constexpr std::string_view kLogFormat =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  "
    "uiSceneManager::initializeUiCamera OK\n\n\n";
constexpr std::string_view kSourcePath =
    "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\PAL4Game\\"
    "UiFrame\\uiScene\\uiSceneManager.cpp";

}  // namespace

std::uint32_t CollectionCameraBootstrap::InitializeAndSetup(
    CollectionState& state, ICollectionCameraRuntime& runtime) {
    const auto manager = runtime.InitializeCameraSubsystem();
    const auto camera = runtime.FindCamera(manager, kCameraName);
    if (camera == 0u) return 0u;

    runtime.SetAngles(runtime.InternalObject(camera), kFirstAngleBits,
                      kSecondAngleBits, 0u, 1u);
    runtime.SetLookAt(runtime.InternalObject(camera), kLookAtBits, 1u);
    runtime.SetScalar(runtime.InternalObject(camera), kDistanceBits, 1u);
    runtime.SetWorldCamera(state, runtime.CameraContainer(camera));
    const auto logger = runtime.InitializeLogger();
    return runtime.AppendLog(logger, kLogFormat, kSourcePath, 154u);
}

std::uint32_t CollectionCameraBootstrap::GetActiveCamera(
    ICollectionCameraRuntime& runtime) {
    const auto manager = runtime.InitializeCameraSubsystem();
    const auto camera = runtime.FindCamera(manager, kCameraName);
    return camera ? runtime.InternalObject(camera) : 0u;
}

}  // namespace pal4
