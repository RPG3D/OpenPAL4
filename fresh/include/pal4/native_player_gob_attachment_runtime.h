#pragma once
#include "pal4/script_player_gob_attachment.h"

namespace pal4 {
class NativePlayerManagerSession;
class NativePlayerEquipmentSession;
class NativeSceneGobAssets;
class NativeScriptObjectVisibilityRuntime;
struct BaseObjectState;

// Borrows scene owners. Equipment owns hand cleanup; GOB assets retain clumps.
// Retire this adapter before equipment, visibility, GOBs or player slots.
class NativePlayerGobAttachmentRuntime final : public IScriptPlayerGobAttachmentRuntime {
public:
    NativePlayerGobAttachmentRuntime(NativePlayerManagerSession& players,
        NativePlayerEquipmentSession& equipment, NativeSceneGobAssets& objects,
        NativeScriptObjectVisibilityRuntime& visibility)
        : players_(players), equipment_(equipment), objects_(objects), visibility_(visibility) {}
    std::uint32_t PlayerManager() override;
    std::uint32_t PlayerByIndex(std::uint32_t, std::int32_t) override;
    std::uint32_t ReadWord(std::uint32_t) override;
    bool NameDiffersFromEmptyError(std::uint32_t, std::uint32_t) override;
    std::uint32_t EmptyStringAddress() override;
    std::uint32_t ObjectManager() override;
    std::uint32_t FindObject(std::uint32_t, std::uint32_t) override;
    std::uint32_t ArenaManager() override;
    void RemoveAnimatedObject(std::uint32_t, std::uint32_t) override;
    std::uint32_t RenderUnit(std::uint32_t, std::uint32_t,
        const std::array<float, 3>&, std::uint32_t, bool) override;
    std::uint32_t RenderObject(std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t, bool) override;
    std::uint32_t CleanupWeapons(std::uint32_t) override;
    std::uint32_t SetupLeftHandEffect(std::uint32_t, std::uint32_t) override;
    std::uint32_t SetupRightHandEffect(std::uint32_t, std::uint32_t) override;
    std::uint32_t UpdateWeaponEffects(std::uint32_t) override;
private:
    BaseObjectState& Player(std::uint32_t);
    NativePlayerManagerSession& players_;
    NativePlayerEquipmentSession& equipment_;
    NativeSceneGobAssets& objects_;
    NativeScriptObjectVisibilityRuntime& visibility_;
};
}
