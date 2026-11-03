#include "pal4/global_key_down_handler.h"

namespace pal4 {
namespace {

void DispatchTimer(IGlobalKeyDownRuntime& runtime) {
    const auto manager = runtime.UiManager();
    const auto frame = runtime.ReadWord(manager + 0x144u);
    runtime.DispatchTimerEvent(runtime.UiManager(),
                               GlobalKeyDownHandler::kTimerEventKey,
                               1u, frame);
}

}  // namespace

std::uint8_t GlobalKeyDownHandler::Process(
    const std::uint32_t handler, const std::uint32_t event,
    IGlobalKeyDownRuntime& runtime) {
    const auto initial_mode = runtime.CurrentModeObject();
    if (initial_mode != 0u) (void)runtime.QueryMode(initial_mode);

    const auto key = runtime.ReadWord(event + 0x10u);
    if (key == 66u) {
        (void)runtime.UiManager();
        runtime.CaptureScreenshot();
        return 0u;
    }

    if (key == 67u && runtime.CurrentModeObject() != 0u) {
        const auto mode = runtime.QueryMode(runtime.CurrentModeObject());
        if (mode == 1u) {
            if (runtime.IsTimerFrameVisible(runtime.UiManager())) {
                DispatchTimer(runtime);
                const auto document = runtime.CreateDocument();
                const auto frame = runtime.FindFrame(runtime.UiManager(),
                                                      kFrameKey);
                const auto attribute = runtime.FindAttribute(document, "Frame");
                if (attribute != 0u)
                    runtime.SetAttributeInt(attribute, frame);
                else
                    runtime.AddAttributeInt(document, "Frame", frame);
                runtime.ChangeGameState(10u, document);
                runtime.DestroyDocument(document);
            }
        } else if (mode == 2u || mode == 4u || mode == 5u) {
            DispatchTimer(runtime);
        }
        return 1u;
    }

    if (key == 1u) {
        const auto player = runtime.BinkPlayer();
        if (runtime.BinkIsPlaying(player + 4u)) {
            runtime.CloseVideo(runtime.BinkPlayer());
            return 0u;
        }
    }

    const auto begin = runtime.ReadWord(handler + 4u);
    if (begin != 0u) {
        const auto end = runtime.ReadWord(handler + 8u);
        if (end != begin) {
            const auto last = runtime.ReadWord(end - 4u);
            return runtime.InvokeLastHandler(last, event);
        }
    }
    return 0u;
}

}  // namespace pal4
