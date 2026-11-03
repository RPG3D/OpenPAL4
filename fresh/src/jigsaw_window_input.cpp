#include "pal4/jigsaw_window_input.h"

namespace pal4 {
namespace {

void PlayForResult(const std::uint32_t result,
                   const std::uint32_t mode,
                   IJigsawWindowInputRuntime& runtime) {
    if (result == 0u)
        runtime.PlaySound("WA26.wav", mode);
    else if (result == 1u)
        runtime.PlaySound("WA27.wav", mode);
}

}  // namespace

std::uint8_t JigsawWindowInput::OnMouseButtonUp(
    const std::uint32_t frame, const std::uint32_t event,
    IJigsawWindowInputRuntime& runtime) {
    if (runtime.ReadByte(frame + 0x18cu) != 0u) return 1u;
    if (runtime.ReadWord(event + 0x1cu) == 1u) return 1u;

    const auto board = runtime.ReadWord(frame + 0x184u);
    const auto rect = runtime.GetBoardRect(board);
    const std::array<float, 2> origin{rect[2], rect[0]};
    const auto result = runtime.ProcessMouse(
        frame + 0x2cu, origin, runtime.ReadPoint(event + 0xcu));
    runtime.Redraw(board);
    if (runtime.IsComplete(frame + 0x2cu)) {
        runtime.ShowResult(frame, 1u);
        return 1u;
    }
    (void)runtime.UpdateCounter(frame);
    PlayForResult(result, 1u, runtime);
    return 1u;
}

std::uint8_t JigsawWindowInput::OnKeyUp(
    const std::uint32_t handler, const std::uint32_t event,
    IJigsawWindowInputRuntime& runtime) {
    if (runtime.ReadByte(handler + 0x164u) != 0u) return 1u;
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
    if (runtime.IsComplete(puzzle)) {
        runtime.ShowResult(frame, 1u);
        return 1u;
    }
    (void)runtime.UpdateCounter(frame);
    PlayForResult(result, 0u, runtime);
    return 1u;
}

std::uint8_t JigsawWindowInput::OnCloseClick(
    const std::uint32_t frame, IJigsawWindowInputRuntime& runtime) {
    if (runtime.ReadByte(frame + 0x18cu) == 0u)
        (void)runtime.Close(frame, 0u, 0u);
    return 1u;
}

}  // namespace pal4
