#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IScriptPlayerGobAttachmentRuntime {
public:
    virtual ~IScriptPlayerGobAttachmentRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool NameDiffersFromEmptyError(
        std::uint32_t string_object, std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindObject(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void RemoveAnimatedObject(std::uint32_t arena,
                                      std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t RenderUnit(
        std::uint32_t player, std::uint32_t object_type,
        const std::array<float, 3>& scale, std::uint32_t name,
        bool left_hand) = 0;
    [[nodiscard]] virtual std::uint32_t RenderObject(
        std::uint32_t player, std::uint32_t object_type,
        std::uint32_t transform, std::uint32_t name,
        bool left_hand) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupWeapons(
        std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint32_t SetupLeftHandEffect(
        std::uint32_t player, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t SetupRightHandEffect(
        std::uint32_t player, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateWeaponEffects(
        std::uint32_t player) = 0;
};

class ScriptPlayerGobAttachment final {
public:
    // IDA 0x5d6ed0, giGOBAttachToPlayer.
    [[nodiscard]] static std::uint32_t Attach(
        std::uint32_t gob_name_object, std::uint32_t render_name_object,
        std::int32_t player_index, std::uint8_t left_hand,
        IScriptPlayerGobAttachmentRuntime& runtime);
    // IDA 0x5d7010, giGOBAttachToCurrentPlayer.
    [[nodiscard]] static std::uint32_t AttachToCurrent(
        std::uint32_t gob_name_object, std::uint32_t render_name_object,
        std::uint8_t left_hand,
        IScriptPlayerGobAttachmentRuntime& runtime);
    // IDA 0x5d7160, giGOBDetachFromPlayer.
    [[nodiscard]] static std::uint32_t Detach(
        std::int32_t player_index,
        IScriptPlayerGobAttachmentRuntime& runtime);
    // IDA 0x5d7180, giGOBDetachFromCurrentPlayer.
    [[nodiscard]] static std::uint32_t DetachCurrent(
        IScriptPlayerGobAttachmentRuntime& runtime);
    // IDA 0x5d71b0, giEffectAttachToPlayer.
    [[nodiscard]] static std::uint32_t AttachEffect(
        std::int32_t player_index, std::uint32_t name_object,
        std::uint8_t left_hand,
        IScriptPlayerGobAttachmentRuntime& runtime);
    // IDA 0x5d7200, giEffectAttachToCurrentPlayer.
    [[nodiscard]] static std::uint32_t AttachEffectToCurrent(
        std::uint32_t name_object, std::uint8_t left_hand,
        IScriptPlayerGobAttachmentRuntime& runtime);
    // IDA 0x5d7260, giEffectDetachFromPlayer.
    [[nodiscard]] static std::uint32_t DetachEffect(
        std::int32_t player_index,
        IScriptPlayerGobAttachmentRuntime& runtime);
    // IDA 0x5d7280, giEffectDetachFromCurrentPlayer.
    [[nodiscard]] static std::uint32_t DetachEffectFromCurrent(
        IScriptPlayerGobAttachmentRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t AttachToPlayer(
        std::uint32_t gob_name_object, std::uint32_t render_name_object,
        std::uint32_t player, std::uint8_t left_hand,
        IScriptPlayerGobAttachmentRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyEffect(
        std::uint32_t player, std::uint32_t name_object,
        std::uint8_t left_hand,
        IScriptPlayerGobAttachmentRuntime& runtime);
};

}  // namespace pal4
