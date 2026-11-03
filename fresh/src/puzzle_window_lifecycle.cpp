#include "pal4/puzzle_window_lifecycle.h"

#include <cstdint>

namespace pal4 {

std::uint32_t PuzzleWindowStorage::Destruct(
    const std::uint32_t frame, IPuzzleWindowStorageRuntime& runtime) {
    runtime.ReleaseStepName(frame + 0x194u);
    return JigsawWindowStorage::Destruct(frame, runtime);
}

std::uint32_t PuzzleWindowUi::SetVisible(
    const std::uint32_t frame, const std::uint8_t active,
    const std::uint32_t parent, IPuzzleWindowUiRuntime& runtime) {
    runtime.SetParent(frame, active, parent);
    runtime.WriteByte(frame + 0x190u, 0u);
    const auto array = runtime.FrameManager() + 0x134u;
    if (active != 0u)
        return runtime.InsertActive(array, frame + 0x28u);

    runtime.RemoveActive(array, frame + 0x28u);
    const auto manager = runtime.FrameManager();
    runtime.RemoveTimerByName(runtime.ReadWord(manager + 0x1a0u),
                              frame + 0x194u);
    const auto document = runtime.CreateXmlDocument();
    try {
        runtime.InitializeStateAndUI(runtime.Game(), 1u, document);
    } catch (...) {
        (void)runtime.DestroyXmlDocument(document);
        throw;
    }
    return runtime.DestroyXmlDocument(document);
}

std::uint8_t PuzzleWindowUi::HandleShowMessage(
    const std::uint32_t message, IPuzzleWindowUiRuntime& runtime) {
    const auto name = runtime.ReadWord(0x8b12dcu);
    const auto type = runtime.ReadWord(message);
    runtime.HandleFrameEvent(name, type, 0u);
    if (runtime.ReadWord(message) == 1u) {
        const auto step = runtime.ReadWord(message + 4u);
        if (static_cast<std::int32_t>(step) < 6) {
            const auto frame = runtime.FindFrame(runtime.FrameManager(), name);
            if (frame != 0u) runtime.SetStepCount(frame, step);
        }
    }
    return 1u;
}

std::uint8_t PuzzleWindowStep::SetStepCount(
    const std::uint32_t frame, const std::uint32_t step,
    IPuzzleWindowStepRuntime& runtime) {
    runtime.ProcessPuzzleStep(frame + 0x2cu, step);
    runtime.SetTextFromString(runtime.ReadWord(frame + 0x18cu),
                              frame + 0xc4u);
    if (runtime.ReadByte(frame + 0x78u) == 0u) {
        const auto manager = runtime.FrameManager();
        runtime.ScheduleTimer(runtime.ReadWord(manager + 0x1a0u),
                              frame + 0x194u, 0x3f800000u,
                              frame, 0x4a1710u, 0u);
    }
    return static_cast<std::uint8_t>(runtime.UpdateDisplay(frame));
}

}  // namespace pal4
