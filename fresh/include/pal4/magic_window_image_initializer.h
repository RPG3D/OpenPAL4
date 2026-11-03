#pragma once

#include "pal4/magic_window_image_setup.h"
#include "pal4/magic_window_numeric_display.h"
#include "pal4/magic_window_skill_rows.h"

#include <cstddef>
#include <cstdint>
#include <span>

namespace pal4 {

class MagicWindowImageInitializer final {
public:
    static std::size_t Initialize(
        std::uint32_t object, const MagicWindowState& state,
        const MagicWindowPanelState& panel,
        std::span<const MagicDbSkillRecord> records,
        IMagicWindowImageSetupRuntime& image_runtime,
        IMagicWindowSkillRowsRuntime& rows_runtime,
        IMagicWindowNumericRuntime& numeric_runtime);
};

}  // namespace pal4
