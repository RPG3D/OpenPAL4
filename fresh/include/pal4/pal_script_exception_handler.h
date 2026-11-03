#pragma once

#include <cstdint>

namespace pal4 {

class IPalScriptExceptionRuntime {
public:
    virtual ~IPalScriptExceptionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ScriptObject(
        std::uint32_t executor_context) = 0;
    [[nodiscard]] virtual std::uint32_t RecoveryObject(
        std::uint32_t script_object) = 0;
    [[nodiscard]] virtual const char* What(
        std::uint32_t exception_address) = 0;
    [[nodiscard]] virtual const char* FunctionName(
        std::uint32_t exception_address) = 0;
    [[nodiscard]] virtual std::uint32_t Report(
        std::uint32_t script_object, const char* what) = 0;
    [[nodiscard]] virtual std::int32_t Classify(
        std::uint32_t script_object, const char* what,
        const char* function_name) = 0;
    [[nodiscard]] virtual std::uint32_t ConvertStatus(
        std::uint32_t script_object, std::int32_t status,
        std::uint32_t option) = 0;
    [[nodiscard]] virtual std::uint32_t FormatMessage(
        std::uint32_t script_object, const char* what,
        std::uint32_t converted_status) = 0;
    [[nodiscard]] virtual std::uint32_t TryRecovery(
        std::uint32_t recovery_object, std::uint32_t message) = 0;
    [[nodiscard]] virtual std::int32_t NextRecoveryStatus(
        std::uint32_t recovery_object, std::uint32_t option) = 0;
    virtual void ResetRecovery(std::uint32_t recovery_object) = 0;
    virtual void ReleaseRecovery(std::uint32_t recovery_object) = 0;
};

class PalScriptExceptionRuntimeStub final : public IPalScriptExceptionRuntime {
public:
    [[nodiscard]] std::uint32_t ScriptObject(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t RecoveryObject(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] const char* What(std::uint32_t) override { return ""; }
    [[nodiscard]] const char* FunctionName(
        std::uint32_t) override { return ""; }
    [[nodiscard]] std::uint32_t Report(
        std::uint32_t, const char*) override { return 0u; }
    [[nodiscard]] std::int32_t Classify(
        std::uint32_t, const char*, const char*) override { return 0; }
    [[nodiscard]] std::uint32_t ConvertStatus(
        std::uint32_t, std::int32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t FormatMessage(
        std::uint32_t, const char*, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t TryRecovery(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::int32_t NextRecoveryStatus(
        std::uint32_t, std::uint32_t) override { return -1; }
    void ResetRecovery(std::uint32_t) override {}
    void ReleaseRecovery(std::uint32_t) override {}
};

class PalScriptExceptionHandler final {
public:
    // IDA 0x600370, reachable control flow.
    [[nodiscard]] static std::uint32_t Handle(
        std::uint32_t executor_address,
        std::uint32_t exception_address,
        IPalScriptExceptionRuntime& runtime);
};

}  // namespace pal4
