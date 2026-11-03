#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalConsoleActorControlRuntime {
public:
    virtual ~IPalConsoleActorControlRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::int32_t SetWord28(
        std::uint32_t control, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
};

class PalConsoleActorControlCommands final {
public:
    // IDA 0x5bd1b0 / 0x5bd1c0.
    [[nodiscard]] static std::uint32_t JumpUnlock(
        IPalConsoleActorControlRuntime& runtime);
    [[nodiscard]] static std::int32_t SetControlMode(
        const char* command, IPalConsoleActorControlRuntime& runtime);
};

}  // namespace pal4
