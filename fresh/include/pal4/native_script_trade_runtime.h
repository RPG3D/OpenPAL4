#pragma once
#include "pal4/native_script_frame_wait_runtime.h"
#include "pal4/script_trade_wait.h"
#include <angelscript.h>
#include <functional>
namespace pal4 {
struct NativeScriptTradeServices {
    std::function<void(std::string_view shop,std::string_view task)> start;
    std::function<std::uint32_t()> current_state;
    std::function<std::uint32_t(std::uint32_t state)> state_id;
};
// One wait belongs to one SDK context. The UI/state services are borrowed and
// may outlive this context; cancellation does not close a foreign/global shop.
class NativeScriptTradeRuntime final : public IScriptTradeWaitRuntime,
                                       public INativeScriptHandlerWaitRuntime {
public:
    NativeScriptTradeRuntime(asIScriptContext&,NativeScriptTradeServices);
    bool BlocksScript()const override{return active_;}
    bool Tick(float elapsed)override;
    void ClearHandlers();
    std::uint32_t TradeSystem()override;
    void SetTradeItemInfo(std::uint32_t,std::string_view,std::string_view)override;
    TradeWaitState* AllocateWait(std::uint32_t)override;
    std::uint32_t ScriptExecutor()override;
    void ConstructWait(TradeWaitState&,std::uint8_t,std::uint32_t)override;
    void FlushWait(TradeWaitState&)override;
    std::uint32_t AddHandler(std::uint32_t,TradeWaitState*)override;
    std::uint32_t TradeUi()override;
    std::int32_t TradeUiCompletion(std::uint32_t)override;
    void CloseWait(TradeWaitState&)override;
private:
    asIScriptContext& context_;NativeScriptTradeServices services_;
    ScriptTradeWait trade_{*this};TradeWaitState state_{};bool active_{};
};
}
