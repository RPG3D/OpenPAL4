#include "pal4/debug_function_loader.h"

namespace pal4 {

std::uint32_t DebugFunctionLoader::Load(IDebugFunctionRuntime& runtime) {
    if (state_.initialized == 1u) {
        return 1u;
    }
    const std::uint32_t module = runtime.LoadLibrary("PSAPI.DLL");
    if (module == 0u) {
        return 0u;
    }
    state_.enum_process_modules = runtime.GetProcAddress(
        module, "EnumProcessModules");
    if (state_.enum_process_modules == 0u) {
        return 0u;
    }
    state_.get_module_base_name = runtime.GetProcAddress(
        module, "GetModuleBaseNameA");
    if (state_.get_module_base_name == 0u) {
        return 0u;
    }
    state_.get_module_file_name = runtime.GetProcAddress(
        module, "GetModuleFileNameExA");
    if (state_.get_module_file_name == 0u) {
        return 0u;
    }
    state_.initialized = 1u;
    return 1u;
}

}  // namespace pal4
