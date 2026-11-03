#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IScriptNpcHandEffectRuntime {
public:
    virtual ~IScriptNpcHandEffectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindNpc(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t SetupLeftHandEffect(
        std::uint32_t npc, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t SetupRightHandEffect(
        std::uint32_t npc, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateWeaponEffects(
        std::uint32_t npc) = 0;
    [[nodiscard]] virtual bool NameDiffersFromEmptyError(
        std::uint32_t string_object, std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindObject(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void RemoveAnimatedObject(std::uint32_t arena,
                                      std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t RenderUnit(
        std::uint32_t npc, std::uint32_t object_type,
        const std::array<float, 3>& scale, std::uint32_t name,
        bool left_hand) = 0;
    [[nodiscard]] virtual std::uint32_t RenderObject(
        std::uint32_t npc, std::uint32_t object_type,
        std::uint32_t transform, std::uint32_t name,
        bool left_hand) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupWeapons(
        std::uint32_t npc) = 0;
};

class ScriptNpcHandEffect final {
public:
    // IDA 0x5d72b0, giEffectAttachToNpc.
    [[nodiscard]] static std::uint32_t Attach(
        std::uint32_t npc_name_object, std::uint32_t effect_name_object,
        std::uint8_t left_hand, IScriptNpcHandEffectRuntime& runtime);
    // IDA 0x5d7310, giEffectDetachFromNpc.
    [[nodiscard]] static std::uint32_t Detach(
        std::uint32_t npc_name_object, IScriptNpcHandEffectRuntime& runtime);
    // IDA 0x5d7340, giGOBAttachToNpc.
    [[nodiscard]] static std::uint32_t AttachGob(
        std::uint32_t gob_name_object, std::uint32_t render_name_object,
        std::uint32_t npc_name_object, std::uint8_t left_hand,
        IScriptNpcHandEffectRuntime& runtime);
    // IDA 0x5d7490, giGOBDetachFromNpc.
    [[nodiscard]] static std::uint32_t DetachGob(
        std::uint32_t npc_name_object, IScriptNpcHandEffectRuntime& runtime);
};

}  // namespace pal4
