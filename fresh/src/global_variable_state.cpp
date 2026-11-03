#include "pal4/global_variable_state.h"

namespace pal4 {

std::uint32_t GlobalVariableState::Setup(
    const std::uint32_t entry_ecx,
    IGlobalVariableStateRuntime& runtime) {
    runtime.WriteByte(kAddress,
                      static_cast<std::uint8_t>(entry_ecx >> 24u));
    runtime.Write(kAddress + 4u, 0u);
    runtime.Write(kAddress + 8u, 0u);
    runtime.Write(kAddress + 12u, 0u);
    return 0u;
}

std::uint32_t GlobalVariableState::InitializeAndRegisterExit(
    const std::uint32_t entry_ecx,
    IGlobalVariableStateRuntime& runtime) {
    (void)Setup(entry_ecx, runtime);
    return runtime.RegisterExitHandler(kCleanupEntry);
}

void GlobalVariableState::Cleanup(IGlobalVariableStateRuntime& runtime) {
    runtime.Free(runtime.Read(kAddress + 4u));
    runtime.Write(kAddress + 4u, 0u);
    runtime.Write(kAddress + 8u, 0u);
    runtime.Write(kAddress + 12u, 0u);
}

}  // namespace pal4
