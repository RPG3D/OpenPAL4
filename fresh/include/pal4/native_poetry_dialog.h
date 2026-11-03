#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;
class NativeUiFrameUpdate;
class NativeGameTimerQueue;
class NativePoetryRuntime;
// Owns the real poetry layout and original 184-byte frame image; borrows the
// live CEGUI/frame/timer owners and retires before any of them.
class NativePoetryDialog final {
public:
    NativePoetryDialog(NativeCeguiUiSession&, NativeUiFrameUpdate&, NativeGameTimerQueue&);
    ~NativePoetryDialog();
    void Show(std::uint32_t index);
    void Close();
    bool Visible() const;
    std::uint32_t FrameAddress() const;
    bool HandleTimer(std::uint32_t owner, std::uint32_t method);
private:
    std::unique_ptr<NativePoetryRuntime> runtime_;
};
}
