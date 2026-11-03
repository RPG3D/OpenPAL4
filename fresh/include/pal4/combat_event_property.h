#pragma once

#include <cstdint>

namespace pal4 {

struct EventAttributeValue {
    std::uint32_t vtable = 0x848108u;
    std::uint32_t value_bits = 0u;
};

class ICombatEventPropertyRuntime {
public:
    virtual ~ICombatEventPropertyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual float GetAsAttribute(
        EventAttributeValue& value, std::uint32_t property) = 0;
    [[nodiscard]] virtual int SetFromAttribute(
        EventAttributeValue& value, std::uint32_t property,
        std::uint32_t bits) = 0;
    [[nodiscard]] virtual bool ResetWaitable(std::uint32_t waitable) = 0;
};

class CombatEventProperty final {
public:
    // IDA 0x57ce40.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, std::uint8_t enabled,
        ICombatEventPropertyRuntime& runtime);
    // IDA 0x57ce70. The native function calls the attribute getter.
    static float SetFloatProperty(std::uint32_t object, float value,
                                 ICombatEventPropertyRuntime& runtime);
    // IDA 0x57cea0.
    [[nodiscard]] static int SetIntProperty(
        std::uint32_t object, std::uint32_t value,
        std::uint32_t bits, ICombatEventPropertyRuntime& runtime);
    // IDA 0x57ced0.
    [[nodiscard]] static int IncrementFloatProperty(
        std::uint32_t object, std::uint32_t value_bits,
        float increment, ICombatEventPropertyRuntime& runtime);
    // IDA 0x57cf20.
    [[nodiscard]] static bool MultiplyFloatProperty(
        std::uint32_t object, std::uint32_t value_address,
        float multiplier, ICombatEventPropertyRuntime& runtime);
};

class ConcurrencyEventWaitNode final {
public:
    // IDA 0x57ce60.
    [[nodiscard]] static bool Reset(
        std::uint32_t node, ICombatEventPropertyRuntime& runtime);
};

class CombatDamageState final {
public:
    // IDA 0x57cf70.
    [[nodiscard]] static std::uint32_t Next(std::uint32_t state);
};

}  // namespace pal4
