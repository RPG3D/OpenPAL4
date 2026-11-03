#pragma once
#include "pal4/native_combat_message_children.h"
#include "pal4/actor_render_state.h"
#include <functional>
#include <array>
namespace pal4 {
class NativeUiComponentResource;class NativePkFileManagerRuntime;
class NativeCombatSkillActorRegistry;class NativeCombatPropertyRuntime;
class NativeEffectSceneSession;class NativeScriptEventRouter;
struct NativeCombatThrowPropertyServices {
    std::function<std::unique_ptr<NativeUiComponentResource>()> component;
    std::function<ActorEffectHandle(std::uint32_t,std::string_view,std::string_view)> effect;
    std::function<std::array<float,3>(std::uint32_t)> position;
    std::function<float(std::uint32_t)> height;
    NativePkFileManagerRuntime* packages{};
};
// Original412-byte ThrowPropertyAction and embedded88-byte projectile.
// Borrows actual actor/CPK services; owns projectile components until Stop.
// Original55DE70 uses draw mode0 and never attaches this component to a world.
class NativeCombatThrowProperty final:public INativeCombatMessageChildServices {
public:
    NativeCombatThrowProperty(std::uint32_t,NativeCombatSkillActorRegistry&,NativeCombatMessageChildren&,
        NativeCombatPropertyRuntime&,NativeEffectSceneSession&,NativeScriptEventRouter&,
        std::function<void(std::uint32_t,std::uint32_t)>,NativeCombatThrowPropertyServices);
    ~NativeCombatThrowProperty();
    void Begin(std::uint32_t)override;void Stop(std::uint32_t)override;
    void Tick(std::uint32_t)override;void Tick(std::uint32_t,float)override;
    bool Complete(std::uint32_t)override;
    void Command(std::uint32_t,std::string_view)override;
    void Effect(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)override;
    std::size_t Subscriptions()const noexcept;
    std::size_t Components()const noexcept;
    std::size_t HiddenComponentsOutsideWorld()const noexcept;
    std::size_t Throws()const noexcept;
    std::size_t PositionUpdates()const noexcept;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
