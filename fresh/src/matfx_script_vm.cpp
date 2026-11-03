#include "pal4/matfx_script_vm.h"

namespace pal4 {

std::uint32_t MatFxScriptVm::ExecuteFile(
    const std::string_view path, IEffectScriptVmRuntime& runtime) {
    return vm_.ExecuteFile(path, runtime);
}

std::uint32_t MatFxScriptVm::Release(IEffectScriptVmRuntime& runtime) {
    return vm_.Release(runtime);
}

std::uint32_t MatFxScriptVm::GetState(
    IEffectScriptVmRuntime& runtime) const {
    return vm_.GetValue284(runtime);
}

std::uint32_t MatFxScriptVm::GetProperty288(
    IEffectScriptVmRuntime& runtime) const {
    return vm_.GetValue288(runtime);
}

}  // namespace pal4
