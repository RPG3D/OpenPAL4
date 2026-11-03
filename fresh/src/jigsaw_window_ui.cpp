#include "pal4/jigsaw_window_ui.h"

#include <cstdint>

namespace pal4 {

std::uint32_t JigsawWindowUi::SetVisible(
    const std::uint32_t frame, const std::uint8_t active,
    const std::uint32_t parent, IJigsawWindowUiRuntime& runtime) {
    runtime.SetParent(frame, active, parent);
    runtime.WriteByte(frame + 0x18cu, 0u);
    const auto array = runtime.FrameManager() + 0x134u;
    if (active != 0u)
        return runtime.InsertActive(array, frame + 0x28u);

    runtime.RemoveActive(array, frame + 0x28u);
    const auto document = runtime.CreateXmlDocument();
    try {
        runtime.InitializeStateAndUI(runtime.Game(), 1u, document);
    } catch (...) {
        (void)runtime.DestroyXmlDocument(document);
        throw;
    }
    return runtime.DestroyXmlDocument(document);
}

std::uint8_t JigsawWindowUi::HandleShowMessage(
    const std::uint32_t message, IJigsawWindowUiRuntime& runtime) {
    const auto name = runtime.ReadWord(0x8b123cu);
    const auto type = runtime.ReadWord(message);
    runtime.HandleFrameEvent(name, type, 0u);
    if (runtime.ReadWord(message) == 1u) {
        const auto step = runtime.ReadWord(message + 4u);
        if (static_cast<std::int32_t>(step + 6u) < 18) {
            const auto frame = runtime.FindFrame(runtime.FrameManager(), name);
            if (frame != 0u) runtime.SetStepWithTimer(frame, step);
        }
    }
    return 1u;
}

}  // namespace pal4
