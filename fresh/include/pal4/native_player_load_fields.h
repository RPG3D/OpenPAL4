#pragma once

#include <cstdint>

namespace pal4 {
struct BinaryXmlNode;
class NativePlayerSlotCopyRuntime;

class NativePlayerLoadFields final {
public:
    // Applies all114 Player attributes, retaining session-owned Role strings.
    // Forbidden skills and final HP/MP reapply belong to GameConfigLoader.
    static void Apply(std::uint32_t player_slot, const BinaryXmlNode& player,
                      NativePlayerSlotCopyRuntime& text_owner);
};
}  // namespace pal4
