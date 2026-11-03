#include "pal4/stack_trace_formatter.h"

#include <algorithm>
#include <array>
#include <cstdio>

namespace pal4 {
namespace {

std::string FormatAddress(const std::uint32_t selector,
                          const std::uint32_t address) {
    std::array<char, 48> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%04X:%08X",
                  selector, address);
    return buffer.data();
}

std::string FormatParams(const IStackTraceRuntime& runtime,
                         const std::uint32_t frame) {
    std::array<char, 96> buffer{};
    std::snprintf(buffer.data(), buffer.size(),
                  " (0x%08X 0x%08X 0x%08X 0x%08X)",
                  runtime.Read(frame + 52u), runtime.Read(frame + 56u),
                  runtime.Read(frame + 60u), runtime.Read(frame + 64u));
    return buffer.data();
}

std::string FormatSymbol(const std::string& name,
                         const std::uint32_t displacement) {
    if (displacement == 0u) {
        return "\n           " + name + '\n';
    }
    std::array<char, 48> suffix{};
    std::snprintf(suffix.data(), suffix.size(), "()+%04d byte(s)\n",
                  static_cast<std::int32_t>(displacement));
    return "\n           " + name + suffix.data();
}

std::string FormatLine(const std::string& file_name,
                       const std::uint32_t line_number,
                       const std::uint32_t displacement) {
    std::array<char, 64> suffix{};
    if (displacement != 0u) {
        std::snprintf(suffix.data(), suffix.size(),
                      "\n           line %04d+%04d byte(s)\n\n",
                      static_cast<std::int32_t>(line_number),
                      static_cast<std::int32_t>(displacement));
    } else {
        std::snprintf(suffix.data(), suffix.size(),
                      "\n           line %04d\n\n",
                      static_cast<std::int32_t>(line_number));
    }
    return "           " + file_name + suffix.data();
}

void AppendTruncatedName(std::string& report, const std::string& name) {
    const std::size_t remaining = report.size() < 1023u
                                      ? 1023u - report.size() : 0u;
    if (remaining != 0u) {
        report.append(name, 0u, std::min(name.size(), remaining - 1u));
    }
}

}  // namespace

const char* StackTraceFormatter::Generate(
    const std::uint32_t flags, const std::uint32_t exception_pointers,
    IStackTraceRuntime& runtime) {
    if (runtime.IsBadReadPointer(exception_pointers, 8u)) {
        return nullptr;
    }

    try {
        runtime.InitializeSymbolHandler();
        const std::uint32_t context = runtime.Read(exception_pointers + 4u);
        const std::uint32_t thread = runtime.GetCurrentThread();
        const std::uint32_t walk_result = runtime.WalkStack(
            0x14cu, runtime.GetCurrentProcessId(), thread,
            kFrameAddress, context, kReadCallback);
        if (walk_result == 0u || runtime.Read(kFrameAddress + 24u) == 0u) {
            frame_count_ = 0u;
            return nullptr;
        }

        const std::uint32_t address = runtime.Read(kFrameAddress);
        const std::uint32_t module = runtime.SymGetModuleBase(
            runtime.GetCurrentProcessId(), address);
        if (module == 0u) {
            frame_count_ = 0u;
            return nullptr;
        }

        report_ = FormatAddress(runtime.Read(context + 188u), address);
        if ((flags & 1u) != 0u) {
            report_ += FormatParams(runtime, kFrameAddress);
        }
        if ((flags & 2u) != 0u) {
            report_ += "\n           ";
            report_ += runtime.GetModuleFileName(
                runtime.GetCurrentProcess(), module, 1024u);
        }

        std::uint32_t displacement = 0u;
        if ((flags & 4u) != 0u) {
            CrashReportSymbol symbol;
            if (runtime.TryGetSymbol(runtime.GetCurrentProcessId(), address,
                                     displacement, symbol) != 1) {
                frame_count_ = 0u;
                return report_.c_str();
            }
            if (symbol.name.size() >
                static_cast<std::uint32_t>(718u - report_.size())) {
                AppendTruncatedName(report_, symbol.name);
                frame_count_ = 0u;
                return report_.c_str();
            }
            report_ += FormatSymbol(symbol.name, displacement);
            if (frame_count_ == 0u) {
                first_symbol_ = symbol.name;
            }
        }

        if ((flags & 8u) != 0u) {
            CrashReportLine line;
            if (runtime.TryGetLine(runtime.GetCurrentProcessId(), address,
                                   displacement, line) == 1) {
                if (line.file_name.size() >
                    static_cast<std::uint32_t>(714u - report_.size())) {
                    AppendTruncatedName(report_, line.file_name);
                    frame_count_ = 0u;
                    return report_.c_str();
                }
                report_ += FormatLine(line.file_name, line.line_number,
                                      displacement);
                if (frame_count_ == 0u) {
                    first_file_ = line.file_name;
                    first_line_ = line.line_number;
                }
            }
        }

        ++frame_count_;
        return report_.c_str();
    } catch (...) {
        frame_count_ = 0u;
        first_symbol_.clear();
        first_file_.clear();
        first_line_ = 0u;
        return nullptr;
    }
}

}  // namespace pal4
