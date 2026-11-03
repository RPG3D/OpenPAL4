#pragma once
#include "pal4/native_monster_pursuit_frame.h"
#include <memory>

namespace pal4 {
class NativePalGameIVTextureStorage;
class NativePalGameIVStateList;
class NativeScriptCombatStart;
class NativeGameArenaManagerOwner;
class NativeFloorAttributeOwner;
class NativeFloorCombatAreas;
class IMonsterProximityRuntime;

// Scene-owned434720 ->434E30 composition. All referenced game/scene owners
// must survive synchronous Entry and the end of the current world traversal.
class NativeMonsterEncounter final : public IMonsterEncounterRuntime {
public:
    NativeMonsterEncounter(NativeMonsterManagerOwner&, NativeBaseObjectMemory&,
        IMonsterProximityRuntime&, NativePalGameIVTextureStorage&,
        NativePalGameIVStateList&, NativeScriptCombatStart&,
        NativeGameArenaManagerOwner&, NativeFloorAttributeOwner&,
        const NativeFloorCombatAreas&);
    ~NativeMonsterEncounter();
    void DispatchCombatPositions(std::uint32_t object, float elapsed) override;
    void InitializeCombat(std::uint32_t object, std::int32_t argument) override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
