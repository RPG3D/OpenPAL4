#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCombatActorModelOwner;
class NativeCombatPlayerModelCache;
class NativeCombatActorSkillRuntime;
class NativeCombatSkillActorRegistry;
class INativeSkillSystemServices;
class NativeEffectSceneSession;
class NativeScriptEventRouter;
class NativeCombatNormalAttackProcessor;
class NativeCombatReactionDomain;
class NativeCombatPhysicalAttackActions;
// Per-source composition of existing playback, AMF and physical task owners.
// Borrowed actors, processor, reaction services and scale outlive this object;
// the shared queue must drain and detach before Close.
class NativeCombatPhysicalAttackDomain final {
public:
    using Motion=std::function<void(std::uint32_t,std::uint32_t)>;
    using EndFactory=std::function<std::uint32_t(std::uint32_t)>;
    NativeCombatPhysicalAttackDomain(NativeCombatActorModelOwner&,NativeCombatActorSkillRuntime&,
        NativeCombatSkillActorRegistry&,INativeSkillSystemServices&,NativeEffectSceneSession&,
        NativeScriptEventRouter&,float&,NativeCombatNormalAttackProcessor&,NativeCombatReactionDomain&,Motion,EndFactory={});
    NativeCombatPhysicalAttackDomain(NativeCombatPlayerModelCache&,NativeCombatActorSkillRuntime&,
        NativeCombatSkillActorRegistry&,INativeSkillSystemServices&,NativeEffectSceneSession&,
        NativeScriptEventRouter&,float&,NativeCombatNormalAttackProcessor&,NativeCombatReactionDomain&,Motion,EndFactory={});
    ~NativeCombatPhysicalAttackDomain();
    NativeCombatPhysicalAttackDomain(const NativeCombatPhysicalAttackDomain&)=delete;
    NativeCombatPhysicalAttackDomain& operator=(const NativeCombatPhysicalAttackDomain&)=delete;
    NativeCombatPhysicalAttackActions& Actions();
    std::uint32_t ActorAddress() const noexcept;
    std::size_t Count() const noexcept;
    std::size_t OwnedAllocations() const noexcept;
    std::size_t OwnedTexts() const noexcept;
    std::size_t Subscriptions() const noexcept;
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
