#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ModuleEntry32 {
    std::uint32_t size = 0u;
    std::uint32_t module_id = 0u;
    std::uint32_t process_id = 0u;
    std::uint32_t global_usage = 0u;
    std::uint32_t process_usage = 0u;
    std::uint32_t base_address = 0u;
    std::uint32_t base_size = 0u;
    std::uint32_t module_handle = 0u;
    char module_name[256]{};
    char executable_path[260]{};
};

static_assert(sizeof(ModuleEntry32) == 548u);
static_assert(offsetof(ModuleEntry32, base_address) == 20u);

class IProcessModuleRuntime {
public:
    virtual ~IProcessModuleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeToolHelp32() = 0;
    [[nodiscard]] virtual std::uint32_t CreateSnapshot(
        std::uint32_t flags, std::uint32_t process_id) = 0;
    [[nodiscard]] virtual std::uint32_t Module32First(
        std::uint32_t snapshot, ModuleEntry32& entry) = 0;
    [[nodiscard]] virtual std::uint32_t Module32Next(
        std::uint32_t snapshot, ModuleEntry32& entry) = 0;
    virtual void WriteModule(std::uint32_t address,
                             std::uint32_t base_address) = 0;
    virtual void CloseHandle(std::uint32_t snapshot) = 0;
    [[nodiscard]] virtual std::uint32_t GetLastError() = 0;
    virtual void SetLastErrorEx(std::uint32_t error, std::uint32_t type) = 0;
    virtual void SetLastError(std::uint32_t error) = 0;
};

class ProcessModuleEnumerator final {
public:
    [[nodiscard]] static std::uint32_t Enumerate(
        std::uint32_t process_id, std::uint32_t capacity,
        std::uint32_t modules_address, std::uint32_t& module_count,
        IProcessModuleRuntime& runtime);
};

}  // namespace pal4
