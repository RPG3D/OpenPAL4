#pragma once

#include <cstdint>

namespace pal4 {

class IEffectSystemObjectListRuntime {
public:
    virtual ~IEffectSystemObjectListRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t pointer) = 0;
};

class EffectSystemObjectList final {
public:
    // IDA 0x633740: insert an object node immediately before an iterator.
    [[nodiscard]] static std::uint32_t InsertBefore(
        std::uint32_t list, std::uint32_t before,
        std::uint32_t object, IEffectSystemObjectListRuntime& runtime);
    // IDA 0x633790: unlink and free the half-open range [first, last).
    [[nodiscard]] static std::uint32_t EraseRange(
        std::uint32_t list, std::uint32_t first,
        std::uint32_t last, IEffectSystemObjectListRuntime& runtime);
    // IDA 0x6337e0: allocate a 12-byte node and initialize its links.
    [[nodiscard]] static std::uint32_t CreateNode(
        std::uint32_t next, std::uint32_t previous,
        IEffectSystemObjectListRuntime& runtime);
};

}  // namespace pal4
