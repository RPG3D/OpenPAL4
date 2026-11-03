#pragma once

#include "pal4/magic_window_caster.h"
#include "pal4/magic_window_cost_processor.h"
#include "pal4/magic_window_requirements.h"

namespace pal4 {

class MagicWindowStuntCaster final {
public:
    static bool Cast(const MagicWindowState& state,
                     std::int32_t target_character_index,
                     const MagicDbStuntRecord& stunt,
                     IMagicWindowCastRuntime& cast_runtime,
                     IMagicWindowRequirementsRuntime& requirements_runtime,
                     IMagicWindowCostRuntime& cost_runtime);
};

}  // namespace pal4
