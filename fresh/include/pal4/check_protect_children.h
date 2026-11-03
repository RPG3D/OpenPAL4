#pragma once

#include <cstdint>

namespace pal4 {

class ICheckProtectChildRuntime {
public:
    virtual ~ICheckProtectChildRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class CheckProtectPositionTask final {
public:
    static constexpr std::uint32_t kSize = 76u;
    static constexpr std::uint32_t kVtable = 0x8459c4u;

    // IDA 0x565620: allocate and construct the position child.
    [[nodiscard]] static std::uint32_t Create(
        ICheckProtectChildRuntime& runtime);
};

class CheckProtectResponseTask final {
public:
    static constexpr std::uint32_t kSize = 72u;
    static constexpr std::uint32_t kVtable = 0x8459e0u;

    // IDA 0x565690: allocate and construct the response child.
    [[nodiscard]] static std::uint32_t Create(
        ICheckProtectChildRuntime& runtime);
};

}  // namespace pal4
