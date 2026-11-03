#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>
#include <utility>
namespace pal4 {
class NativeCombatStatusPropertyChain;
struct ActorManagerTreeGlobals;
namespace detail {class CombatActorOwnerRef;}
class NativePlayerManagerSession;
class NativeInventorySession;
class ICombatActorStatusRuntime;
class IScriptSkillContextRuntime;
class IAnimationEventQueueActivityRuntime;
class IActorManagerPlayerDataTransferRuntime;
class IEffectHandleReleaseRuntime;
// Owns the actual537940 Player combat image and copied host strings/arrays.
// Construct borrows a live, loaded scene-player session only for the copy.
// Model lists, registration, skills and combat dispatch are separate services.
class NativeCombatPlayerActorOwner final {
public:
    NativeCombatPlayerActorOwner();
    ~NativeCombatPlayerActorOwner();
    NativeCombatPlayerActorOwner(const NativeCombatPlayerActorOwner&)=delete;
    NativeCombatPlayerActorOwner& operator=(const NativeCombatPlayerActorOwner&)=delete;
    std::uint32_t Construct(NativePlayerManagerSession&,std::uint32_t index);
    void Destroy();
    void ValidateDetached() const;
    void ValidateAfterEffectShutdown(std::uint32_t group,IEffectHandleReleaseRuntime&) const;
    // Original Release(false) then ResetThunk for the matching unregistered
    // group only. Active/foreign handles still reject before owner mutation.
    void DestroyAfterEffectShutdown(std::uint32_t group,IEffectHandleReleaseRuntime&);
    void SetName(std::string_view);
    std::string_view Name() const;
    std::string_view BaseName() const;
    // Original virtual452/538140: role index1192 with >=4 fallback to zero.
    std::string_view SequenceImage() const;
    std::string_view RequestedModel() const;
    [[nodiscard]] std::uint32_t AttackSequenceFlag() const;
    [[nodiscard]] std::uint32_t AttackMode() const;
    [[nodiscard]] std::string_view SoundName(std::uint32_t selector) const;
    void SetSlot(std::uint32_t);
    std::uint32_t Slot() const;
    // Original actor728 queue. Borrow real task slot0/4/8/12 services; clear
    // and unbind before retiring services/actor. Empty queues update unbound.
    void BindAnimationActivities(IAnimationEventQueueActivityRuntime&);
    void UnbindAnimationActivities();
    std::uint32_t AddAnimation(std::uint32_t animation,bool owned,bool staggered=false);
    void UpdateAnimations(float elapsed);
    void DispatchAnimations(std::uint32_t argument);
    void ClearAnimations();
    std::size_t AnimationCount() const;
    bool AnimationActivitiesBound() const noexcept;
    std::int32_t RebuildStats(ICombatActorStatusRuntime&,IScriptSkillContextRuntime&,NativeInventorySession&);
    // Pins the real actor throughout synchronous outcome callbacks.
    std::uint32_t WriteBack(NativePlayerManagerSession&,std::uint32_t index,
                           IActorManagerPlayerDataTransferRuntime&);
    std::uint32_t Address() const noexcept;
    std::size_t OwnedAllocations() const noexcept;
    std::size_t OwnedTexts() const noexcept;
    std::size_t OwnedArrays() const noexcept;
private:
    friend class detail::CombatActorOwnerRef;
    friend class NativeCombatActorHealthRuntime;
    friend class NativeCombatActorActionProgress;
    void RequireDamagePropertyChain() const;
    NativeCombatStatusPropertyChain& PropertyChain();
    void AcquireSkillRuntime();
    std::vector<std::pair<std::uint32_t,std::uint32_t>> CopiedSkillStorage() const;
    void ReleaseSkillRuntime() noexcept;
    std::uint32_t AllocateSkillStorage(std::uint32_t);
    void FreeSkillStorage(std::uint32_t);
    bool HasLearnedStunt(std::uint32_t) const;
    std::vector<std::uint32_t> LearnedStunts() const;
    void InsertLearnedStunt(std::uint32_t);
    std::uint32_t EmptyEffectTreeLowerBound(std::uint32_t) const;
    ActorManagerTreeGlobals& EffectTreeGlobals();
    friend class NativeCombatPlayerManager;
    friend class NativeCombatPlayerEquipment;
    friend class NativeCombatPlayerModelCache;
    friend class NativeCombatPlayerMotion;
    void AcquireModels();
    void RequireModelMutation() const;
    void ReleaseModels() noexcept;
    void BeginDispatch();
    bool DispatchActive() const noexcept;
    void RequireIdleDispatch() const;
    void EndDispatch() noexcept;
    void AcquireManager();
    void ReleaseManager() noexcept;
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
