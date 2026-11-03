#include "pal4/script_player_gob_attachment.h"

namespace pal4 {

namespace {

std::uint32_t ResolveName(std::uint32_t payload,
                          IScriptPlayerGobAttachmentRuntime& runtime) {
    const auto name = runtime.ReadWord(payload + 4u);
    return name == 0u ? runtime.EmptyStringAddress() : name;
}

}  // namespace

std::uint32_t ScriptPlayerGobAttachment::Attach(
    std::uint32_t gob_name_object, std::uint32_t render_name_object,
    std::int32_t player_index, std::uint8_t left_hand,
    IScriptPlayerGobAttachmentRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                              player_index);
    if (player == 0u) return 0u;
    return AttachToPlayer(gob_name_object, render_name_object, player,
                          left_hand, runtime);
}

std::uint32_t ScriptPlayerGobAttachment::AttachToCurrent(
    std::uint32_t gob_name_object, std::uint32_t render_name_object,
    std::uint8_t left_hand, IScriptPlayerGobAttachmentRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    const auto player = manager + index * 0xb14u;
    if (player == 0u) return manager;
    return AttachToPlayer(gob_name_object, render_name_object, player,
                          left_hand, runtime);
}

std::uint32_t ScriptPlayerGobAttachment::Detach(
    std::int32_t player_index,
    IScriptPlayerGobAttachmentRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                              player_index);
    return player == 0u ? 0u : runtime.CleanupWeapons(player);
}

std::uint32_t ScriptPlayerGobAttachment::DetachCurrent(
    IScriptPlayerGobAttachmentRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    const auto player = manager + index * 0xb14u;
    return player == 0u ? manager : runtime.CleanupWeapons(player);
}

std::uint32_t ScriptPlayerGobAttachment::AttachEffect(
    std::int32_t player_index, std::uint32_t name_object,
    std::uint8_t left_hand, IScriptPlayerGobAttachmentRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                              player_index);
    return player == 0u ? 0u :
        ApplyEffect(player, name_object, left_hand, runtime);
}

std::uint32_t ScriptPlayerGobAttachment::AttachEffectToCurrent(
    std::uint32_t name_object, std::uint8_t left_hand,
    IScriptPlayerGobAttachmentRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    const auto player = manager + index * 0xb14u;
    return player == 0u ? manager :
        ApplyEffect(player, name_object, left_hand, runtime);
}

std::uint32_t ScriptPlayerGobAttachment::ApplyEffect(
    std::uint32_t player, std::uint32_t name_object,
    std::uint8_t left_hand, IScriptPlayerGobAttachmentRuntime& runtime) {
    const auto name = ResolveName(name_object, runtime);
    return left_hand != 0u ? runtime.SetupLeftHandEffect(player, name) :
                             runtime.SetupRightHandEffect(player, name);
}

std::uint32_t ScriptPlayerGobAttachment::DetachEffect(
    std::int32_t player_index,
    IScriptPlayerGobAttachmentRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                              player_index);
    return player == 0u ? 0u : runtime.UpdateWeaponEffects(player);
}

std::uint32_t ScriptPlayerGobAttachment::DetachEffectFromCurrent(
    IScriptPlayerGobAttachmentRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    const auto player = manager + index * 0xb14u;
    return player == 0u ? manager : runtime.UpdateWeaponEffects(player);
}

std::uint32_t ScriptPlayerGobAttachment::AttachToPlayer(
    std::uint32_t gob_name_object, std::uint32_t render_name_object,
    std::uint32_t player, std::uint8_t left_hand,
    IScriptPlayerGobAttachmentRuntime& runtime) {
    const auto gob_name_length = runtime.ReadWord(gob_name_object + 8u);
    if (!runtime.NameDiffersFromEmptyError(gob_name_object, gob_name_length)) {
        if (runtime.ReadWord(render_name_object + 8u) == 0u)
            return render_name_object;
        const auto render_name = ResolveName(render_name_object, runtime);
        const std::array<float, 3> unit_scale{1.0f, 1.0f, 1.0f};
        return runtime.RenderUnit(player, 0u, unit_scale, render_name,
                                  left_hand != 0u);
    }

    const auto gob_name = ResolveName(gob_name_object, runtime);
    const auto object = runtime.FindObject(runtime.ObjectManager(), gob_name);
    if (object == 0u) return 0u;
    runtime.RemoveAnimatedObject(runtime.ArenaManager(), object);
    const auto render_name = ResolveName(render_name_object, runtime);
    const auto object_type = runtime.ReadWord(object + 0x14u);
    return runtime.RenderObject(player, object_type, object + 0xb8u,
                                render_name, left_hand != 0u);
}

}  // namespace pal4
