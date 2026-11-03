#pragma once
#include "pal4/actor_render_state.h"
#include <functional>
#include <string_view>
namespace pal4 {
// Borrowed real attachment and registered EffectHandle services. They outlive
// the actor's skill notifications and every payload in its dynamic-effect tree.
struct NativeCombatSkillEffectServices final {
    std::function<ActorEffectHandle(std::string_view,std::string_view)> create;
    std::function<bool(ActorEffectHandle)> active;
    std::function<void(ActorEffectHandle)> release;
};
}
