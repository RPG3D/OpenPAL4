#include "pal4/toolhelp_loader.h"

namespace pal4 {

std::uint32_t ToolHelpLoader::Initialize(IToolHelpRuntime& runtime) {
    if (state_.initialized == 1u) {
        return 1u;
    }
    const std::uint32_t kernel32 = runtime.GetModuleHandle("KERNEL32.DLL");
    if (kernel32 == 0u) {
        runtime.SetLastErrorEx(0x45au, 1u);
        return 0u;
    }

    static constexpr std::array<const char*, 7> names{
        "CreateToolhelp32Snapshot", "Module32First", "Module32Next",
        "Process32First", "Process32Next", "Thread32First", "Thread32Next"};
    for (std::size_t index = 0u; index < names.size(); ++index) {
        state_.functions[index] = runtime.GetProcAddress(kernel32, names[index]);
    }
    for (const std::uint32_t function : state_.functions) {
        if (function == 0u) {
            return 0u;
        }
    }
    state_.initialized = 1u;
    return 1u;
}

}  // namespace pal4
