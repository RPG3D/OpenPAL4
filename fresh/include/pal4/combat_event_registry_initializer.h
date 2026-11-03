#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatEventRegistryInitRuntime {
public:
    virtual ~ICombatEventRegistryInitRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GameCollection() = 0;
    [[nodiscard]] virtual std::uint32_t DataCollection() = 0;
    [[nodiscard]] virtual std::uint32_t FindElementByName(
        std::uint32_t collection, std::string_view name) = 0;
    virtual void InitializeObject(
        std::uint32_t object, std::uint32_t game_element,
        std::uint32_t data_element, std::uint32_t kind) = 0;
    [[nodiscard]] virtual std::int32_t RegisterAtExit(
        std::uint32_t callback) = 0;
};

class CombatEventRegistryInitializer final {
public:
    // IDA 0x581760.
    [[nodiscard]] static std::int32_t Initialize(
        ICombatEventRegistryInitRuntime& runtime);
};

}  // namespace pal4
