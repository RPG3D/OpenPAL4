#pragma once

#include <cstdint>

namespace pal4 {

class ISmithWindowWordArrayRuntime {
public:
    virtual ~ISmithWindowWordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateBytes(
        std::uint32_t size) = 0;
    virtual void FreeBytes(std::uint32_t address) = 0;
};

class SmithWindowWordArray final {
public:
    static void Clear(std::uint32_t adapter,
                      ISmithWindowWordArrayRuntime& runtime);
    static void Append(std::uint32_t adapter, std::uint32_t value,
                       ISmithWindowWordArrayRuntime& runtime);
};

}  // namespace pal4
