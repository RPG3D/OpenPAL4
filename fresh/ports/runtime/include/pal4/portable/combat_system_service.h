#pragma once

#include "pal4/combat_system_session.h"
#include "pal4/combat_system_start.h"

#include <cstdint>

namespace pal4::portable {

// Combines the original cold-session initializer with the original 5756D0
// start algorithm. The host owns all platform resources and supplies the two
// typed service surfaces; this adapter only connects their lifecycle.
class CombatSystemService final {
public:
    CombatSystemService(std::uint32_t logical_system_owner,
                        ICombatSystemColdRuntime& cold_runtime,
                        ICombatSystemStartServices& start_services);

    CombatSystemService(const CombatSystemService&) = delete;
    CombatSystemService& operator=(const CombatSystemService&) = delete;

    // True means the original ready tail committed input, state 1, prepare,
    // and audio reset. False is the original Start rejection, never a win.
    [[nodiscard]] bool Start(std::uint32_t config, std::uint32_t mode);

private:
    class StartServices;

    std::uint32_t owner_;
    ICombatSystemColdRuntime& cold_runtime_;
    ICombatSystemStartServices& start_services_;
    bool starting_{};
};

} // namespace pal4::portable
