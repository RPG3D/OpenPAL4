#pragma once
#include <cstdint>
#include <functional>
namespace pal4 {
class GameRenderState;
class NativeCameraManagerOwner;
class NativeLoadingFramePresenter final {
public:
    // Only Loading UI, using the selected camera and ordinary HWND policy.
    // Never updates Scene or pumps input into partially loaded resources.
    static void Present(GameRenderState&, NativeCameraManagerOwner&,
        const std::function<void(std::uint32_t)>& render);
};
}
