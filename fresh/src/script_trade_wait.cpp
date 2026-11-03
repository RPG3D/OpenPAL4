#include "pal4/script_trade_wait.h"

namespace pal4 {

std::uint32_t ScriptTradeWait::Start(std::string_view shop_id,
                                     std::string_view context) {
    runtime_.SetTradeItemInfo(runtime_.TradeSystem(), shop_id, context);
    auto* state = runtime_.AllocateWait(sizeof(TradeWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, 1u,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = kWaitVtable;
        runtime_.FlushWait(*state);
    }
    return runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset,
                               state);
}

void ScriptTradeWait::Update(TradeWaitState& state) {
    if (ScriptCurrentStateWait::Complete(runtime_))
        runtime_.CloseWait(state);
}

}  // namespace pal4
