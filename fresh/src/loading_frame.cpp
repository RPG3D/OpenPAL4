#include "pal4/loading_frame.h"

namespace pal4 {

std::uint32_t LoadingFrame::Get(LoadingFrameSingletonState& state,
                                ILoadingFrameRuntime& runtime) {
    if ((state.guard & 1u) != 0u) return state.instance;
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    state.instance = runtime.RegisterSingleton(
        runtime.SingletonManager(), "palLoadingFrame", 1048u,
        kFactory, kTypeInfo);
    return state.instance;
}

std::uint32_t LoadingFrame::Create(ILoadingFrameRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t LoadingFrame::Construct(const std::uint32_t object,
                                      ILoadingFrameRuntime& runtime) {
    runtime.CriticalSectionInitialize(object);
    try {
        runtime.BoneConstruct(object + 0x28u);
        runtime.MovieInitialize(object + 0x3cu);
    } catch (...) {
        runtime.CriticalSectionDestroy(object);
        throw;
    }
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x38u, 0);
    return object;
}

void LoadingFrame::Destruct(const std::uint32_t object,
                            ILoadingFrameRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    try {
        runtime.MovieCleanup(object + 0x3cu);
    } catch (...) {
        runtime.CriticalSectionDestroy(object);
        throw;
    }
    runtime.CriticalSectionDestroy(object);
}

int LoadingFrame::InitializeCamera(
    const std::uint32_t object, ILoadingFrameCameraRuntime& runtime) {
    const auto bounds = runtime.Viewport();
    const auto camera = object + 0x3cu;
    const float far_depth = runtime.GameFarDepth();
    const float near_depth = runtime.GameDepth();
    runtime.CameraInitialize(camera, bounds.right - bounds.left,
                             bounds.bottom - bounds.top, 1u,
                             near_depth, far_depth);
    runtime.CameraAdjustViewWindow(camera, bounds.left, bounds.top,
                                   runtime.WindowScale(), runtime.WindowFlags());
    const std::uint32_t zeroes[3] = {0u, 0u, 0u};
    runtime.CameraSetParameters(camera, zeroes, 225.0f, 30.0f, 0u, 350.0f);
    static_cast<void>(Rt2dAnimation::Initialize(runtime));
    return Rt2dAnimation::SetCamera(runtime.CameraHandle(camera), runtime);
}

int LoadingFrame::CloseCamera(
    const std::uint32_t object, ILoadingFrameCameraRuntime& runtime) {
    static_cast<void>(Rt2dAnimation::Close(runtime));
    return runtime.CameraDestroy(object + 0x3cu);
}

int LoadingFrame::ProcessLoop(
    const std::uint32_t object, ILoadingFrameLoopRuntime& runtime) {
    if (runtime.ReadByte(object + 0x1du) == 0) return 0;
    while (runtime.ReadByte(object + 0x1cu) == 0) {
        runtime.UpdateGameFrame();
        if (runtime.ReadByte(object + 0x38u) != 0) {
            runtime.DestroyMaestro(object + 0x28u);
            return 0;
        }
        runtime.SleepMilliseconds(30u);
        if (runtime.ReadByte(object + 0x1du) == 0) return 0;
    }
    return 0;
}

int LoadingFrame::Render(
    const double delta, LoadingFrameSingletonState& state,
    ILoadingFrameRenderRuntime& runtime) {
    const auto first = Get(state, runtime);
    const float scaled_delta = static_cast<float>(
        static_cast<double>(runtime.GameAnimationScale()) * delta);
    static_cast<void>(UpdateMaestro(first, scaled_delta, runtime));

    const auto frame = Get(state, runtime);
    const auto camera = runtime.ReadWord(frame + 0x3cu);
    if (camera == 0u) return static_cast<int>(frame + 0x3cu);

    runtime.ClearCamera(camera, 0x8fdde8u, 3u);
    const auto updated_camera = runtime.BeginCameraUpdate(camera);
    if (updated_camera == 0u) return 0;

    static_cast<void>(RenderContent(Get(state, runtime), updated_camera,
                                    runtime));
    runtime.EndCameraUpdate(updated_camera);
    return runtime.ShowGameRaster(updated_camera);
}

int LoadingFrame::UpdateMaestro(
    const std::uint32_t object, const float delta,
    ILoadingFrameRenderRuntime& runtime) {
    const auto maestro = runtime.ReadWord(object + 0x28u);
    if (maestro == 0u) return 0;
    runtime.AddMaestroDeltaTime(maestro, delta);
    return runtime.UpdateMaestroAnimations(
        runtime.ReadWord(object + 0x28u));
}

int LoadingFrame::RenderContent(
    const std::uint32_t object, const std::uint32_t,
    ILoadingFrameRenderRuntime& runtime) {
    const auto maestro_state = object + 0x28u;
    const auto active = runtime.ReadWord(maestro_state + 8u);
    if (active == 0u) return 0;

    constexpr std::uint32_t states[][2] = {
        {8u, 0u}, {6u, 0u}, {7u, 2u}, {9u, 2u}, {12u, 1u},
        {20u, 1u}, {10u, 5u}, {11u, 6u}, {9u, 2u}, {12u, 1u}};
    for (const auto& state : states) {
        runtime.SetRenderState(state[0], state[1]);
    }
    runtime.PushTransform();
    const auto maestro = runtime.ReadWord(maestro_state);
    if (maestro != 0u) {
        if (runtime.ReadWord(maestro_state + 12u) != 0u) {
            runtime.ObjectTransformChanged(
                runtime.ReadWord(maestro_state + 4u));
            runtime.WriteWord(maestro_state + 12u, 0u);
        }
        runtime.RenderMaestro(runtime.ReadWord(maestro_state));
    }
    return runtime.PopTransform();
}

}  // namespace pal4
