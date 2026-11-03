#include "pal4/crash_report_formatter.h"
#include "pal4/exception_description_catalog.h"

#include <algorithm>
#include <array>
#include <cstdio>

namespace pal4 {
namespace {

std::string FormatLocation(const std::uint32_t selector,
                           const std::uint32_t address) {
    std::array<char, 48> buffer{};
    std::snprintf(buffer.data(), buffer.size(), " at %04X:%08X",
                  selector, address);
    return buffer.data();
}

std::string FormatSymbol(const std::string& name,
                         const std::uint32_t displacement) {
    if (displacement == 0u) {
        return name + ' ';
    }
    std::array<char, 48> suffix{};
    std::snprintf(suffix.data(), suffix.size(), "()+%04d byte(s)",
                  static_cast<std::int32_t>(displacement));
    return name + suffix.data();
}

std::string FormatLine(const std::uint32_t line_number,
                       const std::uint32_t displacement) {
    std::array<char, 64> buffer{};
    if (displacement != 0u) {
        std::snprintf(buffer.data(), buffer.size(), "%06d+%06d byte(s)",
                      static_cast<std::int32_t>(line_number),
                      static_cast<std::int32_t>(displacement));
    } else {
        std::snprintf(buffer.data(), buffer.size(), "%06d",
                      static_cast<std::int32_t>(line_number));
    }
    return buffer.data();
}

void AppendTruncatedName(std::string& report, const std::string& name) {
    const std::size_t remaining = report.size() < 1023u
                                      ? 1023u - report.size() : 0u;
    if (remaining != 0u) {
        report.append(name, 0u, std::min(name.size(), remaining - 1u));
    }
}

}  // namespace

const char* CrashReportFormatter::Generate(
    const std::uint32_t exception_pointers,
    ICrashReportRuntime& runtime) {
    if (runtime.IsBadReadPointer(exception_pointers, 8u)) {
        return nullptr;
    }

    runtime.InitializeSymbolHandler();
    report_ = "Reason:    ";
    const std::string executable = runtime.GetModuleFileName(
        runtime.GetCurrentProcess(), 0u, kOriginalBufferSize);
    report_ += executable + " caused an ";

    const std::uint32_t exception_record = runtime.Read(exception_pointers);
    const std::uint32_t code = runtime.Read(exception_record);
    const char* description = ExceptionDescriptionCatalog::Lookup(code);
    if (description != nullptr) {
        report_ += description;
    } else {
        report_ += runtime.FormatMessage(
            0xa00u, runtime.GetModuleHandle("NTDLL.DLL"), code, 0u,
            kOriginalBufferSize);
    }

    report_ += "\nLocation:  in module ";
    const std::uint32_t address = runtime.Read(exception_record + 12u);
    const std::uint32_t module = runtime.SymGetModuleBase(
        runtime.GetCurrentProcessId(), address);
    if (module == 0u) {
        report_ += "<UNKNOWN>";
    } else {
        report_ += runtime.GetModuleFileName(
            runtime.GetCurrentProcess(), module,
            static_cast<std::uint32_t>(kOriginalBufferSize - report_.size()));
    }
    const std::uint32_t context = runtime.Read(exception_pointers + 4u);
    report_ += FormatLocation(runtime.Read(context + 188u), address);

    CrashReportSymbol symbol;
    std::uint32_t displacement = 0u;
    if (runtime.TryGetSymbol(runtime.GetCurrentProcessId(), address,
                             displacement, symbol) != 1) {
        return report_.c_str();
    }
    report_ += "\nFunction:  ";
    if (static_cast<std::int64_t>(symbol.name.size()) >
        718 - static_cast<std::int64_t>(report_.size())) {
        AppendTruncatedName(report_, symbol.name);
        return report_.c_str();
    }
    report_ += FormatSymbol(symbol.name, displacement);

    CrashReportLine line;
    if (runtime.TryGetLine(runtime.GetCurrentProcessId(), address,
                           displacement, line) != 1) {
        return report_.c_str();
    }
    report_ += "\nFile:      ";
    if (static_cast<std::int64_t>(line.file_name.size()) >
        714 - static_cast<std::int64_t>(report_.size())) {
        AppendTruncatedName(report_, line.file_name);
        return report_.c_str();
    }
    report_ += line.file_name;
    report_ += "\nLine:      ";
    report_ += FormatLine(line.line_number, displacement);
    return report_.c_str();
}

}  // namespace pal4
