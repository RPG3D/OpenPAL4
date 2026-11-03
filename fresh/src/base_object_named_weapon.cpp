#include "pal4/base_object_named_weapon.h"

namespace pal4 {
namespace {

void ReplaceXmlEffect(BaseObjectState& state, std::uint32_t offset,
                      std::uint32_t effect_name, std::uint32_t clump,
                      IBaseObjectNamedWeaponRuntime& runtime) {
    if (runtime.EffectReady(state, offset)) {
        runtime.DisplayAndCleanupEffect(state, offset);
    }
    if (effect_name == 0u || !runtime.EffectAvailable(effect_name)) return;
    const auto singleton = runtime.EffectSingleton();
    const auto effect = runtime.LoadXmlEffect(singleton, effect_name, clump);
    state.Write(offset, effect.first);
    state.Write(offset + 4u, effect.second);
}

std::uint32_t Setup(BaseObjectState& state, std::string_view model,
                    std::string_view texture, std::uint32_t directory,
                    std::uint32_t effect0, std::uint32_t effect1,
                    BaseObjectHand hand,
                    IBaseObjectNamedWeaponRuntime& runtime) {
    if (state.Read(0x14u) == 0u) return 0u;

    const bool right = hand == BaseObjectHand::Right;
    const auto resource_offset = right ? 0x218u : 0x388u;
    const auto clump_offset = right ? 0x22cu : 0x39cu;
    const auto effect_offset = right ? 0x4f8u : 0x508u;
    const auto label = right ? "PALRole-RHWeapon" : "PALRole-LHWeapon";
    const auto hook_name = right ? "hook_RHWeapon" : "hook_LHWeapon";
    runtime.CleanupNamed(state, hand);
    runtime.AddAndProcessScripts();
    auto path = runtime.CreatePath(directory);
    bool path_cleaned = false;
    const auto cleanup_path = [&] {
        path_cleaned = true;
        runtime.CleanupPath(path);
    };
    try {
        path.append(model);
        path.push_back('\\');
        const auto game = runtime.GameInstance();
        if (!runtime.LoadClump(state, resource_offset, label, path, model,
                               game + 0x1acu, 0u, 0u, 1u)) {
            runtime.CleanupScripts();
            cleanup_path();
            return 0u;
        }
        if (!texture.empty()) {
            path.append(texture);
            const auto dictionary = runtime.TextureDictionary();
            const auto resolved = runtime.ResolveTexturePath(dictionary, path);
            runtime.LoadTexture(state, 0x388u, texture, resolved);
        }
        runtime.CleanupScripts();

        const auto clump = state.Read(clump_offset);
        const auto hand_hook = runtime.GetUserData(clump, "hook_hand");
        if (hand_hook == 0u) {
            cleanup_path();
            return 0u;
        }
        runtime.HideHookAtomics(clump);
        const auto frame = runtime.ReadDword(clump + 4u);
        const auto hook_frame = runtime.ReadDword(hand_hook + 4u);
        runtime.InvertTransform(frame + 16u, hook_frame + 16u);
        runtime.UpdateFrame(frame);
        const auto angle = right ? 90.0f :
            (runtime.CompareRoleToLinsha(state.Read(4u)) == 0 ?
                 -90.0f : 90.0f);
        runtime.RotateMatrix(frame + 16u, 0x8433a0u, angle, 2u);
        if (right) {
            runtime.RotateMatrix(frame + 16u, 0x8433acu, 180.0f, 2u);
        }
        runtime.UpdateFrame(frame);
        runtime.ProcessUserData(state.Read(0x14u), hook_name,
                                runtime.ReadDword(state.Read(clump_offset) + 4u));

        ReplaceXmlEffect(state, effect_offset, effect0,
                         state.Read(clump_offset), runtime);
        ReplaceXmlEffect(state, effect_offset + 8u, effect1,
                         state.Read(clump_offset), runtime);
        const auto weapon_clump = state.Read(clump_offset);
        const auto arena = runtime.ArenaManager();
        runtime.AddClump(arena, weapon_clump);
        cleanup_path();
        return 1u;
    } catch (...) {
        if (!path_cleaned) cleanup_path();
        throw;
    }
}

std::uint32_t Cleanup(BaseObjectState& state, BaseObjectHand hand,
                      IBaseObjectNamedWeaponRuntime& runtime) {
    const bool right = hand == BaseObjectHand::Right;
    const auto clump_offset = right ? 0x22cu : 0x39cu;
    if (state.Read(0x14u) == 0u || state.Read(clump_offset) == 0u) {
        return 0u;
    }
    const auto weapon = state.Read(clump_offset);
    const auto arena = runtime.ArenaManager();
    runtime.RemoveClump(arena, weapon);
    const auto frame = runtime.ReadDword(state.Read(clump_offset) + 4u);
    runtime.RemoveFrameChild(state.Read(0x14u),
                             right ? "hook_RHWeapon" : "hook_LHWeapon",
                             frame);
    const auto effect_offset = right ? 0x4f8u : 0x508u;
    for (auto offset : {effect_offset, effect_offset + 8u}) {
        if (runtime.EffectReady(state, offset)) {
            runtime.DisplayAndCleanupEffect(state, offset);
        }
    }
    const auto game = runtime.GameInstance();
    runtime.CleanupClump(state, right ? 0x218u : 0x388u,
                         game + 0x1acu);
    return 1u;
}

}  // namespace

void IBaseObjectNamedWeaponRuntime::CleanupNamed(
    BaseObjectState& state, BaseObjectHand hand) {
    if (hand == BaseObjectHand::Right) {
        (void)BaseObjectNamedWeapon::CleanupRight(state, *this);
    } else {
        (void)BaseObjectNamedWeapon::CleanupLeft(state, *this);
    }
}

std::uint32_t BaseObjectNamedWeapon::SetupRight(
    BaseObjectState& state, std::string_view model,
    std::string_view texture, std::uint32_t directory,
    std::uint32_t effect0, std::uint32_t effect1,
    IBaseObjectNamedWeaponRuntime& runtime) {
    return Setup(state, model, texture, directory, effect0, effect1,
                 BaseObjectHand::Right, runtime);
}

std::uint32_t BaseObjectNamedWeapon::SetupLeft(
    BaseObjectState& state, std::string_view model,
    std::string_view texture, std::uint32_t directory,
    std::uint32_t effect0, std::uint32_t effect1,
    IBaseObjectNamedWeaponRuntime& runtime) {
    return Setup(state, model, texture, directory, effect0, effect1,
                 BaseObjectHand::Left, runtime);
}

std::uint32_t BaseObjectNamedWeapon::CleanupRight(
    BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime) {
    return Cleanup(state, BaseObjectHand::Right, runtime);
}

std::uint32_t BaseObjectNamedWeapon::CleanupLeft(
    BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime) {
    return Cleanup(state, BaseObjectHand::Left, runtime);
}

}  // namespace pal4
