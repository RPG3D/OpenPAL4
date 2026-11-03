#include "pal4/native_script_common_dialog_runtime.h"
#include <stdexcept>
#include <utility>

namespace pal4 {
namespace {
[[noreturn]] void UnsupportedSelect() {
    throw std::logic_error("CommonDialog adapter does not provide SelectDialog/state10 XML");
}
}
NativeScriptCommonDialogRuntime::NativeScriptCommonDialogRuntime(
    asIScriptContext& context, NativeScriptCommonDialogServices services)
    : context_(context), services_(std::move(services)) {
    if (!services_.manager || !services_.dialog || !services_.show ||
        !services_.visible || !services_.selection)
        throw std::invalid_argument("CommonDialog requires live cached window services");
}
std::uint32_t NativeScriptCommonDialogRuntime::UiFrameManager() { return services_.manager; }
std::uint32_t NativeScriptCommonDialogRuntime::FrameManager() { return UiFrameManager(); }
std::uint32_t NativeScriptCommonDialogRuntime::CreateCommonDialog(std::uint32_t manager) {
    if (manager != services_.manager)
        throw std::invalid_argument("CommonDialog manager owner");
    const auto frame = services_.dialog();
    if (!frame || (active_ && frame_ != frame))
        throw std::logic_error("CommonDialog cached frame unavailable or replaced during wait");
    frame_ = frame;
    return frame;
}
void NativeScriptCommonDialogRuntime::ShowCommon(std::string_view text,
    std::int32_t first, std::int32_t second, std::uint32_t mode, std::uint32_t flags) {
    if (active_ || !frame_ || asGetActiveContext() != &context_ ||
        (mode != 3u && mode != 4u) || flags != 0u)
        throw std::logic_error("CommonDialog show context/mode/owner");
    services_.show(frame_, text, first, second, mode, flags);
}
ScriptDialogWaitState* NativeScriptCommonDialogRuntime::AllocateWait(std::uint32_t bytes) {
    if (bytes != sizeof(state_) || active_)
        throw std::invalid_argument("CommonDialog wait allocation");
    state_ = {}; return &state_;
}
std::uint32_t NativeScriptCommonDialogRuntime::ScriptExecutor() {
    return reinterpret_cast<std::uint32_t>(this);
}
void NativeScriptCommonDialogRuntime::ConstructWait(ScriptDialogWaitState& state,
    std::uint8_t mode, std::uint32_t queue) {
    if (&state != &state_ || mode != 1u || queue != ScriptExecutor() + ScriptDialogs::kQueueOffset)
        throw std::invalid_argument("CommonDialog wait queue owner");
}
void NativeScriptCommonDialogRuntime::FlushWait(ScriptDialogWaitState& state) {
    if (&state != &state_ || state.vtable_address != ScriptDialogs::kCommonVtable)
        throw std::invalid_argument("CommonDialog wait identity");
}
std::uint32_t NativeScriptCommonDialogRuntime::AddHandler(std::uint32_t queue,
    ScriptDialogWaitState* state) {
    if (queue != ScriptExecutor() + ScriptDialogs::kQueueOffset || state != &state_ ||
        active_ || asGetActiveContext() != &context_ || context_.Suspend() < 0)
        throw std::runtime_error("CommonDialog cannot suspend its SDK context");
    active_ = true; return 1u;
}
bool NativeScriptCommonDialogRuntime::IsVisible(std::uint32_t frame) {
    if (!frame_ || frame != frame_) throw std::invalid_argument("CommonDialog visibility owner");
    return services_.visible(frame);
}
void NativeScriptCommonDialogRuntime::CloseWait(ScriptDialogWaitState& state) {
    if (&state != &state_) throw std::invalid_argument("CommonDialog wait close owner");
    active_ = false;
}
std::int32_t NativeScriptCommonDialogRuntime::LastSelection(std::uint32_t frame) {
    if (!frame_ || frame != frame_ || asGetActiveContext() != &context_)
        throw std::logic_error("CommonDialog selection context/owner");
    return services_.selection(frame);
}
bool NativeScriptCommonDialogRuntime::Tick(float) {
    const bool was = active_;
    if (active_) dialogs_.UpdateCommon(state_);
    return was && !active_;
}
void NativeScriptCommonDialogRuntime::ClearHandlers() {
    if (asGetActiveContext() == &context_)
        throw std::logic_error("Cannot retire executing CommonDialog wait");
    active_ = false; state_ = {}; frame_ = 0u;
}
std::uint32_t NativeScriptCommonDialogRuntime::CreateSelectDialog(std::uint32_t) { UnsupportedSelect(); }
void NativeScriptCommonDialogRuntime::SetSelectQuestion(std::string_view, std::uint32_t) { UnsupportedSelect(); }
std::uint32_t NativeScriptCommonDialogRuntime::CreateDocument() { UnsupportedSelect(); }
void NativeScriptCommonDialogRuntime::UpsertPointerAttribute(std::uint32_t, std::string_view, std::uint32_t) { UnsupportedSelect(); }
void NativeScriptCommonDialogRuntime::EnterState(std::uint32_t, std::uint32_t) { UnsupportedSelect(); }
std::uint32_t NativeScriptCommonDialogRuntime::DestroyDocument(std::uint32_t) { UnsupportedSelect(); }
}
