#include "pal4/base_object_attached_effect.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kObjectVisual = 0x530u;
constexpr std::uint32_t kEffect = 0x534u;
constexpr std::uint32_t kRotatedEffect = 0x53cu;

std::uint32_t UpdateAt(BaseObjectState& state, std::uint32_t offset,
                       IBaseObjectAttachedEffectRuntime& runtime) {
    if (runtime.EffectNeedsUpdate(state, offset)) {
        runtime.UpdateEffect(state, offset);
    }
    return 1u;
}

void StoreCreatedEffect(BaseObjectState& state, std::uint32_t offset,
                        BaseObjectEffectCreation& created,
                        IBaseObjectAttachedEffectRuntime& runtime) {
    state.Write(offset, created.persistent.first);
    state.Write(offset + 4u, created.persistent.second);
    runtime.ResetTemporary(created.temporary);
}

}  // namespace

std::uint32_t BaseObjectAttachedEffect::Setup(
    BaseObjectState& state, std::uint32_t effect_name,
    std::uint32_t mode, IBaseObjectAttachedEffectRuntime& runtime) {
    if (state.Read(0x1f0u) == 0u) return 0u;
    const auto visual = state.Read(kObjectVisual);
    if (visual == 0u) return 0u;

    const auto frame = runtime.ReadDword(visual + 4u);
    const auto position = runtime.GetObjectTransform(frame) + 48u;
    (void)UpdateAt(state, kEffect, runtime);
    const auto singleton = runtime.EffectSingleton();
    auto created = runtime.CreateEffectObject(singleton, effect_name,
                                              position, mode, 1u);
    StoreCreatedEffect(state, kEffect, created, runtime);
    runtime.SetVisibility(state, kEffect,
                          runtime.ReadDword(state.Read(kObjectVisual) + 4u));
    runtime.RefreshEffect(state, kEffect);
    return 1u;
}

std::uint32_t BaseObjectAttachedEffect::Update(
    BaseObjectState& state, IBaseObjectAttachedEffectRuntime& runtime) {
    return UpdateAt(state, kEffect, runtime);
}

std::uint32_t BaseObjectAttachedEffect::SetupRotated(
    BaseObjectState& state, std::uint32_t effect_name,
    std::uint32_t mode, IBaseObjectAttachedEffectRuntime& runtime) {
    if (state.Read(0x1f0u) == 0u || state.Read(kObjectVisual) == 0u) {
        return 0u;
    }

    (void)UpdateAt(state, kRotatedEffect, runtime);
    const auto singleton = runtime.EffectSingleton();
    auto created = runtime.CreateEffectObject(singleton, effect_name,
                                              0x84b1dcu, mode, 1u);
    StoreCreatedEffect(state, kRotatedEffect, created, runtime);
    runtime.SetRotation(state, kRotatedEffect, 90.0f, 0.0f, 0.0f, 0u);
    runtime.BindFrame(state, kRotatedEffect,
                      runtime.ReadDword(state.Read(kObjectVisual) + 4u));
    runtime.RefreshEffect(state, kRotatedEffect);
    return 1u;
}

std::uint32_t BaseObjectAttachedEffect::UpdateRotated(
    BaseObjectState& state, IBaseObjectAttachedEffectRuntime& runtime) {
    return UpdateAt(state, kRotatedEffect, runtime);
}

}  // namespace pal4
