#include "pal4/puzzle_window.h"

#include <string>

namespace pal4 {
namespace {

void Subscribe(const std::uint32_t object,
               const std::uint32_t window,
               const PuzzleWindowEvent event,
               const std::uint32_t handler,
               IPuzzleWindowRuntime& runtime) {
    const auto callback = runtime.Allocate(0x18u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, handler);
        runtime.WriteWord(callback + 0xcu, 0u);
        runtime.WriteWord(callback + 0x10u, object);
        runtime.WriteWord(callback, 0x843e94u);
    }
    const auto subscription = runtime.Subscribe(window, event, callback);
    if (subscription != 0u)
        runtime.ReleaseSubscription(subscription);
}

}  // namespace

std::uint32_t PuzzleWindow::Construct(
    const std::uint32_t object, const std::uint32_t layout_name,
    IPuzzleWindowRuntime& runtime) {
    runtime.LoadLayout(object, layout_name);
    runtime.WriteWord(object + 0x28u, 0x8435a4u);
    runtime.InitializePuzzle(object + 0x2cu);
    runtime.SetString(object + 0x194u, "puzzleStep");
    runtime.WriteWord(object, 0x843eb4u);
    runtime.WriteWord(object + 0x28u, 0x843e9cu);

    const std::string prefix(runtime.WindowName(object));
    const auto board = runtime.GetWindow(object, prefix + "StaticBoard");
    runtime.WriteWord(object + 0x184u, board);
    const auto step = runtime.GetWindow(object, prefix + "StaticStep");
    runtime.WriteWord(object + 0x188u, step);
    const auto text = runtime.GetWindow(object, prefix + "StaticText");
    runtime.WriteWord(object + 0x18cu, text);
    runtime.SetStaticTextFormatting(text, 2u, 2u);

    Subscribe(object, board, PuzzleWindowEvent::RenderingEnded,
              0x4a1250u, runtime);
    Subscribe(object, board, PuzzleWindowEvent::MouseButtonUp,
              0x4a12b0u, runtime);
    const auto close = runtime.GetWindow(object, prefix + "BtnClose");
    Subscribe(object, close, PuzzleWindowEvent::Clicked,
              0x4a14a0u, runtime);
    return object;
}

}  // namespace pal4
