#include "pal4/game_data_property_lifecycle.h"

namespace pal4 {

std::uint32_t GameDataPropertyLifecycle::Construct(
    std::uint32_t object, IGameDataPropertyLifecycleRuntime& runtime) {
    runtime.WriteWord(object, 0x8444c4);
    return object;
}

std::uint32_t GameDataPropertyLifecycle::ConstructNumericBase(
    std::uint32_t object, IGameDataPropertyLifecycleRuntime& runtime) {
    runtime.WriteWord(object, 0x843380);
    return object;
}

std::uint32_t GameDataPropertyLifecycle::Destroy(
    std::uint32_t object, std::uint8_t flags,
    IGameDataPropertyLifecycleRuntime& runtime) {
    runtime.DestroyBase(object);
    if ((flags & 1) != 0) runtime.FreeMemory(object);
    return object;
}

}  // namespace pal4
