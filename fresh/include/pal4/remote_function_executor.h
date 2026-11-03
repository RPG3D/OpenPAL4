#pragma once

#include <cstdint>

namespace pal4 {

class IRemoteFunctionRuntime {
public:
    virtual ~IRemoteFunctionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t LoadDebugFunctions() = 0;
    [[nodiscard]] virtual std::uint32_t OpenProcess(
        std::uint32_t access, std::uint32_t inherit,
        std::uint32_t process_id) = 0;
    [[nodiscard]] virtual std::uint32_t IsBadCodePointer() = 0;
    [[nodiscard]] virtual std::uint32_t CallDebugFunction(
        std::uint32_t process, std::uint32_t modules,
        std::uint32_t bytes, std::uint32_t& bytes_written) = 0;
    virtual void CloseHandle(std::uint32_t handle) = 0;
    virtual void WriteOutput(std::uint32_t address, std::uint32_t count) = 0;
    virtual void SetLastErrorEx(std::uint32_t error, std::uint32_t type) = 0;
    virtual void SetLastError(std::uint32_t error) = 0;
};

class RemoteFunctionExecutor final {
public:
    [[nodiscard]] static std::uint32_t Execute(
        std::uint32_t process_id, std::uint32_t count,
        std::uint32_t modules, std::uint32_t output,
        IRemoteFunctionRuntime& runtime);
};

}  // namespace pal4
