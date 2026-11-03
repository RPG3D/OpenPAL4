#pragma once

#include "pal4/matfx_script_vm.h"

#include <cstdint>

namespace pal4 {

class WorldManagerScriptThunks final {
public:
    [[nodiscard]] static std::uint32_t GetState(
        const MatFxScriptVm& vm, IEffectScriptVmRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetProperty288(
        const MatFxScriptVm& vm, IEffectScriptVmRuntime& runtime);
};

}  // namespace pal4
