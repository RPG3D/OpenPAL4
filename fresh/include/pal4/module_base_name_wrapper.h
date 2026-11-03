#pragma once

#include <cstdint>

namespace pal4 {

class IModuleBaseNameRuntime {
public:
    virtual ~IModuleBaseNameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CheckOperatingSystemPlatform() = 0;
    [[nodiscard]] virtual std::uint32_t LoadDebugFunctions() = 0;
    [[nodiscard]] virtual std::uint32_t GetModuleBaseNameA(
        std::uint32_t process, std::uint32_t module,
        std::uint32_t output, std::uint32_t capacity) = 0;
    [[nodiscard]] virtual std::uint32_t IsBadWritePointer(
        std::uint32_t address, std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t GetModuleFileNameA(
        std::uint32_t module, char* filename, std::uint32_t capacity) = 0;
    virtual void CopyN(std::uint32_t output, const char* source,
                       std::uint32_t capacity) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void SetLastError(std::uint32_t error) = 0;
    virtual void SetLastErrorEx(std::uint32_t error, std::uint32_t type) = 0;
};

class ModuleBaseNameWrapper final {
public:
    [[nodiscard]] static std::uint32_t Get(
        std::uint32_t process, std::uint32_t module,
        std::uint32_t output, std::uint32_t capacity,
        IModuleBaseNameRuntime& runtime);
};

}  // namespace pal4
