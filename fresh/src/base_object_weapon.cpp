#include "pal4/base_object_weapon.h"

namespace pal4 {
namespace {

std::uint32_t Setup(BaseObjectState& state, std::uint32_t weapon,
                    std::uint32_t scale, std::uint32_t effect_name,
                    BaseObjectHand hand, IBaseObjectWeaponRuntime& runtime) {
    const auto base_clump = state.Read(0x14u);
    if (base_clump == 0u) return 0u;

    const bool right = hand == BaseObjectHand::Right;
    const auto weapon_offset = right ? 0x518u : 0x51cu;
    const auto effect_offset = right ? 0x520u : 0x528u;
    const auto hook_name = right ? "hook_RHWeapon" : "hook_LHWeapon";
    runtime.CleanupWeapon(state, hand);
    state.Write(weapon_offset, weapon);

    if (weapon != 0u) {
        const auto hand_hook = runtime.GetUserData(weapon, "hook_hand");
        if (hand_hook == 0u) return 0u;

        const auto frame = runtime.ReadDword(weapon + 4u);
        const auto hook_frame = runtime.ReadDword(hand_hook + 4u);
        runtime.InvertTransform(frame + 16u, hook_frame + 16u);
        runtime.ScaleFrame(frame, scale, 2u);
        runtime.UpdateFrame(frame);
        runtime.RotateMatrix(frame + 16u, 0x8433a0u, 90.0f, 2u);
        if (right) {
            runtime.RotateMatrix(frame + 16u, 0x8433acu, 180.0f, 2u);
        }
        runtime.UpdateFrame(frame);
        runtime.ProcessUserData(base_clump, hook_name,
                                runtime.ReadDword(weapon + 4u));
        const auto effect_hook = runtime.GetUserData(weapon, "hook_effect");
        if (runtime.EffectNeedsUpdate(state, effect_offset)) {
            runtime.UpdateEffect(state, effect_offset);
        }
        const auto singleton = runtime.EffectSingleton();
        auto created = runtime.CreateEffect(singleton, effect_name,
                                             0x84b1dcu, 1u, 0u);
        state.Write(effect_offset, created.persistent.first);
        state.Write(effect_offset + 4u, created.persistent.second);
        runtime.ResetTemporary(created.temporary);
        const auto target_frame = effect_hook != 0u ?
            runtime.ReadDword(effect_hook + 4u) :
            runtime.ReadDword(weapon + 4u);
        runtime.SetEffectProperties(state, effect_offset, target_frame);
        runtime.RefreshEffect(state, effect_offset);
        const auto arena = runtime.ArenaManager();
        runtime.AddClump(arena, weapon);
        return 1u;
    }

    if (!runtime.EffectAvailable(effect_name)) return 1u;
    const auto body_hook = runtime.GetUserData(base_clump, hook_name);
    if (body_hook == 0u) return 0u;
    if (runtime.EffectNeedsUpdate(state, effect_offset)) {
        runtime.UpdateEffect(state, effect_offset);
    }
    const auto singleton = runtime.EffectSingleton();
    auto created = runtime.CreateEffect(singleton, effect_name,
                                         0x84b1dcu, 1u, 0u);
    state.Write(effect_offset, created.persistent.first);
    state.Write(effect_offset + 4u, created.persistent.second);
    runtime.ResetTemporary(created.temporary);
    runtime.SetEffectVisibility(state, effect_offset,
                                runtime.ReadDword(body_hook + 4u));
    runtime.RefreshEffect(state, effect_offset);
    return 1u;
}

std::uint32_t Cleanup(BaseObjectState& state, BaseObjectHand hand,
                      IBaseObjectWeaponRuntime& runtime) {
    if (state.Read(0x14u) == 0u) return 0u;

    const bool right = hand == BaseObjectHand::Right;
    const auto weapon_offset = right ? 0x518u : 0x51cu;
    const auto effect_offset = right ? 0x520u : 0x528u;
    if (runtime.EffectNeedsUpdate(state, effect_offset)) {
        runtime.UpdateEffect(state, effect_offset);
    }
    const auto weapon = state.Read(weapon_offset);
    if (weapon == 0u) return 1u;

    const auto arena = runtime.ArenaManager();
    runtime.RemoveClump(arena, weapon);
    const auto current_weapon = state.Read(weapon_offset);
    const auto frame = runtime.ReadDword(current_weapon + 4u);
    runtime.RemoveFrameChild(state.Read(0x14u),
                             right ? "hook_RHWeapon" : "hook_LHWeapon",
                             frame);
    state.Write(weapon_offset, 0u);
    return 1u;
}

}  // namespace

void IBaseObjectWeaponRuntime::CleanupWeapon(
    BaseObjectState& state, BaseObjectHand hand) {
    if (hand == BaseObjectHand::Right) {
        (void)BaseObjectWeapon::CleanupRight(state, *this);
    } else {
        (void)BaseObjectWeapon::CleanupLeft(state, *this);
    }
}

std::uint32_t BaseObjectWeapon::SetupRight(
    BaseObjectState& state, std::uint32_t weapon,
    std::uint32_t scale, std::uint32_t effect_name,
    IBaseObjectWeaponRuntime& runtime) {
    return Setup(state, weapon, scale, effect_name,
                 BaseObjectHand::Right, runtime);
}

std::uint32_t BaseObjectWeapon::SetupLeft(
    BaseObjectState& state, std::uint32_t weapon,
    std::uint32_t scale, std::uint32_t effect_name,
    IBaseObjectWeaponRuntime& runtime) {
    return Setup(state, weapon, scale, effect_name,
                 BaseObjectHand::Left, runtime);
}

std::uint32_t BaseObjectWeapon::CleanupRight(
    BaseObjectState& state, IBaseObjectWeaponRuntime& runtime) {
    return Cleanup(state, BaseObjectHand::Right, runtime);
}

std::uint32_t BaseObjectWeapon::CleanupLeft(
    BaseObjectState& state, IBaseObjectWeaponRuntime& runtime) {
    return Cleanup(state, BaseObjectHand::Left, runtime);
}

}  // namespace pal4
