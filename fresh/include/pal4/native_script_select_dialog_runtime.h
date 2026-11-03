#pragma once
#include "pal4/script_dialogs.h"
#include "pal4/script_select_dialog.h"
#include "pal4/native_script_frame_wait_runtime.h"
#include <angelscript.h>
#include <functional>

namespace pal4 {
class IGameArenaLoadRuntime;
struct NativeScriptSelectDialogServices {
    std::uint32_t manager{};
    std::function<std::uint32_t()> dialog;
    std::function<void(std::uint32_t,std::string_view)> add_item;
    std::function<void(std::uint32_t,std::string_view)> show;
    std::function<bool(std::uint32_t)> visible;
    std::function<std::int32_t(std::uint32_t)> selection;
    std::function<void(std::uint32_t,std::int32_t)> set_default;
};
// ScriptDialogs owns the recovered show/queue/XML ordering. This adapter owns
// only one real SDK context's wait and borrows the cached UI creating service.
class NativeScriptSelectDialogRuntime final : public IScriptDialogsRuntime,
    private IScriptSelectDialogRuntime, public INativeScriptHandlerWaitRuntime {
public:
    NativeScriptSelectDialogRuntime(asIScriptContext&,IGameArenaLoadRuntime&,
                                    NativeScriptSelectDialogServices);
    void AddItem(std::string_view);
    std::int32_t LastSelection();
    void SetDefaultSelection(std::int32_t);
    bool BlocksScript()const override{return active_;}
    bool Tick(float)override;
    void ClearHandlers();
private:
    std::uint32_t UiFrameManager()override;
    std::uint32_t SelectDialog(std::uint32_t)override;
    std::int32_t AddButton(std::uint32_t,std::uint32_t)override;
    std::uint32_t ReadWord(std::uint32_t)override;
    void WriteWord(std::uint32_t,std::uint32_t)override;
    std::uint32_t CreateSelectDialog(std::uint32_t)override;
    void SetSelectQuestion(std::string_view,std::uint32_t)override;
    ScriptDialogWaitState* AllocateWait(std::uint32_t)override;
    std::uint32_t ScriptExecutor()override;
    void ConstructWait(ScriptDialogWaitState&,std::uint8_t,std::uint32_t)override;
    void FlushWait(ScriptDialogWaitState&)override;
    std::uint32_t AddHandler(std::uint32_t,ScriptDialogWaitState*)override;
    bool IsVisible(std::uint32_t)override;
    void CloseWait(ScriptDialogWaitState&)override;
    std::uint32_t CreateDocument()override;
    void UpsertPointerAttribute(std::uint32_t,std::string_view,std::uint32_t)override;
    void EnterState(std::uint32_t,std::uint32_t)override;
    std::uint32_t DestroyDocument(std::uint32_t)override;
    std::uint32_t CreateCommonDialog(std::uint32_t)override;
    void ShowCommon(std::string_view,std::int32_t,std::int32_t,std::uint32_t,std::uint32_t)override;
    void Executing()const;
    asIScriptContext& context_;IGameArenaLoadRuntime& xml_;
    NativeScriptSelectDialogServices services_;
    ScriptDialogWaitState state_{};std::uint32_t frame_{},document_{};
    const std::string_view* borrowed_text_{};bool active_{};
    ScriptDialogs dialogs_{*this};
};
}
