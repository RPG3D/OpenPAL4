#include "pal4/process_module_enumerator.h"

namespace pal4 {

std::uint32_t ProcessModuleEnumerator::Enumerate(
    const std::uint32_t process_id, const std::uint32_t capacity,
    const std::uint32_t modules_address, std::uint32_t& module_count,
    IProcessModuleRuntime& runtime) {
    module_count = 0u;
    if (runtime.InitializeToolHelp32() == 0u) {
        runtime.SetLastErrorEx(0x45au, 1u);
        return 0u;
    }

    const std::uint32_t snapshot = runtime.CreateSnapshot(8u, process_id);
    if (snapshot == 0xffffffffu) {
        return 0u;
    }

    ModuleEntry32 entry{};
    entry.size = sizeof(entry);
    if (runtime.Module32First(snapshot, entry) != 1u) {
        runtime.SetLastErrorEx(runtime.GetLastError(), 1u);
        return 0u;
    }

    bool capacity_reached = false;
    do {
        if (capacity != 0u) {
            if (module_count == capacity) {
                capacity_reached = true;
                break;
            }
            runtime.WriteModule(modules_address + module_count * 4u,
                                entry.base_address);
        }
        ++module_count;
    } while (runtime.Module32Next(snapshot, entry) == 1u);

    runtime.CloseHandle(snapshot);
    if (capacity_reached) {
        runtime.SetLastErrorEx(0x7au, 1u);
        return 0u;
    }
    runtime.SetLastError(0u);
    return 1u;
}

}  // namespace pal4
