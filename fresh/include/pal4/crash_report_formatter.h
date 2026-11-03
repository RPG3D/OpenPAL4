#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

struct CrashReportSymbol {
    std::uint32_t size_of_struct = 24u;
    std::uint32_t max_name_length = 488u;
    std::string name;
};

struct CrashReportLine {
    std::uint32_t size_of_struct = 20u;
    std::uint32_t key = 0u;
    std::uint32_t line_number = 0u;
    std::string file_name;
    std::uint32_t address = 0u;
};

class ICrashReportRuntime {
public:
    virtual ~ICrashReportRuntime() = default;
    [[nodiscard]] virtual bool IsBadReadPointer(
        std::uint32_t address, std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void InitializeSymbolHandler() = 0;
    [[nodiscard]] virtual std::uint32_t GetCurrentProcess() = 0;
    [[nodiscard]] virtual std::uint32_t GetCurrentProcessId() = 0;
    [[nodiscard]] virtual std::string GetModuleFileName(
        std::uint32_t process, std::uint32_t module,
        std::uint32_t max_chars) = 0;
    [[nodiscard]] virtual std::uint32_t GetModuleHandle(
        const char* module_name) = 0;
    [[nodiscard]] virtual std::string FormatMessage(
        std::uint32_t flags, std::uint32_t module, std::uint32_t code,
        std::uint32_t language, std::uint32_t max_chars) = 0;
    [[nodiscard]] virtual std::uint32_t SymGetModuleBase(
        std::uint32_t process_id, std::uint32_t address) = 0;
    [[nodiscard]] virtual int TryGetSymbol(
        std::uint32_t process_id, std::uint32_t address,
        std::uint32_t& displacement, CrashReportSymbol& symbol) = 0;
    [[nodiscard]] virtual int TryGetLine(
        std::uint32_t process_id, std::uint32_t address,
        std::uint32_t& displacement, CrashReportLine& line) = 0;
};

class CrashReportFormatter final {
public:
    static constexpr std::uint32_t kOriginalBufferAddress = 0x8df878u;
    static constexpr std::uint32_t kOriginalBufferSize = 1024u;

    [[nodiscard]] const char* Generate(
        std::uint32_t exception_pointers,
        ICrashReportRuntime& runtime);
    [[nodiscard]] const std::string& Report() const { return report_; }

private:
    std::string report_;
};

}  // namespace pal4
