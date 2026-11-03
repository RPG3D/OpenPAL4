#include "pal4/native_script_trade_runtime.h"
#include <stdexcept>
#include <utility>
namespace pal4 {
NativeScriptTradeRuntime::NativeScriptTradeRuntime(asIScriptContext& c,NativeScriptTradeServices s):context_(c),services_(std::move(s)){
    if(!services_.start || !services_.current_state || !services_.state_id)throw std::invalid_argument("Trade wait requires live UI/current-state services");
}
std::uint32_t NativeScriptTradeRuntime::TradeSystem(){return reinterpret_cast<std::uint32_t>(this);}
std::uint32_t NativeScriptTradeRuntime::ScriptExecutor(){return TradeSystem();}
void NativeScriptTradeRuntime::SetTradeItemInfo(std::uint32_t owner,std::string_view shop,std::string_view task){
    if(owner!=TradeSystem() || active_ || asGetActiveContext()!=&context_)throw std::logic_error("Trade wait requires its executing SDK context");
    services_.start(shop,task);
}
TradeWaitState* NativeScriptTradeRuntime::AllocateWait(std::uint32_t bytes){
    if(bytes!=sizeof(state_) || active_)throw std::invalid_argument("Trade wait state allocation");state_={};return &state_;
}
void NativeScriptTradeRuntime::ConstructWait(TradeWaitState& s,std::uint8_t mode,std::uint32_t queue){
    if(&s!=&state_ || mode!=1u || queue!=ScriptExecutor()+ScriptTradeWait::kQueueOffset)throw std::invalid_argument("Trade wait queue owner");
}
void NativeScriptTradeRuntime::FlushWait(TradeWaitState& s){
    if(&s!=&state_ || s.vtable_address!=ScriptTradeWait::kWaitVtable)throw std::invalid_argument("Trade wait identity");
}
std::uint32_t NativeScriptTradeRuntime::AddHandler(std::uint32_t queue,TradeWaitState* s){
    if(queue!=ScriptExecutor()+ScriptTradeWait::kQueueOffset || s!=&state_ || active_ || asGetActiveContext()!=&context_ || context_.Suspend()<0)
        throw std::runtime_error("Trade wait cannot suspend its SDK context");active_=true;return 1u;
}
std::uint32_t NativeScriptTradeRuntime::TradeUi(){return services_.current_state();}
std::int32_t NativeScriptTradeRuntime::TradeUiCompletion(std::uint32_t state){return static_cast<std::int32_t>(services_.state_id(state));}
void NativeScriptTradeRuntime::CloseWait(TradeWaitState& s){if(&s!=&state_)throw std::invalid_argument("Trade wait close owner");active_=false;}
bool NativeScriptTradeRuntime::Tick(float){const bool was=active_;if(active_)trade_.Update(state_);return was && !active_;}
void NativeScriptTradeRuntime::ClearHandlers(){if(asGetActiveContext()==&context_)throw std::logic_error("Cannot retire the executing Trade wait");active_=false;state_={};}
}
