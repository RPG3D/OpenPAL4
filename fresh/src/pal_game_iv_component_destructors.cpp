#include "pal4/pal_game_iv_component_destructors.h"

namespace pal4 {
namespace {

void ReleaseObject(std::uint32_t object, std::uint8_t flags,
                   IPalGameIVComponentDestructorRuntime& runtime) {
    if ((flags & 1u) != 0u) runtime.Free(object);
}

}  // namespace

std::uint32_t PalGameIVComponentDestructors::DestructScene(
    std::uint32_t object, IPalGameIVComponentDestructorRuntime& runtime) {
    const auto result = runtime.ResetEffectHandle(object + 4u);
    runtime.WriteWord(object, 0x8485c8u);
    return result;
}

std::uint32_t PalGameIVComponentDestructors::DestructCombat(
    std::uint32_t object, IPalGameIVComponentDestructorRuntime& runtime) {
    const auto result = runtime.DestroyXmlNode(object + 12u);
    runtime.WriteWord(object, 0x8485c8u);
    return result;
}

void PalGameIVComponentDestructors::DestructBase(
    std::uint32_t object, IPalGameIVComponentDestructorRuntime& runtime) {
    runtime.WriteWord(object, 0x8485c8u);
}

std::uint32_t PalGameIVComponentDestructors::DeleteScene(
    std::uint32_t object, std::uint8_t flags,
    IPalGameIVComponentDestructorRuntime& runtime) {
    (void)DestructScene(object, runtime);
    ReleaseObject(object, flags, runtime);
    return object;
}

std::uint32_t PalGameIVComponentDestructors::DeleteCombat(
    std::uint32_t object, std::uint8_t flags,
    IPalGameIVComponentDestructorRuntime& runtime) {
    (void)DestructCombat(object, runtime);
    ReleaseObject(object, flags, runtime);
    return object;
}

std::uint32_t PalGameIVComponentDestructors::DeleteBase(
    std::uint32_t object, std::uint8_t flags,
    IPalGameIVComponentDestructorRuntime& runtime) {
    DestructBase(object, runtime);
    ReleaseObject(object, flags, runtime);
    return object;
}

}  // namespace pal4
