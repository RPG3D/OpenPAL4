#include "pal4/pal_script_exception_handler.h"

namespace pal4 {

namespace {

std::uint32_t ReportWithLowByte(
    const std::uint32_t script_object,
    const std::uint32_t exception_address,
    const std::uint8_t low_byte,
    IPalScriptExceptionRuntime& runtime) {
    const auto result = runtime.Report(
        script_object, runtime.What(exception_address));
    return (result & 0xffffff00u) | low_byte;
}

void CleanupRecovery(const std::uint32_t recovery_object,
                     IPalScriptExceptionRuntime& runtime) {
    runtime.ResetRecovery(recovery_object);
    runtime.ReleaseRecovery(recovery_object);
}

}  // namespace

std::uint32_t PalScriptExceptionHandler::Handle(
    const std::uint32_t executor_address,
    const std::uint32_t exception_address,
    IPalScriptExceptionRuntime& runtime) {
    const auto script_object = runtime.ScriptObject(executor_address + 84u);
    if (script_object == 0u) return 0u;

    const auto recovery_object = runtime.RecoveryObject(script_object);
    if (recovery_object == 0u) {
        return ReportWithLowByte(
            script_object, exception_address, 0u, runtime);
    }

    const auto function_name = runtime.FunctionName(exception_address);
    const auto status = runtime.Classify(
        script_object, runtime.What(exception_address), function_name);
    if (status == -6) {
        CleanupRecovery(recovery_object, runtime);
        return ReportWithLowByte(
            script_object, exception_address, 0u, runtime);
    }

    const auto converted = runtime.ConvertStatus(script_object, status, 0u);
    const auto message = runtime.FormatMessage(
        script_object, runtime.What(exception_address), converted);
    if (runtime.TryRecovery(recovery_object, message) != 0u) {
        CleanupRecovery(recovery_object, runtime);
        return ReportWithLowByte(
            script_object, exception_address, 0u, runtime);
    }

    for (;;) {
        const auto next = runtime.NextRecoveryStatus(recovery_object, 0u);
        if (next == -1) {
            CleanupRecovery(recovery_object, runtime);
            return ReportWithLowByte(
                script_object, exception_address, 0u, runtime);
        }
        if (next == 0) {
            CleanupRecovery(recovery_object, runtime);
            return ReportWithLowByte(
                script_object, exception_address, 1u, runtime);
        }
    }
}

}  // namespace pal4
