#pragma once

#include <cstdint>

namespace pal4 {

class IUiRedBlackTreeLinkRuntime {
public:
    virtual ~IUiRedBlackTreeLinkRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
};

class UiRedBlackTreeRotation final {
public:
    // IDA 0x4c1530. Promotes the node's right child.
    [[nodiscard]] static std::uint32_t RotateLeft(
        std::uint32_t tree, std::uint32_t node,
        IUiRedBlackTreeLinkRuntime& runtime);

    // IDA 0x4c1590. Promotes the node's left child.
    [[nodiscard]] static std::uint32_t RotateRight(
        std::uint32_t tree, std::uint32_t node,
        IUiRedBlackTreeLinkRuntime& runtime);
};

}  // namespace pal4
