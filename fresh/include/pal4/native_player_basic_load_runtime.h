#pragma once

#include <cstdint>

namespace pal4 {
struct BinaryXmlNode;

class NativePlayerBasicLoadRuntime final {
public:
    // Applies an already-read BASIC node. Missing fields retain their values;
    // invalid enum indices leave their target word unchanged.
    static void Apply(std::uint32_t player_slot, const BinaryXmlNode& basic);
};
}  // namespace pal4
