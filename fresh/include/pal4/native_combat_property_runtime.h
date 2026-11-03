#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <functional>
#include "pal4/actor_render_state.h"
namespace pal4 {
class NativeCombatSkillActorRegistry;class NativeInventorySession;class NativeCombatRosterTargets;class NativeScriptEventRouter;
class IPropertyActionTaskRuntime;
class IThrowPropertyActionRuntime;class IThrowPropertyTaskRuntime;class NativeEffectSceneSession;
struct NativeCombatPropertyTargetServices {
    std::function<ActorEffectHandle(std::uint32_t,std::string_view,std::string_view)> effect;
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> wounded;
};
// Original embedded80-byte UseProperty execution. Borrows all actors/records
// and the actual inventory event route; enclosing animation owns the task image.
class NativeCombatPropertyRuntime final {
public:
    NativeCombatPropertyRuntime(NativeCombatSkillActorRegistry&,NativeInventorySession&,NativeCombatRosterTargets&,NativeScriptEventRouter&);
    ~NativeCombatPropertyRuntime();
    // Shared read-only inventory-record admission for parent and embedded task.
    // Original55D0B0 does not restrict the attached Skill's FUNCTION family.
    // Actual skill dispatch remains owned by NativeCombatActorSkillRuntime.
    void ValidateDefinition(std::uint32_t record) const;
    void BindTargetEffects(NativeEffectSceneSession&,NativeCombatPropertyTargetServices);
    void Process(std::span<std::uint32_t,20> task);
    bool Complete(std::span<std::uint32_t,20> task);
    void ProcessDirectCommand(std::span<std::uint32_t,81> child,std::string_view,IPropertyActionTaskRuntime&);
    void UpdateThrow(std::span<std::uint32_t,103> child,float,IThrowPropertyActionRuntime&,IThrowPropertyTaskRuntime&);
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
