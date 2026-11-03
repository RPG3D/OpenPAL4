#pragma once

#include <cstdint>

namespace pal4 {

class IUiRedBlackTreeIteratorRuntime {
public:
    virtual ~IUiRedBlackTreeIteratorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
};

class UiRedBlackTreeIterator final {
public:
    // IDA 0x4c0770/0x4c1650/0x4c21c0. Shared nil selects the tree variant.
    [[nodiscard]] static std::uint32_t Next(
        std::uint32_t cursor,
        IUiRedBlackTreeIteratorRuntime& runtime);

    // IDA 0x4c0fc0. Advances cursor and writes its former node to output.
    [[nodiscard]] static std::uint32_t AdvanceAndReturnPrevious(
        std::uint32_t cursor, std::uint32_t output,
        IUiRedBlackTreeIteratorRuntime& runtime);

    // IDA 0x4c15f0. Moves toward the predecessor; return is EAX residue.
    [[nodiscard]] static std::uint32_t Previous(
        std::uint32_t cursor,
        IUiRedBlackTreeIteratorRuntime& runtime);

    // IDA 0x4c1bb0. Factory-tree nodes store colour at +0x14.
    [[nodiscard]] static std::uint32_t PreviousFactory(
        std::uint32_t cursor,
        IUiRedBlackTreeIteratorRuntime& runtime);
};

}  // namespace pal4
