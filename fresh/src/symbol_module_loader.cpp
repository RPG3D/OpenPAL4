#include "pal4/symbol_module_loader.h"

#include <array>

namespace pal4 {

std::uint32_t SymbolModuleLoader::Initialize(
    const std::uint32_t process_id, const std::uint32_t process,
    const std::uint32_t search_path, const std::uint32_t invade_process,
    ISymbolModuleRuntime& runtime) {
    const std::uint32_t is_nt = runtime.CheckOperatingSystemPlatform();
    if (is_nt == 1u || invade_process == 0u) {
        return runtime.SymInitialize(process, search_path, invade_process);
    }

    const std::uint32_t initialized = runtime.SymInitialize(
        process, search_path, invade_process);
    if (initialized == 0u) {
        return 0u;
    }

    std::uint32_t count = 0u;
    if (runtime.EnumerateProcessModules(process_id, 0u, 0u, count) == 0u) {
        runtime.SymCleanup(process);
        return 0u;
    }

    const std::uint32_t buffer = runtime.Allocate(count * 4u);
    if (runtime.EnumerateProcessModules(process_id, count, buffer, count) == 0u) {
        runtime.SymCleanup(process);
        runtime.Free(buffer);
        return 0u;
    }

    for (std::uint32_t index = 0u; index < count; ++index) {
        const std::uint32_t module = runtime.ReadModule(buffer, index);
        std::array<char, 260> filename{};
        if (runtime.GetModuleFileNameA(module, filename.data(), 0x104u) == 0u) {
            runtime.SymCleanup(process);
            runtime.Free(buffer);
            return 0u;
        }
        const std::uint32_t file = runtime.CreateFileA(
            filename.data(), 0x80000000u, 1u, 0u, 3u, 0u, 0u);
        if (runtime.SymLoadModule(process, file, filename.data(), 0u,
                                  module, 0u) == 0u &&
            runtime.GetLastError() != 0u) {
            runtime.SymCleanup(process);
            runtime.Free(buffer);
            return 0u;
        }
    }
    runtime.Free(buffer);
    return 1u;
}

}  // namespace pal4
