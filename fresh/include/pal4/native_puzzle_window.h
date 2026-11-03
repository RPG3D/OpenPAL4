#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;
class NativeGameTimerQueue;
class NativeEventHandlerArray;
class NativeUiFrameUpdate;
class NativeBaseObjectMemory;
class IUiTextImageOverlayRuntime;
struct NativePuzzleWindowServices {
    std::uint32_t manager{},game{};
    std::function<void()> return_scene;
    std::function<void(std::string_view,std::uint32_t)> sound;
};
// Borrows live UI/frame/input/timer/animation owners. Subscribers and frame image
// are owned here; global result animations own their buffers independently.
class NativePuzzleWindow final {
public:
    NativePuzzleWindow(NativeCeguiUiSession&,const NativeGameDatabase&,
        NativeGameTimerQueue&,NativeEventHandlerArray&,NativeUiFrameUpdate&,
        NativeBaseObjectMemory&,IUiTextImageOverlayRuntime&,NativePuzzleWindowServices);
    ~NativePuzzleWindow();
    void Show(std::uint32_t type);
    void Close();
    bool Visible() const;
    std::uint32_t FrameAddress() const;
    bool OwnsActivity(std::uint32_t) const;
    std::uint32_t Callback(std::uint32_t,std::uint32_t);
    std::uint8_t Predicate(std::uint32_t,std::uint32_t,std::uint32_t);
    bool HandleTimer(std::uint32_t owner,std::uint32_t method);
    void RethrowEventError();
    std::int32_t Counter() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace pal4
