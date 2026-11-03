#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class PalConsoleArgumentParser final {
public:
    // IDA 0x5bb350: one-based, space-delimited argument access.
    [[nodiscard]] std::optional<std::string_view> Parse(
        const char* command, std::int32_t index);

private:
    std::string buffer_;
};

class IPalConsoleNativeCommandRuntime {
public:
    virtual ~IPalConsoleNativeCommandRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameConfigAddress() = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InventorySubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::int32_t PlayerId(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t id) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateInventoryState(
        std::uint32_t player, std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::int32_t SetPlayerPosition(
        std::uint32_t player, const std::array<float, 3>& position,
        std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
};

class PalConsoleNativeCommands final {
public:
    // IDA 0x5bb3d0.
    [[nodiscard]] static std::uint32_t Exit(
        IPalConsoleNativeCommandRuntime& runtime);
    // IDA 0x5bb3e0.
    [[nodiscard]] static std::int32_t SetPlayerPosition(
        const char* command, IPalConsoleNativeCommandRuntime& runtime);
    // IDA 0x5bba50.
    [[nodiscard]] static std::int32_t SetGlobalVariable(
        const char* command, IPalConsoleNativeCommandRuntime& runtime);
    // IDA 0x5bbcd0 / 0x5bbd50.
    [[nodiscard]] static std::uint8_t PlayerJoin(
        const char* command, IPalConsoleNativeCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t PlayerLeave(
        const char* command, IPalConsoleNativeCommandRuntime& runtime);
    // IDA 0x5bbdd0.
    [[nodiscard]] static std::uint32_t SetInventoryValue(
        const char* command, IPalConsoleNativeCommandRuntime& runtime);
};

}  // namespace pal4
