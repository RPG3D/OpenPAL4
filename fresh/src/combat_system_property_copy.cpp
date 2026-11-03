#include "pal4/combat_system_property_copy.h"

namespace pal4 {

std::uint8_t CombatSystemPropertyCopy::CopyConfiguration(
    const std::uint32_t source, ICombatSystemPropertyCopyRuntime& runtime) {
    const auto system = runtime.CombatSystemInstance();
    for (std::uint32_t offset = 0u; offset < 20u; offset += 4u)
        runtime.WriteWord(system + 432u + offset,
                          runtime.ReadWord(source + offset));
    runtime.AssignFirstText(system + 452u, source + 20u);
    static_cast<void>(CombatSystemTextProperties::AssignSecond(
        system, source + 36u, runtime));
    static_cast<void>(CombatSystemTextProperties::AssignThird(
        system, source + 52u, runtime));
    runtime.WriteByte(system + 500u, runtime.ReadByte(source + 68u));
    runtime.WriteByte(system + 501u, runtime.ReadByte(source + 69u));
    const auto result = runtime.ReadByte(source + 70u);
    runtime.WriteByte(system + 502u, result);
    return result;
}

std::uint8_t CombatSystemPropertyCopy::AssignFirstText(
    const std::uint32_t source, ICombatSystemPropertyCopyRuntime& runtime) {
    return CombatSystemTextProperties::AssignFirst(
        runtime.CombatSystemInstance(), source, runtime);
}

std::uint8_t CombatSystemPropertyCopy::AssignSecondText(
    const std::uint32_t source, ICombatSystemPropertyCopyRuntime& runtime) {
    return CombatSystemTextProperties::AssignSecond(
        runtime.CombatSystemInstance(), source, runtime);
}

}  // namespace pal4
