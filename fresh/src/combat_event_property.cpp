#include "pal4/combat_event_property.h"

#include <bit>

namespace pal4 {

std::uint32_t CombatEventProperty::Construct(
    std::uint32_t object, std::uint32_t first,
    std::uint32_t second, std::uint8_t enabled,
    ICombatEventPropertyRuntime& runtime) {
    runtime.WriteWord(object + 4u, first);
    runtime.WriteWord(object + 8u, second);
    runtime.WriteByte(object + 12u, enabled);
    runtime.WriteWord(object, 0x8480f4u);
    return object;
}

float CombatEventProperty::SetFloatProperty(
    std::uint32_t object, float value,
    ICombatEventPropertyRuntime& runtime) {
    const auto property = runtime.ReadWord(object + 4u);
    EventAttributeValue temporary{0x848108u,
                                  std::bit_cast<std::uint32_t>(value)};
    return runtime.GetAsAttribute(temporary, property);
}

int CombatEventProperty::SetIntProperty(
    std::uint32_t object, std::uint32_t value,
    std::uint32_t bits, ICombatEventPropertyRuntime& runtime) {
    const auto property = runtime.ReadWord(object + 8u);
    EventAttributeValue temporary{0x848108u, value};
    return runtime.SetFromAttribute(temporary, property, bits);
}

int CombatEventProperty::IncrementFloatProperty(
    std::uint32_t object, std::uint32_t value_bits,
    float increment, ICombatEventPropertyRuntime& runtime) {
    EventAttributeValue temporary{0x848108u, value_bits};
    const auto property = runtime.ReadWord(object + 8u);
    const auto current = runtime.GetAsAttribute(temporary, property);
    const auto result_bits = std::bit_cast<std::uint32_t>(current + increment);
    const auto target = runtime.ReadWord(object + 8u);
    return runtime.SetFromAttribute(temporary, target, result_bits);
}

bool CombatEventProperty::MultiplyFloatProperty(
    std::uint32_t object, std::uint32_t value_address,
    float multiplier, ICombatEventPropertyRuntime& runtime) {
    if (runtime.ReadByte(object + 12u) == 0u)
        return false;
    const auto old_value = std::bit_cast<float>(
        runtime.ReadWord(value_address));
    runtime.WriteWord(value_address,
                      std::bit_cast<std::uint32_t>(multiplier * old_value));
    return true;
}

bool ConcurrencyEventWaitNode::Reset(
    std::uint32_t node, ICombatEventPropertyRuntime& runtime) {
    return runtime.ResetWaitable(runtime.ReadWord(node + 4u));
}

std::uint32_t CombatDamageState::Next(std::uint32_t state) {
    return state == 4u ? 0u : state + 1u;
}

}  // namespace pal4
