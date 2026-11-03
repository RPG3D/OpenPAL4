#pragma once

#include "pal4/script_dialogs.h"
#include "pal4/script_common_dialog_selection.h"
#include "pal4/native_script_frame_wait_runtime.h"
#include <angelscript.h>
#include <functional>

namespace pal4 {
struct NativeScriptCommonDialogServices {
    std::uint32_t manager{};
    std::function<std::uint32_t()> dialog;
    std::function<void(std::uint32_t, std::string_view, std::int32_t,
                       std::int32_t, std::uint32_t, std::uint32_t)> show;
    std::function<bool(std::uint32_t)> visible;
    std::function<std::int32_t(std::uint32_t)> selection;
};

// CommonDialog stays in Scene. Its shared wait polls the actual cached window;
// only the invoking SDK context is suspended. SelectDialog is a separate frame.
class NativeScriptCommonDialogRuntime final : public IScriptDialogsRuntime,
    public IScriptCommonDialogSelectionRuntime, public INativeScriptHandlerWaitRuntime {
public:
    NativeScriptCommonDialogRuntime(asIScriptContext&, NativeScriptCommonDialogServices);
    bool BlocksScript() const override { return active_; }
    bool Tick(float) override;
    void ClearHandlers();
private:
    std::uint32_t UiFrameManager() override;
    std::uint32_t FrameManager() override;
    std::uint32_t CreateCommonDialog(std::uint32_t) override;
    void ShowCommon(std::string_view, std::int32_t, std::int32_t,
                    std::uint32_t, std::uint32_t) override;
    ScriptDialogWaitState* AllocateWait(std::uint32_t) override;
    std::uint32_t ScriptExecutor() override;
    void ConstructWait(ScriptDialogWaitState&, std::uint8_t, std::uint32_t) override;
    void FlushWait(ScriptDialogWaitState&) override;
    std::uint32_t AddHandler(std::uint32_t, ScriptDialogWaitState*) override;
    bool IsVisible(std::uint32_t) override;
    void CloseWait(ScriptDialogWaitState&) override;
    std::int32_t LastSelection(std::uint32_t) override;
    std::uint32_t CreateSelectDialog(std::uint32_t) override;
    void SetSelectQuestion(std::string_view, std::uint32_t) override;
    std::uint32_t CreateDocument() override;
    void UpsertPointerAttribute(std::uint32_t, std::string_view, std::uint32_t) override;
    void EnterState(std::uint32_t, std::uint32_t) override;
    std::uint32_t DestroyDocument(std::uint32_t) override;
    asIScriptContext& context_;
    NativeScriptCommonDialogServices services_;
    ScriptDialogWaitState state_{};
    std::uint32_t frame_{};
    bool active_{};
    ScriptDialogs dialogs_{*this};
};
}
