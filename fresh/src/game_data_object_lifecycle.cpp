#include "pal4/game_data_object_lifecycle.h"

namespace pal4 {

std::uint32_t GameDataObjectLifecycle::ConstructBaseStates(
    std::uint32_t object, IGameDataStructureStateRuntime& runtime) {
    static_cast<void>(GameDataStructureState::Construct(object + 4u, runtime));
    static_cast<void>(GameDataStructureState::Construct(object + 0x154u,
                                                        runtime));
    InitializeVtable(object, runtime);
    return object;
}

void GameDataObjectLifecycle::InitializeVtable(
    std::uint32_t object, IGameDataStructureStateRuntime& runtime) {
    runtime.WriteWord(object, kBaseVtable);
}

}  // namespace pal4
