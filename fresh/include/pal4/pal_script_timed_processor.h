#pragma once

#include "pal4/pal_script_executor.h"
#include "pal4/string_buffer.h"

#include <cstdint>

namespace pal4 {

struct TimedScriptInterpreterRequest final {
    std::uint32_t mode = 0u;
    std::uint32_t script_path = 0u;
    std::uint32_t full_path = 0u;
    std::uint32_t callback = 0u;
    std::uint32_t arena_context = 0u;
    std::uint32_t read_callback = 0x600d50u;
};

class IPalScriptTimedRuntime {
public:
    virtual ~IPalScriptTimedRuntime() = default;
    [[nodiscard]] virtual bool ActorControlActive() = 0;
    [[nodiscard]] virtual std::uint32_t ExecutorAddress() = 0;
    [[nodiscard]] virtual std::uint32_t ExecutorWord(
        std::uint32_t address) = 0;
    virtual void InitializeScriptSystem() = 0;
    virtual void InitializePath(StringBufferState& destination,
                                const char* text) = 0;
    virtual void CopyString(StringBufferState& destination,
                            const StringBufferState& source) = 0;
    virtual void AppendLiteral(StringBufferState& destination,
                               const char* text, std::uint32_t length) = 0;
    virtual void AppendText(StringBufferState& destination,
                            std::uint32_t text, std::uint32_t length) = 0;
    [[nodiscard]] virtual bool HasStackModule(
        std::uint32_t context, std::uint32_t full_path) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaContext() = 0;
    [[nodiscard]] virtual std::uint32_t ScriptMode() = 0;
    [[nodiscard]] virtual std::uint32_t CreateInterpreter(
        const TimedScriptInterpreterRequest& request) = 0;
    [[nodiscard]] virtual bool AddScriptModule(
        std::uint32_t context, std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual const char* FunctionName(
        std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual const char* FullPath(
        std::uint32_t interpreter) = 0;
    virtual void PushStackModule(std::uint32_t context, const char* full_path,
                                 const char* function_name) = 0;
    virtual void ReleaseInterpreter(std::uint32_t interpreter) = 0;
    virtual void ShowMissingScriptMessage() = 0;
    virtual void CleanupAllScripts() = 0;
    [[nodiscard]] virtual std::uint32_t CleanupString(
        StringBufferState& text) = 0;
};

class PalScriptTimedRuntimeStub final : public IPalScriptTimedRuntime {
public:
    [[nodiscard]] bool ActorControlActive() override { return false; }
    [[nodiscard]] std::uint32_t ExecutorAddress() override { return 0u; }
    [[nodiscard]] std::uint32_t ExecutorWord(
        std::uint32_t) override { return 0u; }
    void InitializeScriptSystem() override {}
    void InitializePath(StringBufferState&, const char*) override {}
    void CopyString(StringBufferState&, const StringBufferState&) override {}
    void AppendLiteral(StringBufferState&, const char*,
                       std::uint32_t) override {}
    void AppendText(StringBufferState&, std::uint32_t,
                    std::uint32_t) override {}
    [[nodiscard]] bool HasStackModule(
        std::uint32_t, std::uint32_t) override { return false; }
    [[nodiscard]] std::uint32_t ArenaContext() override { return 0u; }
    [[nodiscard]] std::uint32_t ScriptMode() override { return 0u; }
    [[nodiscard]] std::uint32_t CreateInterpreter(
        const TimedScriptInterpreterRequest&) override { return 0u; }
    [[nodiscard]] bool AddScriptModule(
        std::uint32_t, std::uint32_t) override { return false; }
    [[nodiscard]] const char* FunctionName(
        std::uint32_t) override { return ""; }
    [[nodiscard]] const char* FullPath(
        std::uint32_t) override { return ""; }
    void PushStackModule(std::uint32_t, const char*, const char*) override {}
    void ReleaseInterpreter(std::uint32_t) override {}
    void ShowMissingScriptMessage() override {}
    void CleanupAllScripts() override {}
    [[nodiscard]] std::uint32_t CleanupString(
        StringBufferState&) override { return 0u; }
};

class PalScriptTimedProcessor final {
public:
    // IDA 0x600b90.
    [[nodiscard]] static std::uint32_t Process(
        PalScriptExecutorState& state, float delta,
        IPalScriptTimedRuntime& runtime);
};

}  // namespace pal4
