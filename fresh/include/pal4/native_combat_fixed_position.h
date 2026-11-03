#pragma once
#include <array>
namespace pal4 {
// Original virtual448 fixed descriptors538130/531050; distinct from slot444.
constexpr std::array<float,3> NativeCombatFixedPosition(bool player){
    return player?std::array<float,3>{8.f,0.f,156.f}:std::array<float,3>{41.f,0.f,-148.f};
}
}
