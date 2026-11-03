#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalConsoleVariableRuntime {
public:
    virtual ~IPalConsoleVariableRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void InitializeFormatBuffer() = 0;
    [[nodiscard]] virtual std::int32_t CleanupFormatBuffer() = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
};

class PalConsoleVariableCommands final {
public:
    // IDA 0x5bd460 / 0x5bdb20.
    [[nodiscard]] static std::int32_t Show(
        const char* command, IPalConsoleVariableRuntime& runtime);
    [[nodiscard]] static std::int32_t Set(
        const char* command, IPalConsoleVariableRuntime& runtime);
};

}  // namespace pal4
