#pragma once

#include <cstdint>

namespace pal4 {

class IUiRedBlackTreeLookupRuntime {
public:
    virtual ~IUiRedBlackTreeLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::int32_t CompareRange(
        std::uint32_t node_text, std::uint32_t node_length,
        std::uint32_t query_text, std::uint32_t query_length) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class UiRedBlackTreeLookup final {
public:
    // IDA 0x4c0c00. Returns the first node >= query, or the head.
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::uint32_t output, std::uint32_t query,
        IUiRedBlackTreeLookupRuntime& runtime);

    // IDA 0x4c0fa0: despite its name, follows left links to the minimum.
    [[nodiscard]] static std::uint32_t LeftmostFromSlot(
        std::uint32_t slot, IUiRedBlackTreeLookupRuntime& runtime);
};

}  // namespace pal4
