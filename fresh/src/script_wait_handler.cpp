#include "pal4/script_wait_handler.h"

namespace pal4 {

ScriptWaitHandlerState* ScriptWaitHandlerRuntimeStub::Allocate(std::uint32_t) {
    return nullptr;
}

std::uint32_t ScriptWaitHandlerRuntimeStub::ScriptExecutor() { return 0u; }
void ScriptWaitHandlerRuntimeStub::ConstructBase(
    ScriptWaitHandlerState&, std::uint32_t, std::uint32_t) {}
void ScriptWaitHandlerRuntimeStub::Flush(ScriptWaitHandlerState&) {}
void ScriptWaitHandlerRuntimeStub::AddHandler(
    std::uint32_t, ScriptWaitHandlerState*) {}
int ScriptWaitHandlerRuntimeStub::SetExecutionFlag(std::uint32_t) { return 0; }
void ScriptWaitHandlerRuntimeStub::SetClosedFlag(ScriptWaitHandlerState&) {}

int ScriptWaitHandler::Create(double duration) {
    auto* handler = runtime_.Allocate(kSize);
    if (handler != nullptr) {
        runtime_.ConstructBase(*handler, 1u,
                               runtime_.ScriptExecutor() + kQueueOffset);
        handler->remaining = duration;
        handler->vtable_address = kVtable;
        runtime_.Flush(*handler);
    }
    runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset, handler);
    return runtime_.SetExecutionFlag(runtime_.ScriptExecutor() + kQueueOffset);
}

void ScriptWaitHandler::Update(ScriptWaitHandlerState& handler,
                               double elapsed) {
    handler.remaining -= elapsed;
    if (handler.remaining <= 0.0) {
        handler.remaining = 0.0;
        runtime_.SetClosedFlag(handler);
    }
}

void ScriptWaitHandler::Close(ScriptWaitHandlerState& handler) {
    handler.remaining = 0.0;
    runtime_.SetClosedFlag(handler);
}

}  // namespace pal4
