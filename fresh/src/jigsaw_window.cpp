#include "pal4/jigsaw_window.h"

#include <string>

namespace pal4 {
namespace {

void Subscribe(const std::uint32_t object,
               const std::uint32_t window,
               const JigsawWindowEvent event,
               const std::uint32_t handler,
               IJigsawWindowRuntime& runtime) {
    const auto callback = runtime.Allocate(0x18u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, handler);
        runtime.WriteWord(callback + 0xcu, 0u);
        runtime.WriteWord(callback + 0x10u, object);
        runtime.WriteWord(callback, 0x843e3cu);
    }
    const auto subscription = runtime.Subscribe(window, event, callback);
    if (subscription != 0u)
        runtime.ReleaseSubscription(subscription);
}

}  // namespace

std::uint32_t JigsawWindow::Construct(
    const std::uint32_t object, const std::uint32_t layout_name,
    IJigsawWindowRuntime& runtime) {
    runtime.LoadLayout(object, layout_name);
    runtime.WriteWord(object + 0x28u, 0x8435a4u);
    runtime.InitializePuzzle(object + 0x2cu);
    runtime.WriteWord(object, 0x843e5cu);
    runtime.WriteWord(object + 0x28u, 0x843e44u);

    const std::string prefix(runtime.WindowName(object));
    const auto board = runtime.GetWindow(object, prefix + "StaticBoard");
    runtime.WriteWord(object + 0x184u, board);
    const auto step = runtime.GetWindow(object, prefix + "StaticStep");
    runtime.WriteWord(object + 0x188u, step);

    Subscribe(object, board, JigsawWindowEvent::RenderingEnded,
              0x4a1250u, runtime);
    Subscribe(object, board, JigsawWindowEvent::MouseButtonUp,
              0x4a00a0u, runtime);
    const auto close = runtime.GetWindow(object, prefix + "BtnClose");
    Subscribe(object, close, JigsawWindowEvent::Clicked,
              0x4a0290u, runtime);
    return object;
}

}  // namespace pal4
