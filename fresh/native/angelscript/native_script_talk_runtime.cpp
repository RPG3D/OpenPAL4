#include "pal4/native_script_talk_runtime.h"

#include "pal4/native_cegui_ui_session.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

static_assert(sizeof(void*) == sizeof(std::uint32_t));

namespace pal4 {

NativeScriptTalkRuntime::NativeScriptTalkRuntime(
    NativeCeguiUiSession& ui, asIScriptContext& context,
    VoiceFlagReader voice_flag, VoicePlayer play_voice)
    : ui_(ui), context_(context), voice_flag_(std::move(voice_flag)),
      play_voice_(std::move(play_voice)), talk_(*this) {
    if (!voice_flag_)
        throw std::invalid_argument("PAL4 dialog voice flag reader is required");
}

std::uint32_t NativeScriptTalkRuntime::QueueAddress() const noexcept {
    return reinterpret_cast<std::uint32_t>(this) +
           ScriptTalkHandler::kQueueOffset;
}

std::uint8_t NativeScriptTalkRuntime::VoiceFlag() {
    return voice_flag_();
}

std::uint32_t NativeScriptTalkRuntime::ScriptExecutor() {
    return reinterpret_cast<std::uint32_t>(this);
}

std::uint32_t NativeScriptTalkRuntime::AudioManager() {
    return reinterpret_cast<std::uint32_t>(this);
}

std::uint32_t NativeScriptTalkRuntime::PlayMusic(
    const std::uint32_t manager, const std::string_view path,
    const std::uint32_t mode) {
    if (manager != AudioManager() || mode != 1u || !play_voice_)
        throw std::logic_error("PAL4 dialog voice playback is unavailable");
    return play_voice_(path);
}

void NativeScriptTalkRuntime::StoreVoiceHandle(
    const std::uint32_t address, const std::uint32_t handle) {
    if (address != ScriptExecutor() + ScriptTalkHandler::kVoiceHandleOffset)
        throw std::logic_error("PAL4 dialog voice handle address mismatch");
    voice_handle_ = handle;
}

ScenarioDialogWaitState* NativeScriptTalkRuntime::Allocate(
    const std::uint32_t size) {
    if (size != ScriptTalkHandler::kHandlerSize || pending_)
        throw std::logic_error("PAL4 dialog wait allocation mismatch");
    pending_ = std::make_unique<ScenarioDialogWaitState>();
    return pending_.get();
}

void NativeScriptTalkRuntime::ConstructBase(
    ScenarioDialogWaitState& handler, const std::uint32_t wait_mode,
    const std::uint32_t queue_address) {
    if (&handler != pending_.get() || wait_mode != 1u ||
        queue_address != QueueAddress())
        throw std::logic_error("PAL4 dialog wait queue mismatch");
}

void NativeScriptTalkRuntime::Flush(ScenarioDialogWaitState& handler) {
    if (&handler != pending_.get() ||
        handler.vtable_address != ScriptTalkHandler::kVtable)
        throw std::logic_error("PAL4 dialog wait handler is not pending");
}

std::uint32_t NativeScriptTalkRuntime::AddHandler(
    const std::uint32_t queue_address, ScenarioDialogWaitState* handler) {
    if (queue_address != QueueAddress() || handler != pending_.get() ||
        !handler || asGetActiveContext() != &context_ ||
        context_.Suspend() < 0)
        throw std::runtime_error("PAL4 dialog cannot suspend script");
    handlers_.push_back({std::move(pending_), false});
    return 1u;
}

std::uint32_t NativeScriptTalkRuntime::UiFrameManager() {
    return reinterpret_cast<std::uint32_t>(this);
}

std::uint32_t NativeScriptTalkRuntime::CreateScenarioDialog(
    const std::uint32_t manager) {
    if (manager != UiFrameManager())
        throw std::logic_error("PAL4 scenario dialog manager mismatch");
    // IDA4BFA60 creates the persistent dialog on first use, including after
    // a cold archive load that never ran the NewGame UI preparation path.
    if (!ui_.EnsureScenarioDialogLayout())
        throw std::runtime_error("PAL4 scenario dialog creation failed: " + ui_.LastError());
    return manager;
}

std::uint8_t NativeScriptTalkRuntime::IsVisible(
    const std::uint32_t dialog) {
    if (dialog != UiFrameManager())
        throw std::logic_error("PAL4 scenario dialog address mismatch");
    return ui_.ScenarioDialogVisible() ? 1u : 0u;
}

std::uint8_t NativeScriptTalkRuntime::SetText(
    const std::uint32_t dialog, const std::string_view text,
    const std::uint32_t display_value) {
    if (dialog != UiFrameManager() ||
        display_value != ScriptTalkHandler::kDisplayValue)
        throw std::logic_error("PAL4 scenario dialog text arguments mismatch");
    if (!ui_.ShowScenarioDialogText(text, display_value))
        throw std::runtime_error("PAL4 scenario dialog text failed: " +
                                 ui_.LastError());
    return 1u;
}

void NativeScriptTalkRuntime::HideDialog(
    const std::uint32_t dialog, const std::uint32_t first,
    const std::uint32_t second) {
    if (dialog != UiFrameManager() || first != 0u || second != 0u ||
        !ui_.HideScenarioDialog())
        throw std::runtime_error("PAL4 scenario dialog hide failed");
}

void NativeScriptTalkRuntime::SetClosedFlag(
    ScenarioDialogWaitState& handler) {
    const auto found = std::find_if(handlers_.begin(), handlers_.end(),
        [&handler](const Handler& entry) {
            return entry.state.get() == &handler;
        });
    if (found == handlers_.end())
        throw std::logic_error("PAL4 dialog wait handler is not registered");
    found->closed = true;
}

std::uint32_t NativeScriptTalkRuntime::ScenarioDialog(
    const std::uint32_t manager) {
    return CreateScenarioDialog(manager);
}

std::uint32_t NativeScriptTalkRuntime::ReadWord(const std::uint32_t address) {
    return *reinterpret_cast<const std::uint32_t*>(address);
}

std::uint32_t NativeScriptTalkRuntime::EmptyStringAddress() {
    static constexpr char empty[] = "";
    return reinterpret_cast<std::uint32_t>(empty);
}

std::uint8_t NativeScriptTalkRuntime::SetPortrait(
    const std::uint32_t dialog, const std::uint32_t name,
    const std::int32_t argument) {
    if (dialog != UiFrameManager() ||
        !ui_.SetScenarioDialogPortrait(
            reinterpret_cast<const char*>(name), argument != 0))
        throw std::runtime_error("PAL4 scenario portrait failed: " +
                                 ui_.LastError());
    return 1u;
}

bool NativeScriptTalkRuntime::Tick(float) {
    const bool had_handlers = Waiting();
    for (auto& handler : handlers_)
        talk_.Update(*handler.state);
    std::erase_if(handlers_, [](const Handler& handler) {
        return handler.closed;
    });
    return had_handlers && !Waiting();
}


void NativeScriptTalkRuntime::ClearHandlers() {
    if (asGetActiveContext() == &context_)
        throw std::logic_error("PAL4 cannot clear active dialog handlers");
    for (auto& handler : handlers_) talk_.Close(*handler.state);
    handlers_.clear();
    pending_.reset();
}

}  // namespace pal4
