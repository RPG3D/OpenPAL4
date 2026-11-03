#include "pal4/base_object_hand_effect.h"

namespace pal4 {
namespace {

std::uint32_t EffectOffset(BaseObjectHand hand) {
    return hand == BaseObjectHand::Right ? 0x520u : 0x528u;
}

std::uint32_t Update(BaseObjectState& state, BaseObjectHand hand,
                     IBaseObjectNamedWeaponRuntime& runtime) {
    const auto offset = EffectOffset(hand);
    if (runtime.EffectNeedsUpdate(state, offset)) {
        runtime.UpdateEffect(state, offset);
    }
    return 1u;
}

std::uint32_t Setup(BaseObjectState& state, std::uint32_t effect_name,
                    BaseObjectHand hand,
                    IBaseObjectNamedWeaponRuntime& runtime) {
    const auto clump = state.Read(0x14u);
    if (clump == 0u) return 0u;

    (void)Update(state, hand, runtime);
    const auto hook = runtime.GetUserData(
        clump, hand == BaseObjectHand::Right ?
                   "hook_ef_rhand" : "hook_ef_lhand");
    if (hook == 0u) return 0u;

    const auto offset = EffectOffset(hand);
    if (runtime.EffectNeedsUpdate(state, offset)) {
        runtime.UpdateEffect(state, offset);
    }
    if (effect_name != 0u && runtime.EffectAvailable(effect_name)) {
        const auto singleton = runtime.EffectSingleton();
        auto created = runtime.CreateEffect(singleton, effect_name,
                                            0x84b1dcu, 1u, 0u);
        state.Write(offset, created.persistent.first);
        state.Write(offset + 4u, created.persistent.second);
        runtime.ResetTemporary(created.temporary);
        runtime.SetEffectProperties(state, offset,
                                    runtime.ReadDword(hook + 4u));
        runtime.RefreshEffect(state, offset);
    }
    return 1u;
}

}  // namespace

std::uint32_t BaseObjectHandEffect::SetupRight(
    BaseObjectState& state, std::uint32_t effect_name,
    IBaseObjectNamedWeaponRuntime& runtime) {
    return Setup(state, effect_name, BaseObjectHand::Right, runtime);
}

std::uint32_t BaseObjectHandEffect::SetupLeft(
    BaseObjectState& state, std::uint32_t effect_name,
    IBaseObjectNamedWeaponRuntime& runtime) {
    return Setup(state, effect_name, BaseObjectHand::Left, runtime);
}

std::uint32_t BaseObjectHandEffect::UpdateRight(
    BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime) {
    return Update(state, BaseObjectHand::Right, runtime);
}

std::uint32_t BaseObjectHandEffect::UpdateLeft(
    BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime) {
    return Update(state, BaseObjectHand::Left, runtime);
}

std::uint32_t BaseObjectHandEffect::CleanupWeapons(
    BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime) {
    (void)BaseObjectWeapon::CleanupRight(state, runtime);
    (void)BaseObjectWeapon::CleanupLeft(state, runtime);
    return 1u;
}

std::uint32_t BaseObjectHandEffect::CleanupWeaponEffects(
    BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime) {
    (void)BaseObjectNamedWeapon::CleanupRight(state, runtime);
    (void)BaseObjectNamedWeapon::CleanupLeft(state, runtime);
    return 1u;
}

std::uint32_t BaseObjectHandEffect::UpdateWeaponEffects(
    BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime) {
    (void)UpdateRight(state, runtime);
    (void)UpdateLeft(state, runtime);
    return 1u;
}

std::uint32_t BaseObjectHandEffect::CleanupAll(
    BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime) {
    (void)CleanupWeapons(state, runtime);
    (void)CleanupWeaponEffects(state, runtime);
    (void)UpdateWeaponEffects(state, runtime);
    return 1u;
}

}  // namespace pal4
