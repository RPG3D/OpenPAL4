#pragma once

#include "pal4/combat_console_image_events.h"

namespace pal4::detail {

// Owns an event's allocation until its actor queue accepts the activity.
// ConstructMotion unwinds its own text constructor on failure. After successful
// construction, overlay destruction must precede freeing the object itself.
// As with queue removal, runtime destruction/deallocation must not throw.
class CombatConsoleActivityOwner final {
public:
    CombatConsoleActivityOwner(std::uint32_t allocation,
                              ICombatConsoleVisualRuntime& runtime)
        : allocation_(allocation), runtime_(runtime) {}
    CombatConsoleActivityOwner(const CombatConsoleActivityOwner&) = delete;
    CombatConsoleActivityOwner& operator=(const CombatConsoleActivityOwner&) = delete;
    ~CombatConsoleActivityOwner() {
        if (!allocation_) return;
        if (constructed_) (void)UiTextImageOverlay::Destroy(allocation_, runtime_);
        (void)runtime_.Free(allocation_);
    }
    void Constructed() noexcept { constructed_ = true; }
    void Release() noexcept { allocation_ = 0u; }
private:
    std::uint32_t allocation_;
    ICombatConsoleVisualRuntime& runtime_;
    bool constructed_{};
};

}  // namespace pal4::detail
