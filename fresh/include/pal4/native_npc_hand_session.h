#pragma once

#include "pal4/script_npc_hand_effect.h"
#include "pal4/native_player_weapon_effect_host.h"
#include <memory>
#include <unordered_set>

namespace pal4 {
class NativeArenaNpcSceneLoader;
class NativeSceneGobAssets;
class NativeScriptObjectVisibilityRuntime;
class NativeEffectSceneSession;

// Borrows scene NPC/GOB owners. Clear before either owner or the world/effects
// is released. Ordinary GOB clumps stay owned by NativeSceneGobAssets.
class NativeNpcHandSession final : public IScriptNpcHandEffectRuntime,
                                   private INativePlayerWeaponResources {
public:
    NativeNpcHandSession(NativeArenaNpcSceneLoader&, NativeSceneGobAssets&,
        NativeScriptObjectVisibilityRuntime&, RpWorld&, std::uint32_t game,
        NativeEffectSceneSession&, NativePlayerTransformRuntime&);
    ~NativeNpcHandSession();
    void Clear();
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint32_t EmptyStringAddress() override;
    std::uint32_t NpcManager() override;
    std::uint32_t FindNpc(std::uint32_t, std::uint32_t) override;
    std::uint32_t SetupLeftHandEffect(std::uint32_t, std::uint32_t) override;
    std::uint32_t SetupRightHandEffect(std::uint32_t, std::uint32_t) override;
    std::uint32_t UpdateWeaponEffects(std::uint32_t) override;
    bool NameDiffersFromEmptyError(std::uint32_t, std::uint32_t) override;
    std::uint32_t ObjectManager() override;
    std::uint32_t FindObject(std::uint32_t, std::uint32_t) override;
    std::uint32_t ArenaManager() override;
    void RemoveAnimatedObject(std::uint32_t, std::uint32_t) override;
    std::uint32_t RenderUnit(std::uint32_t, std::uint32_t,
        const std::array<float, 3>&, std::uint32_t, bool) override;
    std::uint32_t RenderObject(std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t, bool) override;
    std::uint32_t CleanupWeapons(std::uint32_t) override;
private:
    BaseObjectState& Object(std::uint32_t);
    void CleanupNamedResource(BaseObjectState&, std::uint32_t, std::uint32_t) override;
    NativeArenaNpcSceneLoader& npcs_;
    NativeSceneGobAssets& objects_;
    NativeScriptObjectVisibilityRuntime& visibility_;
    std::unique_ptr<NativePlayerWeaponEffectHost> host_;
    NativePlayerWeaponRuntime weapons_;
    std::unordered_set<std::uint32_t> touched_;
};
}
