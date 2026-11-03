#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/rt2d_animation.h"

namespace pal4 {

struct LoadingFrameSingletonState {
    std::uint8_t guard = 0;
    std::uint32_t instance = 0;
};

class ILoadingFrameRuntime {
public:
    virtual ~ILoadingFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t type_info) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CriticalSectionInitialize(std::uint32_t address) = 0;
    virtual void CriticalSectionDestroy(std::uint32_t address) = 0;
    virtual void BoneConstruct(std::uint32_t address) = 0;
    virtual void MovieInitialize(std::uint32_t address) = 0;
    virtual void MovieCleanup(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

struct LoadingFrameViewport {
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t right = 0;
    std::int32_t bottom = 0;
};

class ILoadingFrameCameraRuntime : public IRt2dAnimationRuntime {
public:
    [[nodiscard]] virtual LoadingFrameViewport Viewport() = 0;
    [[nodiscard]] virtual float GameFarDepth() = 0;
    [[nodiscard]] virtual float WindowScale() = 0;
    [[nodiscard]] virtual std::uint32_t WindowFlags() = 0;
    virtual void CameraInitialize(std::uint32_t camera,
                                  std::int32_t width, std::int32_t height,
                                  std::uint32_t mode, float near_depth,
                                  float far_depth) = 0;
    virtual void CameraAdjustViewWindow(std::uint32_t camera,
                                        std::int32_t left, std::int32_t top,
                                        float scale, std::uint32_t flags) = 0;
    virtual void CameraSetParameters(std::uint32_t camera,
                                     const std::uint32_t (&zeroes)[3],
                                     float first, float second,
                                     std::uint32_t mode, float last) = 0;
    [[nodiscard]] virtual std::uint32_t CameraHandle(
        std::uint32_t camera) = 0;
    [[nodiscard]] virtual int CameraDestroy(std::uint32_t camera) = 0;
};

class ILoadingFrameLoopRuntime {
public:
    virtual ~ILoadingFrameLoopRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void UpdateGameFrame() = 0;
    virtual void SleepMilliseconds(std::uint32_t duration) = 0;
    virtual void DestroyMaestro(std::uint32_t address) = 0;
};

class ILoadingFrameRenderRuntime : public ILoadingFrameRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float GameAnimationScale() = 0;
    virtual void AddMaestroDeltaTime(std::uint32_t, float) {}
    [[nodiscard]] virtual int UpdateMaestroAnimations(std::uint32_t) {
        return 0;
    }
    virtual void ClearCamera(std::uint32_t, std::uint32_t,
                             std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t BeginCameraUpdate(std::uint32_t) {
        return 0;
    }
    virtual void EndCameraUpdate(std::uint32_t) {}
    [[nodiscard]] virtual int ShowGameRaster(std::uint32_t) { return 0; }
    virtual void SetRenderState(std::uint32_t, std::uint32_t) {}
    virtual void PushTransform() {}
    [[nodiscard]] virtual int PopTransform() { return 0; }
    virtual void ObjectTransformChanged(std::uint32_t) {}
    virtual void RenderMaestro(std::uint32_t) {}
};

class LoadingFrame final {
public:
    static constexpr std::uint32_t kSize = 0x328u;
    static constexpr std::uint32_t kVtable = 0x8493e8u;
    static constexpr std::uint32_t kFactory = 0x5aa3b0u;
    static constexpr std::uint32_t kTypeInfo = 0x7de5d0u;

    // IDA 0x5aa370/0x5aa3b0/0x5aa410/0x5aa490.
    [[nodiscard]] static std::uint32_t Get(
        LoadingFrameSingletonState& state, ILoadingFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(ILoadingFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ILoadingFrameRuntime& runtime);
    static void Destruct(std::uint32_t object,
                         ILoadingFrameRuntime& runtime);
    // IDA 0x5aa4f0/0x5aa670.
    [[nodiscard]] static int InitializeCamera(
        std::uint32_t object, ILoadingFrameCameraRuntime& runtime);
    [[nodiscard]] static int CloseCamera(
        std::uint32_t object, ILoadingFrameCameraRuntime& runtime);
    // IDA 0x5aa690.
    [[nodiscard]] static int ProcessLoop(
        std::uint32_t object, ILoadingFrameLoopRuntime& runtime);
    // IDA 0x5aa6e0/0x5aa750/0x5aa760.
    [[nodiscard]] static int Render(
        double delta, LoadingFrameSingletonState& state,
        ILoadingFrameRenderRuntime& runtime);
    [[nodiscard]] static int UpdateMaestro(
        std::uint32_t object, float delta,
        ILoadingFrameRenderRuntime& runtime);
    [[nodiscard]] static int RenderContent(
        std::uint32_t object, std::uint32_t camera_update,
        ILoadingFrameRenderRuntime& runtime);
};

}  // namespace pal4
