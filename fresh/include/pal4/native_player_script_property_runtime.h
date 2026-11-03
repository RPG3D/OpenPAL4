#pragma once

#include <cstdint>

namespace pal4 {
struct BinaryXmlNode;
struct ScriptBindingSpec;

// Shared native binding for the player's scalar script properties. The script
// component starts at player+0x654. These setters retain clamps, mirror writes
// and health-status notification from the original component vtable.
class NativePlayerScriptPropertyRuntime final {
public:
    // Floating setters return their result as float bits; integer setters
    // return the original integer result. Apply does not consume either result.
    // Slots24/54 add unsigned delta bits through the existing indexed/current
    // property getter/setter pairs, retaining current-stat clamps/notifications.
    [[nodiscard]] static std::uint32_t Set(
        std::uint32_t script, std::uint32_t virtual_slot,
        std::uint32_t index, std::uint32_t value_bits);
    static void UpdateHealthStatus(std::uint32_t script);
    [[nodiscard]] static std::uint32_t SetBinding(
        std::uint32_t script, const ScriptBindingSpec& binding, std::uint32_t bits);
    // Applies only the 84 script-backed Player attributes. The role loader
    // reapplies HP/MP afterwards, once all maximum values have been restored.
    static void Apply(std::uint32_t player_slot, const BinaryXmlNode& player);
};
}  // namespace pal4
