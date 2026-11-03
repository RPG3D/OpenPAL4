#pragma once

#include "pal4/crash_report_formatter.h"

#include <cstdint>
#include <string>

namespace pal4 {

class IStackTraceRuntime {
public:
    virtual ~IStackTraceRuntime() = default;
    [[nodiscard]] virtual bool IsBadReadPointer(
        std::uint32_t address, std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void InitializeSymbolHandler() = 0;
    [[nodiscard]] virtual std::uint32_t GetCurrentThread() = 0;
    [[nodiscard]] virtual std::uint32_t GetCurrentProcessId() = 0;
    [[nodiscard]] virtual std::uint32_t GetCurrentProcess() = 0;
    [[nodiscard]] virtual std::uint32_t WalkStack(
        std::uint32_t machine, std::uint32_t process_id,
        std::uint32_t thread, std::uint32_t frame,
        std::uint32_t context, std::uint32_t read_callback) = 0;
    [[nodiscard]] virtual std::uint32_t SymGetModuleBase(
        std::uint32_t process_id, std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string GetModuleFileName(
        std::uint32_t process, std::uint32_t module,
        std::uint32_t max_chars) = 0;
    [[nodiscard]] virtual int TryGetSymbol(
        std::uint32_t process_id, std::uint32_t address,
        std::uint32_t& displacement, CrashReportSymbol& symbol) = 0;
    [[nodiscard]] virtual int TryGetLine(
        std::uint32_t process_id, std::uint32_t address,
        std::uint32_t& displacement, CrashReportLine& line) = 0;
};

class StackTraceFormatter final {
public:
    static constexpr std::uint32_t kFrameAddress = 0x8df7c0u;
    static constexpr std::uint32_t kReportAddress = 0x8df878u;
    static constexpr std::uint32_t kReadCallback = 0x403fc0u;
    static constexpr std::uint32_t kFirstSymbolAddress = 0x8df4a8u;
    static constexpr std::uint32_t kFirstFileAddress = 0x8df5c0u;
    static constexpr std::uint32_t kFrameCountAddress = 0x8e0278u;
    static constexpr std::uint32_t kFirstLineAddress = 0x8e027cu;

    [[nodiscard]] const char* Generate(
        std::uint32_t flags, std::uint32_t exception_pointers,
        IStackTraceRuntime& runtime);
    [[nodiscard]] const std::string& Report() const { return report_; }
    [[nodiscard]] const std::string& FirstSymbol() const { return first_symbol_; }
    [[nodiscard]] const std::string& FirstFile() const { return first_file_; }
    [[nodiscard]] std::uint32_t FirstLine() const { return first_line_; }
    [[nodiscard]] std::uint32_t FrameCount() const { return frame_count_; }

private:
    std::string report_;
    std::string first_symbol_;
    std::string first_file_;
    std::uint32_t first_line_ = 0u;
    std::uint32_t frame_count_ = 0u;
};

}  // namespace pal4
