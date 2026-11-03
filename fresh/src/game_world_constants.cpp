#include "pal4/game_world_constants.h"

#include <bit>

namespace pal4 {

void GameWorldConstants::Initialize(std::uint32_t& angle_bits) {
    InitializeValue(angle_bits);
}

void GameWorldConstants::InitializeValue(std::uint32_t& angle_bits) {
    const auto first = std::bit_cast<float>(kFirstAngleBits);
    const auto second = std::bit_cast<float>(kSecondAngleBits);
    angle_bits = std::bit_cast<std::uint32_t>(first + second);
}

}  // namespace pal4
