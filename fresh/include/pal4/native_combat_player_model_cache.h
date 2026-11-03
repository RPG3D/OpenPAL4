#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string_view>
#include <array>
namespace pal4 {
class NativeCombatPlayerActorOwner;
class NativeActorInstancePool;
class NativeActorInstanceOwner;
class NativeInventorySession;
class NativeScriptEventRouter;
class INativeSkillSystemServices;
class NativeCombatActorSkillRuntime;
// Owns Player1296/1300/1304 and all checked-out models. Actor, pool, inventory
// and model services outlive this cache. Close before releasing the actor.
// Activate is5390F0 model preparation, not stage entry or world activation.
class NativeCombatPlayerModelCache final {
public:
    NativeCombatPlayerModelCache(NativeCombatPlayerActorOwner&,NativeActorInstancePool&,
        NativeInventorySession&,std::int32_t version);
    ~NativeCombatPlayerModelCache();
    NativeCombatPlayerModelCache(const NativeCombatPlayerModelCache&)=delete;
    NativeCombatPlayerModelCache& operator=(const NativeCombatPlayerModelCache&)=delete;
    std::uint32_t FindOrLoad(std::string_view);
    std::uint32_t Activate();
    // Original539820 wrapper, including539350 requested-model fallback.
    // Explicit action execution may run inside the actor's turn dispatch.
    void PlayAction(std::string_view,std::uint8_t,INativeSkillSystemServices&,NativeScriptEventRouter&);
    // Player virtual568/539350; restore requested model without starting action.
    void RestoreRequestedModel(INativeSkillSystemServices&,NativeScriptEventRouter&);
    //5394E0/539560: original formation and facet events. Router subscribers
    //provide world activation; this does not replace the Combat coordinator.
    std::uint32_t EnterStage(NativeScriptEventRouter&,std::uint32_t context=0u);
    std::uint32_t LeaveStage(NativeScriptEventRouter&,std::uint32_t context=0u);
    // Original576 inside this Player's authorized9126/388 callback.
    std::uint32_t LeaveStageDuringSkill(NativeScriptEventRouter&,NativeCombatActorSkillRuntime&);
    std::uint32_t LeaveStageDuringReplacement(NativeScriptEventRouter&,NativeCombatActorSkillRuntime&);
    // Original virtual444 formation descriptor and child position, for5395E0.
    void ResetFormationPosition();
    std::array<float,3> FormationPosition();
    NativeActorInstanceOwner& Current();
    void DeactivateCurrent(std::uint32_t context=0u);
    // Host retirement only: no stage/scene events are substituted here.
    // On callback failure retain the remaining owners and retry Close.
    void Close(std::uint32_t context=0u);
    std::size_t Count() const;
    std::size_t OwnedModels() const;
    std::uint32_t ActorAddress() const;
    void BindSceneEvents();
    void UnbindSceneEvents() noexcept;
    void RestoreRenderCallbacks();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
