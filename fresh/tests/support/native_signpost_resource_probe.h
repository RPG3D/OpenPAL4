#pragma once
#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_actor_user_data.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_arena_skill_manager_owner.h"
#include "pal4/effect_handle.h"
#include <rwcore.h>
#include <rpworld.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
// Resource prerequisite for IDA521C00. This does not create a manager record
// or substitute a model for the original492-byte ArenaActorResource.
inline void VerifyNativeSignpostResources(const std::filesystem::path& root,
                                         NativeEffectSceneSession& session) {
    NativeGobClumpAsset model;
    if (!model.Load(root, "gamedata\\PALObject\\M001\\", "M001"))
        throw std::runtime_error("original signpost M001: " + model.LastError());
    if (!model.Clump() || !model.AtomicCount() || model.MissingTextures() ||
        model.TextureDecodeFailures() || RpClumpGetWorld(model.Clump()) ||
        !RpClumpGetFrame(model.Clump()) || !NativeActorUserData::HideHooks(model.Clump()))
        throw std::runtime_error("original signpost M001 SDK resource prerequisites");

    const auto active = session.ActiveCount();
    auto& effects = session.ContentHandles();
    {
        NativeArenaSkillManagerOwner unbound(effects);
        const auto allocations = unbound.Storage().OutstandingAllocations();
        bool rejected = false;
        try { (void)unbound.CreateSignpostEffect({0.0f, 1.0f, 1.0f}); }
        catch (const std::logic_error&) { rejected = true; }
        if (!rejected || unbound.Storage().OutstandingAllocations() != allocations ||
            session.ActiveCount() != active)
            throw std::runtime_error("unbound signpost placement changed resources");
        unbound.Destroy();
        if (unbound.Storage().OutstandingAllocations())
            throw std::runtime_error("unbound signpost manager cleanup leaked");
    }
    const auto handle = session.ArenaSkills().CreateSignpostEffect({0.0f, 1.0f, 1.0f});
    struct Release {
        IEffectHandleContentRuntime& effects;
        EffectSystemHandleWords handle;
        ~Release() { if (handle.system) EffectHandle::Release(handle, true, effects); }
    } release{effects, {handle.id, handle.system}};
    if (!handle.system || !EffectHandle::DialogNeedsUpdate(release.handle, effects))
        throw std::runtime_error("original signpost M_001 effect creation");
    // Exercise real Dialog callbacks/resource playback; no user input or
    // desktop rendering is synthesized by this resource preparation probe.
    session.Update(0.016);
    if (!EffectHandle::DialogNeedsUpdate(release.handle, effects))
        throw std::runtime_error("original signpost M_001 ended during initial update");
    EffectHandle::Release(release.handle, true, effects);
    release.handle = {};
    if (EffectHandle::DialogNeedsUpdate({handle.id,handle.system}, effects) || session.ActiveCount() != active)
        throw std::runtime_error("original signpost M_001 release did not retire its Dialog");
    std::cout << "Native signpost resources: actual M001 SDK atomics=" << model.AtomicCount()
              << " missing_textures=0 M_001 Dialog update/release pass; actor/record binding pending\n";
}
}
