#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
#include <span>
#include "pal4/animation_string_table.h"

namespace pal4 {
class NativeAnimatedActorAsset;
class INativeSkillSystemServices;
class IActorAnimationControlRuntime;
class NativeScriptEventRouter;
class NativePkFileManagerRuntime;
class NativeEffectSceneSession;
struct ActorAttachmentLoadArgs;
struct ResourceVec3;

// Owned 0x610-byte Combat model child. Uses the original ActorInstance
// lifecycle, not the scene NPC layout. SDK engine and game image outlive it.
// Resource preparation does not register a monster or dispatch me_new_added.
class NativeActorInstanceOwner final {
public:
    NativeActorInstanceOwner();
    ~NativeActorInstanceOwner();
    NativeActorInstanceOwner(const NativeActorInstanceOwner&) = delete;
    NativeActorInstanceOwner& operator=(const NativeActorInstanceOwner&) = delete;
    std::uint32_t Construct(std::uint32_t game_instance);
    void Load(const std::filesystem::path& game_root, std::string_view model);
    // Bind owned hand loading after body preparation. Services and Combat
    // activity provider outlive the model, including time spent in the pool.
    void BindHandResources(const std::filesystem::path&,NativePkFileManagerRuntime&,
        NativeEffectSceneSession&,INativeSkillSystemServices&,std::function<bool()> inactive);
    std::uint32_t LoadHand(bool right,const ActorAttachmentLoadArgs&);
    void ClearHandResources();
    // IDA52AA20: select the default combat animation, restore material RGBA
    // from520 and rebuild the clump transform/bounds. Requires loaded records.
    [[nodiscard]] std::uint32_t InitializeFrame();
    // Borrowed52ACB0 adapter for this loaded model; invalidate on return/reload.
    // Populated attachments require this owner's bound hand resource services.
    // Geometry payload is the real transparent render queue, not a callback.
    [[nodiscard]] IActorAnimationControlRuntime& AnimationRuntime();
    //6664A0(enabled0,context0), including the original shadow/light guard.
    void DisableCamera();
    //52ADF0: install the actual queue callbacks, enable the shadow camera
    //when resources exist, then activate both hands. World/queue are borrowed.
    void Activate(std::uint32_t world,std::uint32_t queue,std::uint32_t effect_context=0u);
    [[nodiscard]] std::uint32_t SetPosition(const ResourceVec3& position);
    // Actual child slot8 transform, retaining original absolute/relative rules.
    void SetRotation(const ResourceVec3&,bool relative=false);
    //52B9D0 through the shared action algorithm and native borrowed clips.
    // Hand activation forwards through the bound resource/effect owners.
    void PlayAction(std::uint32_t actor,std::string_view,std::uint8_t,
                    INativeSkillSystemServices&);
    // Owned child primitives used by Player539820/539350 and shared52B9D0.
    bool HasActionAnimation(std::string_view filename) const;
    void SetHandActive(bool right,bool active,std::uint32_t context);
    void SetActionRestartFlag(std::uint8_t);
    void SetActionName(std::string_view);
    // Borrowed current-record name; invalidated by clip/model changes.
    [[nodiscard]] std::string_view AnimationRecordName() const;
    // Borrowed lazy AMF parameter tables for the actual current record.
    [[nodiscard]] std::span<const AnimationStringTableData> AnimationParameters();
    [[nodiscard]] std::uint32_t AnimationCompletedFlag() const;
    //52AC10 body/embedded animation/hands, or667590 camera/shadow phase.
    // Caller begins camera; no elapsed-time advancement occurs here.
    void RenderSlot(std::uint32_t camera,std::uint32_t slot);
    std::uint32_t Update(float elapsed,NativeScriptEventRouter&);
    //52AE70 pool-return preparation. Validate both hand owners before camera
    // cleanup; retire effects and restore body/hand queue callbacks afterward.
    void Deactivate(std::uint32_t effect_context = 0u);
    // Retire borrowed queue callbacks without changing activity or effects.
    void RestoreRenderCallbacks();
    //52A9C0 stores both parent fields. Host metadata tracks whether the parent
    // still owns an active association; returned cache entries retain fields.
    [[nodiscard]] std::uint32_t BindParent(std::uint32_t actor);
    // Preflight the live association and both hand owners before equipment work.
    void ValidateParent(std::uint32_t actor);
    void ReleaseToPool(std::uint32_t effect_context = 0u);
    void Destroy();
    [[nodiscard]] std::uint32_t Address() const noexcept;
    [[nodiscard]] const NativeAnimatedActorAsset* Asset() const noexcept;
    [[nodiscard]] std::string_view ModelName() const noexcept;
    [[nodiscard]] std::size_t OwnedAllocations() const noexcept;
    [[nodiscard]] std::size_t OwnedTexts() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
