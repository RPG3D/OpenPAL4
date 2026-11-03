#include "pal4/effect_script_vm.h"

namespace pal4 {

std::uint32_t EffectScriptVm::ExecuteFile(
    const std::string_view path, IEffectScriptVmRuntime& runtime) {
    (void)Release(runtime);
    const auto device = runtime.CurrentDevice();
    const auto file = runtime.OpenFile(path, "r");
    if (file == 0u) return 0u;

    const auto bytes = runtime.ScriptSize(file);
    const auto buffer = runtime.Allocate(bytes);
    runtime.ReadFile(buffer, bytes, 1u, file);
    runtime.CloseFile(file);
    std::uint32_t error = 0u;
    const auto result = runtime.LoadAndExecute(
        device, buffer, bytes, 0u, 0u, 260u, 0u, vm_, error);
    if (result != 0u && error != 0u)
        runtime.ShowError(runtime.ErrorText(error), "error", 0u);
    runtime.Free(buffer);
    return result == 0u ? 1u : 0u;
}

std::uint32_t EffectScriptVm::Release(IEffectScriptVmRuntime& runtime) {
    if (vm_ != 0u) {
        runtime.ReleaseVm(vm_);
        vm_ = 0u;
    }
    return 1u;
}

std::uint32_t EffectScriptVm::GetValue284(
    IEffectScriptVmRuntime& runtime) const {
    return vm_ == 0u ? 0u : runtime.InvokeVmSlot(vm_, 284u);
}

std::uint32_t EffectScriptVm::GetValue288(
    IEffectScriptVmRuntime& runtime) const {
    return vm_ == 0u ? 0u : runtime.InvokeVmSlot(vm_, 288u);
}

}  // namespace pal4
