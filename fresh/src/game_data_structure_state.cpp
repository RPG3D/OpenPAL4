#include "pal4/game_data_structure_state.h"

namespace pal4 {

std::uint32_t GameDataStructureState::Construct(
    std::uint32_t object, IGameDataStructureStateRuntime& runtime) {
    runtime.InitializeOuterBase(object);
    (void)Initialize(object, runtime);
    return object;
}

std::uint32_t GameDataStructureState::Initialize(
    std::uint32_t object, IGameDataStructureStateRuntime& runtime) {
    runtime.InitializeInnerBase(object);
    for (std::uint32_t offset = 0x90; offset < 0x148; offset += 4)
        runtime.WriteWord(object + offset, 0);
    runtime.WriteWord(object + 0x148, 6);
    runtime.WriteWord(object + 0x14c, 0xffffffff);
    for (std::uint32_t offset = 0xf4; offset <= 0x100; offset += 4)
        runtime.WriteWord(object + offset, 0x3f800000);
    for (std::uint32_t offset = 0x108; offset <= 0x120; offset += 4)
        runtime.WriteWord(object + offset, 0x3f800000);
    runtime.WriteWord(object + 0xdc, 1);
    return 0x3f800000;
}

}  // namespace pal4
