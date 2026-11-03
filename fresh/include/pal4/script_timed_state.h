#pragma once

#include <cstdint>

namespace pal4 {

class IScriptTimedStateRuntime {
public:
    virtual ~IScriptTimedStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyString() = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeState(
        std::uint32_t executor, std::uint32_t value,
        std::uint32_t callback) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t TextLength(
        std::uint32_t text) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeText(
        std::uint32_t destination, std::uint32_t text,
        std::uint32_t length) = 0;
};

class ScriptTimedState final {
public:
    // IDA 0x5d8610, giTimeScript.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t value, std::uint32_t callback_record,
        IScriptTimedStateRuntime& runtime);
    // IDA 0x5d8640, giTimeScriptTerminate; includes its tail at 0x600b50.
    [[nodiscard]] static std::uint32_t Terminate(
        IScriptTimedStateRuntime& runtime);
};

}  // namespace pal4
