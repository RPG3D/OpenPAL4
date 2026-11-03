#pragma once

#include "pal4/pal_game_iv_state_registry.h"

#include <cstdint>

namespace pal4 {

enum class PalGameIVComponentKind {
    World,
    SceneNamedNode,
    CombatXmlDocument,
    WorldLoading,
    GameOver,
    Save,
    ModalNamedNode,
    WorldMap,
    Bink,
};

class IPalGameIVComponentsRuntime : public IPalGameIVStateRegistryRuntime {
public:
    virtual ~IPalGameIVComponentsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ConstructComponent(
        PalGameIVComponentKind kind, std::uint32_t object) = 0;
};

class PalGameIVComponents final {
public:
    // The Combat allocation/registration is owned by the caller. IDA5B6CB0:
    // base vtable, embedded document+12, then PALIV_COMBAT vtable.
    [[nodiscard]] static std::uint32_t ConstructCombatState(
        std::uint32_t object, IPalGameIVComponentsRuntime& runtime);
    [[nodiscard]] static bool Initialize(
        std::uint32_t game, IPalGameIVComponentsRuntime& runtime);
};

}  // namespace pal4
