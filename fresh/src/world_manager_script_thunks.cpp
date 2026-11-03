#include "pal4/world_manager_script_thunks.h"

namespace pal4 {

std::uint32_t WorldManagerScriptThunks::GetState(
    const MatFxScriptVm& vm, IEffectScriptVmRuntime& runtime) {
    return vm.GetState(runtime);
}

std::uint32_t WorldManagerScriptThunks::GetProperty288(
    const MatFxScriptVm& vm, IEffectScriptVmRuntime& runtime) {
    return vm.GetProperty288(runtime);
}

}  // namespace pal4
