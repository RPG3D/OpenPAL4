#pragma once

#include "pal4/collection_iterator_lifecycle.h"

#include <cstdint>

namespace pal4 {

struct CollectionForEachVisitorState {
    std::uint32_t vtable{0x849938u};
    std::uint32_t parameter{};
    std::uint32_t value{};
};
static_assert(sizeof(CollectionForEachVisitorState) == 12u);

class ICollectionForEachRuntime : public ICollectionIteratorLifecycleRuntime {
public:
    [[nodiscard]] virtual bool AtEnd(std::uint32_t iterator,
                                     std::uint32_t vtable_offset) = 0;
    virtual void Advance(std::uint32_t iterator,
                         std::uint32_t vtable_offset) = 0;
    virtual void Apply(std::uint32_t element,
                       const CollectionForEachVisitorState& visitor,
                       std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::uint32_t Transform(
        std::uint32_t element, std::uint32_t value,
        std::uint32_t vtable_offset) = 0;
};

class CollectionForEachRuntimeStub final : public ICollectionForEachRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void Free(std::uint32_t address) override;
    bool AtEnd(std::uint32_t iterator,
               std::uint32_t vtable_offset) override;
    void Advance(std::uint32_t iterator,
                 std::uint32_t vtable_offset) override;
    void Apply(std::uint32_t element,
               const CollectionForEachVisitorState& visitor,
               std::uint32_t vtable_offset) override;
    std::uint32_t Transform(std::uint32_t element,
                            std::uint32_t value,
                            std::uint32_t vtable_offset) override;
};

class CollectionForEach final {
public:
    // IDA 0x5efac0.
    [[nodiscard]] static std::uint32_t Visit(
        std::uint32_t collection, std::uint32_t value,
        std::uint32_t parameter, ICollectionForEachRuntime& runtime);
    // IDA 0x5f00b0.
    [[nodiscard]] static std::uint32_t SetProperties(
        std::uint32_t collection, std::uint32_t value,
        std::uint32_t parameter, ICollectionForEachRuntime& runtime);
};

}  // namespace pal4
