#pragma once

#include "pal4/native_script_frame_wait_runtime.h"
#include "pal4/script_wait_handler.h"

#include <angelscript.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace pal4 {

class NativeScriptTimedWaitRuntime final : public IScriptWaitHandlerRuntime,
                                           public INativeScriptHandlerWaitRuntime {
public:
    explicit NativeScriptTimedWaitRuntime(asIScriptContext& context)
        : context_(context), handler_(*this) {}

    [[nodiscard]] bool Waiting() const { return !handlers_.empty(); }
    [[nodiscard]] bool BlocksScript() const override { return Waiting(); }
    [[nodiscard]] double Remaining() const;
    // The script context must be stopped before its handlers are removed.
    void ClearHandlers();
    [[nodiscard]] bool Tick(float elapsed) override;

    [[nodiscard]] ScriptWaitHandlerState* Allocate(
        std::uint32_t size) override;
    [[nodiscard]] std::uint32_t ScriptExecutor() override;
    void ConstructBase(ScriptWaitHandlerState& handler,
                       std::uint32_t wait_mode,
                       std::uint32_t queue_address) override;
    void Flush(ScriptWaitHandlerState& handler) override;
    void AddHandler(std::uint32_t queue_address,
                    ScriptWaitHandlerState* handler) override;
    [[nodiscard]] int SetExecutionFlag(std::uint32_t queue_address) override;
    void SetClosedFlag(ScriptWaitHandlerState& handler) override;

private:
    struct Handler {
        std::unique_ptr<ScriptWaitHandlerState> state;
        bool closed{};
    };

    [[nodiscard]] std::uint32_t QueueAddress() const;

    asIScriptContext& context_;
    ScriptWaitHandler handler_;
    std::unique_ptr<ScriptWaitHandlerState> pending_;
    std::vector<Handler> handlers_;
};

}  // namespace pal4
