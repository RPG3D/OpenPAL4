#pragma once

#include "pal4/game_data_structure_state.h"

#include <cstdint>

namespace pal4 {

class GameDataObjectLifecycle final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x843270u;

    // IDA 0x583580.
    [[nodiscard]] static std::uint32_t ConstructBaseStates(
        std::uint32_t object, IGameDataStructureStateRuntime& runtime);
    // IDA 0x5835a0.
    static void InitializeVtable(
        std::uint32_t object, IGameDataStructureStateRuntime& runtime);
};

}  // namespace pal4
