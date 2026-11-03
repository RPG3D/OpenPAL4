#pragma once
#include "pal4/native_combat_fatal_children.h"
#include "pal4/native_combat_message_children.h"
#include <array>
namespace pal4 {
class NativeCombatActionChildren;
// Mixed FendOff impact/message/prepare children in one real GOM. This owner
// owns the common object/name/list domain; containing GOM lists retire first.
class NativeCombatReactionChildren final {
public:
    NativeCombatReactionChildren(ICombatStatusActorRuntime&,NativeCombatActorSkillRuntime&,
        ICombatActionTaskRuntime&,NativeCombatFatalChildren::PrepareAction,
        NativeCombatMessageChildren::Model,NativeEffectSceneSession&,NativeScriptEventRouter&,float&);
    ~NativeCombatReactionChildren();
    NativeCombatReactionChildren(const NativeCombatReactionChildren&)=delete;
    NativeCombatReactionChildren& operator=(const NativeCombatReactionChildren&)=delete;
    std::uint32_t Create(std::uint32_t factory);
    void AssignAction(std::uint32_t,std::string_view);
    // Borrow original position children for the shared WinAction GOM.
    void BindPosition(NativeCombatActionChildren&);
    void ConfigurePosition(std::uint32_t,std::uint32_t,const std::array<float,3>&);
    void Destroy(std::uint32_t);
    void Close();
    void ValidateIdle() const;
    IGomTaskBaseRuntime& BaseRuntime() noexcept;
    IGomTaskContainerRuntime& ContainerRuntime() noexcept;
    std::size_t Count() const noexcept;
    std::size_t OwnedAllocations() const noexcept;
    std::size_t OwnedTexts() const noexcept;
    std::size_t Subscriptions() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
