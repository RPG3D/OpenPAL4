#pragma once
namespace pal4 {
class ICombatActionTaskRuntime;
class IDeadEffectTaskRuntime;
class IFadeOffTaskRuntime;
// Optional borrowed services, all required for an active Monster Dead recipe.
struct NativeCombatFatalVisualServices {
    ICombatActionTaskRuntime* action{};
    IDeadEffectTaskRuntime* effect{};
    IFadeOffTaskRuntime* fade{};
};
}
