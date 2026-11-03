#include "pal4/script_talk_handler.h"

#include <string>

namespace pal4 {

std::uint8_t ScriptTalkHandlerRuntimeStub::VoiceFlag() { return 0u; }
std::uint32_t ScriptTalkHandlerRuntimeStub::ScriptExecutor() { return 0u; }
std::uint32_t ScriptTalkHandlerRuntimeStub::AudioManager() { return 0u; }
std::uint32_t ScriptTalkHandlerRuntimeStub::PlayMusic(
    std::uint32_t, std::string_view, std::uint32_t) { return 0u; }
void ScriptTalkHandlerRuntimeStub::StoreVoiceHandle(
    std::uint32_t, std::uint32_t) {}
ScenarioDialogWaitState* ScriptTalkHandlerRuntimeStub::Allocate(std::uint32_t) {
    return nullptr;
}
void ScriptTalkHandlerRuntimeStub::ConstructBase(
    ScenarioDialogWaitState&, std::uint32_t, std::uint32_t) {}
void ScriptTalkHandlerRuntimeStub::Flush(ScenarioDialogWaitState&) {}
std::uint32_t ScriptTalkHandlerRuntimeStub::AddHandler(
    std::uint32_t, ScenarioDialogWaitState*) { return 0u; }
std::uint32_t ScriptTalkHandlerRuntimeStub::UiFrameManager() { return 0u; }
std::uint32_t ScriptTalkHandlerRuntimeStub::CreateScenarioDialog(
    std::uint32_t) { return 0u; }
std::uint8_t ScriptTalkHandlerRuntimeStub::IsVisible(std::uint32_t) {
    return 0u;
}
std::uint8_t ScriptTalkHandlerRuntimeStub::SetText(
    std::uint32_t, std::string_view, std::uint32_t) { return 0u; }
void ScriptTalkHandlerRuntimeStub::HideDialog(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void ScriptTalkHandlerRuntimeStub::SetClosedFlag(ScenarioDialogWaitState&) {}

std::uint32_t ScriptTalkHandler::ScenarioDialog() {
    return runtime_.CreateScenarioDialog(runtime_.UiFrameManager());
}

std::uint8_t ScriptTalkHandler::Invoke(std::string_view text,
                                        std::string_view voice_id) {
    if (runtime_.VoiceFlag() == 1u && !voice_id.empty()) {
        std::string path(kVoiceDirectory);
        path.append(voice_id);
        path.append(".mp3");
        const auto executor = runtime_.ScriptExecutor();
        const auto audio_manager = runtime_.AudioManager();
        const auto handle = runtime_.PlayMusic(audio_manager, path, 1u);
        runtime_.StoreVoiceHandle(executor + kVoiceHandleOffset, handle);
    }

    auto* handler = runtime_.Allocate(kHandlerSize);
    if (handler != nullptr) {
        runtime_.ConstructBase(*handler, 1u,
                               runtime_.ScriptExecutor() + kQueueOffset);
        handler->vtable_address = kVtable;
        runtime_.Flush(*handler);
    }
    static_cast<void>(runtime_.AddHandler(
        runtime_.ScriptExecutor() + kQueueOffset, handler));

    const auto dialog = ScenarioDialog();
    const auto visible = runtime_.IsVisible(dialog);
    if (visible != 0u) return visible;
    return runtime_.SetText(dialog, text, kDisplayValue);
}

std::uint32_t ScriptTalkHandler::Wait() {
    auto* handler = runtime_.Allocate(kHandlerSize);
    if (handler != nullptr) {
        runtime_.ConstructBase(*handler, 1u,
                               runtime_.ScriptExecutor() + kQueueOffset);
        handler->vtable_address = kVtable;
        runtime_.Flush(*handler);
    }
    return runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset,
                               handler);
}

void ScriptTalkHandler::Update(ScenarioDialogWaitState& handler) {
    if (runtime_.IsVisible(ScenarioDialog()) == 0u)
        runtime_.SetClosedFlag(handler);
}

void ScriptTalkHandler::Close(ScenarioDialogWaitState& handler) {
    runtime_.HideDialog(ScenarioDialog(), 0u, 0u);
    runtime_.SetClosedFlag(handler);
}

}  // namespace pal4
