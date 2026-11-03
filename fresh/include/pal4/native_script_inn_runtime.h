#pragma once
#include "pal4/script_inn_dialog.h"
#include "pal4/native_script_frame_wait_runtime.h"
#include <angelscript.h>
#include <functional>
namespace pal4 {
class IGameArenaLoadRuntime;
struct NativeScriptInnServices {
    std::uint32_t manager{};
    std::function<void(std::uint32_t)> show;
    std::function<std::uint32_t()> frame;
    std::function<std::int32_t(std::uint32_t)> result;
    std::function<std::uint32_t()> current_state_id;
};
// One state10 wait per real SDK context; UI close alone cannot resume the VM.
class NativeScriptInnRuntime final : public IScriptInnDialogRuntime,public INativeScriptHandlerWaitRuntime {
public:
    NativeScriptInnRuntime(asIScriptContext&,IGameArenaLoadRuntime&,NativeScriptInnServices);
    bool BlocksScript()const override{return active_;}
    bool Tick(float)override;
    void ClearHandlers();
private:
    std::uint32_t FrameManager()override;
    void ProcessEvent(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)override;
    std::uint32_t FindFrame(std::uint32_t,std::uint32_t)override;
    std::int32_t ReadSignedWord(std::uint32_t)override;
    std::uint32_t InitializeDocument()override;
    std::uint32_t DestroyDocument(std::uint32_t)override;
    void BeginFrameName(std::string_view)override;
    void EndFrameName()override;
    ScriptFrameAttributeLookup FindAttribute(std::uint32_t,std::string_view)override;
    std::uint32_t CreateFrameAttribute(std::string_view,std::uint32_t)override;
    void AddAttribute(std::uint32_t,std::uint32_t)override;
    void DestroyAttribute(std::uint32_t)override;
    void SetExistingAttribute(std::uint32_t,std::uint32_t)override;
    std::uint32_t Game()override;
    void InitializeStateAndUi(std::uint32_t,std::uint32_t,std::uint32_t)override;
    asIScriptContext& context_;IGameArenaLoadRuntime& xml_;NativeScriptInnServices services_;
    std::uint32_t document_{},frame_{};bool active_{},name_active_{};
};
}
