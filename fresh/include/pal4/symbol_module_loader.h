#pragma once

#include <cstdint>

namespace pal4 {

class ISymbolModuleRuntime {
public:
    virtual ~ISymbolModuleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CheckOperatingSystemPlatform() = 0;
    [[nodiscard]] virtual std::uint32_t SymInitialize(
        std::uint32_t process, std::uint32_t search_path,
        std::uint32_t invade_process) = 0;
    [[nodiscard]] virtual std::uint32_t EnumerateProcessModules(
        std::uint32_t process_id, std::uint32_t capacity,
        std::uint32_t buffer, std::uint32_t& count) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadModule(
        std::uint32_t buffer, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t GetModuleFileNameA(
        std::uint32_t module, char* filename, std::uint32_t capacity) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFileA(
        const char* filename, std::uint32_t access,
        std::uint32_t share, std::uint32_t security,
        std::uint32_t creation, std::uint32_t flags,
        std::uint32_t template_file) = 0;
    [[nodiscard]] virtual std::uint32_t SymLoadModule(
        std::uint32_t process, std::uint32_t file,
        const char* filename, std::uint32_t module_name,
        std::uint32_t base, std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t GetLastError() = 0;
    virtual void SymCleanup(std::uint32_t process) = 0;
};

class SymbolModuleLoader final {
public:
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t process_id, std::uint32_t process,
        std::uint32_t search_path, std::uint32_t invade_process,
        ISymbolModuleRuntime& runtime);
};

}  // namespace pal4
