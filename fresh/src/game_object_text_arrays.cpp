#include "pal4/game_object_text_arrays.h"

namespace pal4 {

void GameObjectTextCleanupRuntimeStub::DestroyTextRecord(
    std::uint32_t) {}

void GameObjectTextArrays::Cleanup(
    std::uint32_t object, IGameObjectTextCleanupRuntime& runtime) {
    for (std::uint32_t count = 5u; count != 0u; --count)
        runtime.DestroyTextRecord(object + 0x60u + (count - 1u) * 0x10u);
    for (std::uint32_t count = 4u; count != 0u; --count)
        runtime.DestroyTextRecord(object + (count - 1u) * 0x10u);
}

}  // namespace pal4
