#include "pal4/script_dialogs.h"

namespace pal4 {

std::uint32_t ScriptDialogs::ShowCommon(std::string_view text,
                                        std::int32_t first,
                                        std::int32_t second) {
    return ShowCommonWithMode(text, first, second, 4u);
}

std::uint32_t ScriptDialogs::ShowCommonInSelectMode(
    std::string_view text, std::int32_t first, std::int32_t second) {
    return ShowCommonWithMode(text, first, second, 3u);
}

std::uint32_t ScriptDialogs::ShowCommonWithMode(
    std::string_view text, std::int32_t first, std::int32_t second,
    std::uint32_t mode) {
    static_cast<void>(runtime_.CreateCommonDialog(runtime_.UiFrameManager()));
    runtime_.ShowCommon(text, first, second, mode, 0u);
    return QueueWaitWithResult(kCommonVtable);
}

void ScriptDialogs::UpdateCommon(ScriptDialogWaitState& state) {
    const auto dialog = runtime_.CreateCommonDialog(runtime_.UiFrameManager());
    if (!runtime_.IsVisible(dialog)) runtime_.CloseWait(state);
}

std::uint32_t ScriptDialogs::ShowSelect(std::string_view text) {
    static_cast<void>(runtime_.CreateSelectDialog(runtime_.UiFrameManager()));
    runtime_.SetSelectQuestion(text, 0u);
    QueueWait(kSelectVtable);

    const auto document = runtime_.CreateDocument();
    try {
        const auto dialog = runtime_.CreateSelectDialog(
            runtime_.UiFrameManager());
        runtime_.UpsertPointerAttribute(document, "frame", dialog);
        runtime_.EnterState(10u, document);
    } catch (...) {
        static_cast<void>(runtime_.DestroyDocument(document));
        throw;
    }
    return runtime_.DestroyDocument(document);
}

void ScriptDialogs::UpdateSelect(ScriptDialogWaitState& state) {
    const auto dialog = runtime_.CreateSelectDialog(runtime_.UiFrameManager());
    if (!runtime_.IsVisible(dialog)) runtime_.CloseWait(state);
}

void ScriptDialogs::QueueWait(std::uint32_t vtable) {
    static_cast<void>(QueueWaitWithResult(vtable));
}

std::uint32_t ScriptDialogs::QueueWaitWithResult(std::uint32_t vtable) {
    auto* state = runtime_.AllocateWait(sizeof(ScriptDialogWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, 1u,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = vtable;
        runtime_.FlushWait(*state);
    }
    return runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset,
                               state);
}

}  // namespace pal4
