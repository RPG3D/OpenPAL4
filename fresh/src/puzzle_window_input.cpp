#include "pal4/puzzle_window_input.h"

namespace pal4 {
namespace {

void PlayForResult(const std::uint32_t result,
                   const std::uint32_t mode,
                   IPuzzleWindowInputRuntime& runtime) {
    if (result == 0u)
        runtime.PlaySound("WA26.wav", mode);
    else if (result == 1u)
        runtime.PlaySound("WA27.wav", mode);
}

}  // namespace

std::uint8_t PuzzleWindowInput::OnRender(
    const std::uint32_t frame, IPuzzleWindowInputRuntime& runtime) {
    const auto renderer = runtime.Renderer();
    const auto rect = runtime.GetBoardRect(runtime.ReadWord(frame + 0x184u));
    runtime.RenderPuzzle(frame + 0x2cu, {rect[2], rect[0]}, renderer);
    return 1u;
}

std::uint8_t PuzzleWindowInput::OnMouseButtonUp(
    const std::uint32_t frame, const std::uint32_t event,
    IPuzzleWindowInputRuntime& runtime) {
    if (runtime.ReadByte(frame + 0x190u) != 0u) return 1u;
    if (runtime.ReadWord(event + 0x1cu) == 1u) return 1u;

    const auto board = runtime.ReadWord(frame + 0x184u);
    const auto rect = runtime.GetBoardRect(board);
    const auto result = runtime.ProcessMouse(
        frame + 0x2cu, {rect[2], rect[0]},
        runtime.ReadPoint(event + 0xcu));
    runtime.Redraw(board);
    (void)runtime.UpdateDisplay(frame);
    if (runtime.IsComplete(frame + 0x2cu)) {
        runtime.ShowResult(frame, 1u);
        return 1u;
    }
    PlayForResult(result, 1u, runtime);
    return 1u;
}

std::uint8_t PuzzleWindowInput::OnKeyUp(
    const std::uint32_t handler, const std::uint32_t event,
    IPuzzleWindowInputRuntime& runtime) {
    if (runtime.ReadByte(handler + 0x168u) != 0u) return 1u;
    const auto key = runtime.ReadWord(event + 0x10u);
    const auto puzzle = handler + 4u;
    std::uint32_t result = 2u;
    switch (key) {
    case 203u:
        result = runtime.ProcessDirection(puzzle, JigsawDirection::Left);
        break;
    case 205u:
        result = runtime.ProcessDirection(puzzle, JigsawDirection::Right);
        break;
    case 200u:
        result = runtime.ProcessDirection(puzzle, JigsawDirection::Up);
        break;
    case 208u:
        result = runtime.ProcessDirection(puzzle, JigsawDirection::Down);
        break;
    case 1u:
        if (runtime.ReadWord(handler + 0x44u) == 0u) {
            (void)runtime.Close(handler - 0x28u, 0u, 0u);
            return 1u;
        }
        runtime.ResetSelection(puzzle);
        break;
    case 28u:
        runtime.ConfirmSelection(puzzle);
        break;
    default:
        return 1u;
    }

    runtime.Redraw(runtime.ReadWord(handler + 0x15cu));
    const auto frame = handler - 0x28u;
    (void)runtime.UpdateDisplay(frame);
    if (runtime.IsComplete(puzzle)) {
        runtime.ShowResult(frame, 1u);
        return 1u;
    }
    PlayForResult(result, 0u, runtime);
    return 1u;
}

std::uint8_t PuzzleWindowInput::OnCloseClick(
    const std::uint32_t frame, IPuzzleWindowInputRuntime& runtime) {
    if (runtime.ReadByte(frame + 0x190u) == 0u)
        (void)runtime.Close(frame, 0u, 0u);
    return 1u;
}

}  // namespace pal4
