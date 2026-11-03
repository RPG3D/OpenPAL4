#pragma once

#include "pal4/actor_manager_player_text_property.h"

#include <cstdint>

namespace pal4 {

class ActorManagerPlayerTextLookup final {
public:
    static constexpr std::uint32_t kGuard = 0x978ab0u;
    static constexpr std::uint32_t kFallback = 0x978ac0u;
    static constexpr std::uint32_t kCleanupFunction = 0x53a6c0u;

    [[nodiscard]] static std::uint32_t Record(
        std::uint32_t base, std::int32_t index,
        IActorManagerPlayerTextRuntime& runtime);
    static void Cleanup(IActorManagerPlayerTextRuntime& runtime);
};

}  // namespace pal4
