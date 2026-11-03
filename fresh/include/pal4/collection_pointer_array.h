#pragma once

#include "pal4/dword_collection_span.h"

#include <cstdint>

namespace pal4 {

class ICollectionPointerArrayRuntime {
public:
    virtual ~ICollectionPointerArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

class CollectionPointerArrayRuntimeStub final
    : public ICollectionPointerArrayRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t AllocateMemory(std::uint32_t bytes) override;
    void FreeMemory(std::uint32_t address) override;
};

class CollectionPointerArray final {
public:
    // IDA 0x5ee0f0: the three pointer fields occupy owner +4/+8/+12.
    [[nodiscard]] static std::uint32_t PushBack(
        DwordCollectionSpanState& collection, std::uint32_t value,
        ICollectionPointerArrayRuntime& runtime);
};

}  // namespace pal4
