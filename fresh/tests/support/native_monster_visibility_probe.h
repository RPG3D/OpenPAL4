#pragma once
#include "pal4/native_script_monster_visibility_runtime.h"
#include "pal4/native_monster_manager_owner.h"
#include "pal4/native_monster_object_owner.h"
#include "pal4/native_scene_movement_asset.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <string>

namespace pal4::test {
inline void VerifyNativeMonsterVisibility(NativeMonsterManagerOwner& monsters,
    NativeScriptMonsterVisibilityRuntime& runtime, NativeSceneMovementAsset& movement,
    std::uint32_t collision, IGameResourceManagerRuntime& transforms) {
    const auto object = monsters.FindObject("m01-01-01");
    if (!object) throw std::runtime_error("original cave monster is missing");
    const auto source = runtime.ReadWord(object + 0x654u);
    if (!source) throw std::runtime_error("original cave monster has no behavior source");
    const auto document = runtime.ReadWord(source + 0x48u);
    const auto first_child = document
        ? monsters.OwnedComponent(object).Runtime().XmlChildByIndex(document, 0u) : 0u;
    const auto visible_before = runtime.ReadWord(object + 0x1f0u) != 0u;
    const std::array<std::uint32_t, 2> payload{
        0u, reinterpret_cast<std::uint32_t>("m01-01-01")};
    const auto name = reinterpret_cast<std::uint32_t>(payload.data());
    auto* clump = reinterpret_cast<RpClump*>(runtime.ReadWord(object + 0x14u));
    const auto verify = [&](bool visible) {
        if (runtime.ReadWord(object + 0x654u) != source ||
            runtime.ReadWord(object + 0x1f0u) != (visible ? 1u : 0u) ||
            runtime.ReadWord(object + 0x568u) != (visible ? collision : 0u) ||
            movement.IsObjectClumpRegistered(clump) != visible ||
            (runtime.ReadWord(object + 0x670u) & 0xffu) != (visible && document ? 1u : 0u) ||
            runtime.ReadWord(object + 0x6d0u) != (visible ? first_child : 0u))
            throw std::runtime_error("monster state/behavior/collision restoration mismatch visible=" +
                std::to_string(visible) + " source=" + std::to_string(runtime.ReadWord(object + 0x654u)) +
                " expected-source=" + std::to_string(source) +
                " state=" + std::to_string(runtime.ReadWord(object + 0x1f0u)) +
                " collision=" + std::to_string(runtime.ReadWord(object + 0x568u)) +
                " expected-collision=" + std::to_string(collision) +
                " movement=" + std::to_string(movement.IsObjectClumpRegistered(clump)) +
                " flag=" + std::to_string(runtime.ReadWord(object + 0x670u)) +
                " behavior=" + std::to_string(runtime.ReadWord(object + 0x6d0u)));
        if (visible) {
            const auto action = runtime.AnimationName(object + 0x170u);
            if (runtime.CompareIgnoreCase(action, "C01") && runtime.CompareIgnoreCase(action, "Z01"))
                throw std::runtime_error("monster idle animation was not restored");
        }
        // Notify/reset dispatch restores the original behavior position.
        for (unsigned axis = 0u; axis < 3u; ++axis)
            if (runtime.ReadWord(object + 0xacu + axis * 4u) !=
                runtime.ReadWord(source + 0x18u + axis * 4u))
                throw std::runtime_error("monster source position was not restored");
        if (visible && document)
            for (unsigned axis = 0u; axis < 3u; ++axis)
                if (runtime.ReadWord(object + 0xa0u + axis * 4u) !=
                    runtime.ReadWord(source + 0x24u + axis * 4u))
                    throw std::runtime_error("monster source rotation was not restored");
    };
    for (unsigned repeat = 0u; repeat < 2u; ++repeat) {
        const auto coordinate = [&](std::uint32_t offset) {
            return std::bit_cast<float>(runtime.ReadWord(source + offset));
        };
        // Displace the real clump so notify/reset cannot pass without applying
        // the original behavior position and rotation fields.
        monsters.OwnedComponent(object).SetTransform(
            {coordinate(0x18u) + 123.0f, coordinate(0x1cu), coordinate(0x20u)},
            {coordinate(0x24u), coordinate(0x28u) + 11.0f, coordinate(0x2cu)}, transforms);
        (void)ScriptMonsterVisibility::SetHide(name, 0u, runtime);
        verify(false);
        (void)ScriptMonsterVisibility::SetHide(name, 1u, runtime);
        verify(true);
        const auto registered = movement.RegisteredObjectAtomics();
        const auto allocations = movement.OutstandingAllocations();
        // M03 func2003 repeats SetHide(true) after the day scene is loaded.
        // Exercise the real shared entry callbacks and their later removal.
        (void)ScriptMonsterVisibility::SetHide(name, 1u, runtime);
        verify(true);
        if (movement.RegisteredObjectAtomics() != registered ||
            movement.OutstandingAllocations() != allocations)
            throw std::runtime_error("repeated monster entry duplicated movement ownership");
        (void)ScriptMonsterVisibility::SetVisible(name, 0u, runtime);
        // SetVisible(false) leaves scene but retains behavior until replacement.
        if (runtime.ReadWord(object + 0x1f0u) || runtime.ReadWord(object + 0x568u) ||
            movement.IsObjectClumpRegistered(clump))
            throw std::runtime_error("monster visible=false left scene membership");
        (void)ScriptMonsterVisibility::SetVisible(name, 1u, runtime);
        verify(true);
    }
    if (!visible_before) (void)ScriptMonsterVisibility::SetHide(name, 0u, runtime);
    const auto count = monsters.ObjectCount();
    const std::array<std::uint32_t, 2> missing{
        0u, reinterpret_cast<std::uint32_t>("missing-native-visibility-probe")};
    if (ScriptMonsterVisibility::SetHide(reinterpret_cast<std::uint32_t>(missing.data()), 1u, runtime) ||
        monsters.ObjectCount() != count)
        throw std::runtime_error("missing monster visibility mutated the manager");
}
}  // namespace pal4::test
