#include "pal4/symbol_handler.h"

namespace pal4 {

std::uint32_t SymbolHandler::Initialize(ISymbolHandlerRuntime& runtime) {
    if (initialized_ != 0u) {
        return initialized_;
    }
    runtime.SymSetOptions(runtime.SymGetOptions() | 0x14u);
    const std::uint32_t process_id = runtime.GetCurrentProcessId();
    const std::uint32_t result = runtime.InitializeAndLoadModules(
        process_id, process_id, 0u, 1u);
    initialized_ = 1u;
    return result;
}

int SymbolHandler::GetLineInfo(
    const std::uint32_t process, const std::uint32_t address,
    std::uint32_t& displacement, CrashReportLine& line,
    ISymbolHandlerRuntime& runtime) {
    return runtime.SymGetLineFromAddr(process, address, displacement, line);
}

}  // namespace pal4
