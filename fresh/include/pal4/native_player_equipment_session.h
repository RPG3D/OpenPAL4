#pragma once
#include "pal4/native_player_weapon_effect_host.h"
#include "pal4/native_player_equipment_visual_runtime.h"
#include <filesystem>
#include <memory>

namespace pal4 {
class NativeEffectSceneSession;
class NativeInventorySession;
struct PlayerManagerObjectState;

// Owns scene weapon resources and their effect adapter. Slots, world, inventory,
// packages, effects and transforms must outlive it. Clear before actor/world
// release; Clear transfers retiring effects before releasing their clumps.
class NativePlayerEquipmentSession final : private INativePlayerWeaponResources {
public:
    NativePlayerEquipmentSession(const std::filesystem::path& root,RpWorld& world,
        std::uint32_t game,PlayerManagerObjectState& players,NativeInventorySession& inventory,
        NativePkFileManagerRuntime& packages,NativeEffectSceneSession& effects,
        NativePlayerTransformRuntime& transforms);
    ~NativePlayerEquipmentSession();
    NativePlayerEquipmentSession(const NativePlayerEquipmentSession&)=delete;
    NativePlayerEquipmentSession& operator=(const NativePlayerEquipmentSession&)=delete;
    void Apply(PlayerManagerSlotState& slot);
    void CleanupNamed(PlayerManagerSlotState& slot);
    void CleanupPlayer(BaseObjectState& state);
    bool DialogNeedsUpdate(BaseObjectState& state,std::uint32_t offset);
    void UpdateDialog(BaseObjectState& state,std::uint32_t offset);
    void Clear();
    [[nodiscard]] std::size_t OwnedCount() const noexcept;
    // Script attachments share this owner's cleanup and named-resource policy.
    [[nodiscard]] IBaseObjectNamedWeaponRuntime& WeaponRuntime(BaseObjectState& state);
private:
    void RequireSlot(PlayerManagerSlotState& slot) const;
    void RequirePlayer(BaseObjectState& state) const;
    void CleanupNamedResource(BaseObjectState&,std::uint32_t,std::uint32_t) override;
    PlayerManagerObjectState& players_;
    std::unique_ptr<NativePlayerWeaponEffectHost> host_;
    NativePlayerNamedWeaponRuntime weapons_;
    NativePlayerEquipmentVisualRuntime visuals_;
};
}
