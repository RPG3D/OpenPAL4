#include "pal4/opening_video_controller.h"

namespace pal4 {

std::uint32_t OpeningVideoController::Construct(
    std::uint32_t object, IOpeningVideoRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    return object;
}

std::uint32_t OpeningVideoController::Destroy(
    std::uint32_t object, std::uint32_t flags,
    IOpeningVideoRuntime& runtime) {
    InitializeBase(object, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

void OpeningVideoController::InitializeBase(
    std::uint32_t object, IOpeningVideoRuntime& runtime) {
    runtime.WriteWord(object, kBaseVtable);
}

std::uint32_t OpeningVideoController::DestroyBase(
    std::uint32_t object, std::uint32_t flags,
    IOpeningVideoRuntime& runtime) {
    InitializeBase(object, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

std::uint32_t OpeningVideoController::Initialize(
    std::uint32_t object, std::uint32_t state, std::uint32_t,
    IOpeningVideoRuntime& runtime) {
    runtime.SetUIFlag(runtime.UIFrameManager(), 1u);
    runtime.WriteWord(runtime.InitializeConsole(), 0u);
    runtime.WriteWord(object + 4u, state);
    return 1u;
}

void OpeningVideoController::Render(
    std::uint32_t record, IOpeningVideoRuntime& runtime) {
    if (runtime.ReadWord(record) != 0u)
        runtime.RenderVideo(runtime.BinkPlayer(), record);
}

std::uint32_t OpeningVideoController::Update(
    std::uint32_t object, float delta, IOpeningVideoRuntime& runtime) {
    runtime.ProcessScriptWaitHandlers(runtime.ScriptExecutor() + 0x54u,
                               static_cast<double>(delta));
    runtime.AdvanceScreenEffect(runtime.ScreenEffect(), delta);
    const auto playing = runtime.IsPlaying(runtime.BinkPlayer() + 4u);
    if (playing != 0u || runtime.ReadWord(object + 4u) != 1u)
        return playing;

    const auto document = runtime.ConstructXmlDocument();
    try {
        runtime.InitializeStateAndUI(runtime.GameInstance(), 1u, document);
    } catch (...) {
        static_cast<void>(runtime.DestroyXmlDocument(document));
        throw;
    }
    return runtime.DestroyXmlDocument(document);
}

}  // namespace pal4
