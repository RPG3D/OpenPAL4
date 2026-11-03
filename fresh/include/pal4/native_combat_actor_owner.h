#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace pal4 {
class NativeCombatStatusPropertyChain;
struct ActorManagerTreeGlobals;
class IEffectHandleReleaseRuntime;
namespace detail {class CombatActorOwnerRef;}
class NativeMonsterRecordCatalog;
class ICombatActorStatusRuntime;
class IScriptSkillContextRuntime;
class NativeCombatActorHealthRuntime;
class NativeCombatActorSkillEvents;
class NativeCombatActorSkillRuntime;
class IAnimationEventQueueActivityRuntime;

// Owns the native 0x490-byte monster actor constructor boundary. This is a
// detached actor: registration, child model, effects and action execution still
// require their real Combat services. The source catalog is borrowed only by
// Construct; copied property/text data survive its destruction.
class NativeCombatActorOwner final {
public:
    NativeCombatActorOwner();
    ~NativeCombatActorOwner();
    NativeCombatActorOwner(const NativeCombatActorOwner&) = delete;
    NativeCombatActorOwner& operator=(const NativeCombatActorOwner&) = delete;
    std::uint32_t Construct(const NativeMonsterRecordCatalog&, std::int32_t id);
    void Destroy();
    // Original Release(false) then ResetThunk for the matching unregistered
    // group only. Active/foreign handles still reject before owner mutation.
    void DestroyAfterEffectShutdown(std::uint32_t group,IEffectHandleReleaseRuntime&);
    [[nodiscard]] std::uint32_t Slot() const;
    void SetSlot(std::uint32_t slot);
    // Host-owned manager name at the existing actor812 text boundary.
    void SetName(std::string_view);
    [[nodiscard]] std::string_view Name() const;
    [[nodiscard]] std::string_view BaseName() const;
    // Original virtual452/531080: record984, distinct from model/base name.
    [[nodiscard]] std::string_view SequenceImage() const;
    // Original Monster tooltip text record1032, distinct from name968/image984.
    [[nodiscard]] std::string_view TooltipText() const;
    [[nodiscard]] std::uint32_t AttackSequenceFlag() const;
    [[nodiscard]] std::uint32_t AttackMode() const;
    [[nodiscard]] std::string_view SoundName(std::uint32_t selector) const;
    // Borrows real slot0/4/8/12 services for actor728. Clear and unbind before
    // destroying those services or this actor. Unbound empty queues may update.
    void BindAnimationActivities(IAnimationEventQueueActivityRuntime&);
    void UnbindAnimationActivities();
    std::uint32_t AddAnimation(std::uint32_t animation, bool owned, bool staggered=false);
    void UpdateAnimations(float elapsed);
    void DispatchAnimations(std::uint32_t argument);
    void ClearAnimations();
    [[nodiscard]] std::size_t AnimationCount() const;
    [[nodiscard]] bool AnimationActivitiesBound() const noexcept;
    // IDA531450. Borrow real skill/actor virtual dispatch services for this call.
    // Status dispatch must follow the monster property chain, including events;
    // the Player scalar status setter is not equivalent. No services are cached.
    [[nodiscard]] std::int32_t RebuildStats(
        ICombatActorStatusRuntime& health, IScriptSkillContextRuntime& skills);
    [[nodiscard]] std::uint32_t Address() const noexcept;
    [[nodiscard]] std::size_t OwnedAllocations() const noexcept;
    [[nodiscard]] std::size_t OwnedTexts() const noexcept;
private:
    void CopyPendingRecord(const NativeMonsterRecordCatalog&,std::uint32_t record);
    friend class detail::CombatActorOwnerRef;
    friend class NativeCombatActorHealthRuntime;
    friend class NativeCombatActorSkillEvents;
    friend class NativeCombatActorSkillRuntime;
    friend class NativeCombatActorActionProgress;
    friend class NativeCombatActorSelection;
    void AcquireSkillRuntime();
    void ReleaseSkillRuntime() noexcept;
    std::uint32_t AllocateSkillStorage(std::uint32_t bytes);
    void FreeSkillStorage(std::uint32_t address);
    void BeginActorDispatch();
    void RequireIdleDispatch() const;
    void EndActorDispatch() noexcept;
    void RequireDamagePropertyChain() const;
    NativeCombatStatusPropertyChain& PropertyChain();
    [[nodiscard]] std::uint32_t EmptyEffectTreeLowerBound(std::uint32_t tree) const;
    ActorManagerTreeGlobals& EffectTreeGlobals();
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace pal4
