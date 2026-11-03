#pragma once
#include <memory>
#include <functional>
#include <string_view>
#include <cstdint>
#include <cstddef>
namespace pal4 {
class NativeActorInstanceOwner;
class NativeCombatActorSkillRuntime;
class NativeEffectSceneSession;
class NativeScriptEventRouter;
class ICombatActionTaskRuntime;
class IGomTaskBaseRuntime;
class IGomTaskContainerRuntime;
class NativeBaseObjectMemory;
struct EffectSystemHandleWords;
// Borrowed actual Magic/Property lifecycle. This owner controls allocation/base
// messages; the typed consumer owns auxiliary subscriptions/effect/skill dispatch.
class INativeCombatMessageChildServices {
public:
    virtual ~INativeCombatMessageChildServices()=default;
    virtual void Begin(std::uint32_t)=0;
    virtual void Stop(std::uint32_t)=0;
    virtual void Tick(std::uint32_t)=0;
    virtual void Tick(std::uint32_t task,float seconds){(void)seconds;Tick(task);}
    virtual bool Complete(std::uint32_t)=0;
    virtual void Command(std::uint32_t,std::string_view)=0;
    virtual void Effect(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)=0;
};
// Actual message/physical/Magic/direct-Property children. Borrow current-model lookup,
// playback, actor, effects/router and the live game frame-scale field.
// Containing GOM lists must retire before this owner or its dependencies.
class NativeCombatMessageChildren final {
public:
    using Model=std::function<NativeActorInstanceOwner&()>;
    NativeCombatMessageChildren(NativeCombatActorSkillRuntime&,ICombatActionTaskRuntime&,
        Model,NativeEffectSceneSession&,NativeScriptEventRouter&,float& frame_scale);
    NativeCombatMessageChildren(NativeCombatActorSkillRuntime&,ICombatActionTaskRuntime&,
        Model,NativeEffectSceneSession&,NativeScriptEventRouter&,float&,NativeBaseObjectMemory&);
    ~NativeCombatMessageChildren();
    NativeCombatMessageChildren(const NativeCombatMessageChildren&)=delete;
    NativeCombatMessageChildren& operator=(const NativeCombatMessageChildren&)=delete;
    std::uint32_t Create(std::string_view action);
    // Required processor handles the actual embedded76-byte NormalAttackTask.
    // It borrows battle damage/reaction services; no attack is synthesized here.
    using AttackProcessor=std::function<void(std::uint32_t embedded_task)>;
    std::uint32_t CreatePhysical(std::string_view action,std::uint32_t target,
        std::uint8_t flag,std::uint8_t mode,AttackProcessor);
    std::uint32_t CreateMagic(std::string_view,std::uint32_t target,std::uint32_t record,INativeCombatMessageChildServices&);
    std::uint32_t CreateStunt(std::string_view,std::uint32_t target,std::uint32_t record,INativeCombatMessageChildServices&);
    std::uint32_t CreateProperty(std::string_view,std::uint32_t target,std::uint32_t record,INativeCombatMessageChildServices&);
    std::uint32_t CreateThrowProperty(std::string_view,std::uint32_t target,std::uint32_t record,INativeCombatMessageChildServices&);
    // Borrowed Magic service may invoke these while its validated child is dispatching.
    void BeginMessage(std::uint32_t);
    void StopMessage(std::uint32_t);
    bool MessageComplete(std::uint32_t);
    void MessageCommand(std::uint32_t,std::string_view);
    // Raw cbCast/cbAttachCast values retain the original task address. The
    // current child also records which effects were bound during this run,
    // so a retired effect cannot match a later allocation at that address.
    void BindEffectCallback(std::uint32_t,const EffectSystemHandleWords&,std::string_view);
    bool OwnsEffectCallback(std::uint32_t,const EffectSystemHandleWords&,std::string_view);
    void Destroy(std::uint32_t);
    void Close();
    void AssignAction(std::uint32_t,std::string_view);
    bool Owns(std::uint32_t) const noexcept;
    void ValidateIdle() const;
    IGomTaskBaseRuntime& BaseRuntime() noexcept;
    IGomTaskContainerRuntime& ContainerRuntime() noexcept;
    std::size_t Count() const noexcept;
    std::size_t SubscriptionCount() const noexcept;
    // Excludes a borrowed domain; its containing owner accounts for that memory.
    std::size_t OwnedAllocations() const noexcept;
    std::size_t OwnedTexts() const noexcept;
    std::size_t Commands() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
