#include "pal4/pal_game_iv_object_factories.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<PalGameIVFactoryRegistration, 6> kEventVolume = {{
    {0u, 0x5a9380u}, {1u, 0x5a9e80u}, {2u, 0x5a94c0u},
    {3u, 0x5a9420u}, {4u, 0x5a92f0u}, {5u, 0x5aa0f0u},
}};

constexpr std::array<PalGameIVFactoryRegistration, 11> kGameObjects = {{
    {0u, 0x5a42e0u}, {1u, 0x59f380u}, {2u, 0x59d100u},
    {3u, 0x5a5f40u}, {4u, 0x59b780u}, {5u, 0x59fc00u},
    {6u, 0x59aa00u}, {7u, 0x59e6f0u}, {8u, 0x59c460u},
    {9u, 0x5a3cd0u}, {10u, 0x5a4c50u},
}};

constexpr std::array<PalGameIVFactoryRegistration, 13> kStorageObjects = {{
    {6u, 0x59b1a0u}, {4u, 0x59bfc0u}, {8u, 0x59cc70u},
    {7u, 0x59edb0u}, {5u, 0x5a3160u}, {0u, 0x5a47d0u},
    {1u, 0x59f770u}, {2u, 0x59df90u}, {9u, 0x5a3fa0u},
    {10u, 0x5a5bf0u}, {3u, 0x5a6990u}, {11u, 0x432180u},
    {12u, 0x40bfb0u},
}};

}  // namespace

std::span<const PalGameIVFactoryRegistration>
PalGameIVObjectFactories::GameObjects() {
    return kGameObjects;
}

bool PalGameIVObjectFactories::RegisterAll(
    IPalGameIVObjectFactoryRuntime& runtime) {
    for (const auto entry : kEventVolume)
        runtime.RegisterEventVolume(runtime.EventVolumeRegistry(), entry);
    for (const auto entry : kGameObjects)
        runtime.RegisterGameObject(runtime.GameObjectRegistry(), entry);
    bool result = false;
    for (const auto entry : kStorageObjects)
        result = runtime.RegisterStorageObject(
            runtime.StorageObjectRegistry(), entry);
    return result;
}

}  // namespace pal4
