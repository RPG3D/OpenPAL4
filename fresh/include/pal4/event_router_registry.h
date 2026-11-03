#pragma once

#include <cstdint>

namespace pal4 {

class IEventRouterRegistryRuntime {
public:
    virtual ~IEventRouterRegistryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindGroup(
        std::uint32_t router, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t CreateGroup(
        std::uint32_t router, std::uint32_t key,
        std::uint8_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t FindPosition(
        std::uint32_t first, std::uint32_t head,
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t CreateScriptEvent(
        std::uint32_t owner, std::uint32_t callback,
        std::uint32_t context, std::uint32_t output_slot) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class EventRouterRegistry final {
public:
    static constexpr std::uint32_t kEventVtable = 0x844910u;

    // IDA EventRouterRegistry_Register at 0x549c60.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t router, std::uint32_t mode,
        std::uint32_t key, std::uint32_t owner,
        std::uint32_t callback, std::uint32_t context,
        IEventRouterRegistryRuntime& runtime);
};

}  // namespace pal4
