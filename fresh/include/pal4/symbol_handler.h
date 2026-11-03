#pragma once

#include "pal4/crash_report_formatter.h"

#include <cstdint>

namespace pal4 {

class ISymbolHandlerRuntime {
public:
    virtual ~ISymbolHandlerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SymGetOptions() = 0;
    virtual void SymSetOptions(std::uint32_t options) = 0;
    [[nodiscard]] virtual std::uint32_t GetCurrentProcessId() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeAndLoadModules(
        std::uint32_t process_id, std::uint32_t process_handle,
        std::uint32_t search_path, std::uint32_t invade_process) = 0;
    [[nodiscard]] virtual int SymGetLineFromAddr(
        std::uint32_t process, std::uint32_t address,
        std::uint32_t& displacement, CrashReportLine& line) = 0;
};

class SymbolHandler final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e0280u;

    explicit SymbolHandler(std::uint32_t& initialized)
        : initialized_(initialized) {}

    [[nodiscard]] std::uint32_t Initialize(ISymbolHandlerRuntime& runtime);
    [[nodiscard]] static int GetLineInfo(
        std::uint32_t process, std::uint32_t address,
        std::uint32_t& displacement, CrashReportLine& line,
        ISymbolHandlerRuntime& runtime);

private:
    std::uint32_t& initialized_;
};

}  // namespace pal4
