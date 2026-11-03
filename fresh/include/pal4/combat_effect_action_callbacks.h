#pragma once

#include "pal4/property_action_task.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct CombatEffectActionLayout {
    std::uint32_t owner_offset;
    std::uint32_t completion_offset;
    std::uint32_t effect_offset;
};

class CombatEffectActionCallbacks final {
public:
    [[nodiscard]] static std::uint32_t HandleEffectEvent(
        std::uint32_t task, std::uint32_t event,
        CombatEffectActionLayout layout,
        IPropertyActionEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleCompletionEvent(
        std::uint32_t task, std::uint32_t event,
        CombatEffectActionLayout layout,
        IPropertyActionEventRuntime& runtime);
    [[nodiscard]] static int HandleAnimationEvent(
        std::uint32_t task, PropertyTaskEffectHandle effect,
        std::string_view event_name, CombatEffectActionLayout layout,
        IPropertyActionEventRuntime& runtime);
};

}  // namespace pal4
