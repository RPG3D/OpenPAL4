#pragma once

#include <cstdint>

namespace pal4 {

class IUiFrameManagerCursorTreeRuntime {
public:
    virtual ~IUiFrameManagerCursorTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::int32_t CompareRange(
        std::uint32_t key, std::uint32_t text,
        std::uint32_t length) = 0;
    [[nodiscard]] virtual std::int32_t CompareText(
        std::uint32_t first, std::uint32_t second) = 0;
    [[nodiscard]] virtual std::int32_t CompareBytes(
        std::uint32_t first, std::uint32_t second,
        std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateNode(
        std::uint32_t tree, std::uint32_t parent,
        std::uint32_t flag) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t Predecessor(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t InsertNode(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t key) = 0;
    virtual void SetExistingResult(std::uint32_t output,
                                   std::uint32_t node) = 0;
    virtual void RotateLeft(std::uint32_t tree,
                            std::uint32_t node) = 0;
    virtual void RotateRight(std::uint32_t tree,
                             std::uint32_t node) = 0;
};

class UiFrameManagerCursorTree final {
public:
    // IDA 0x4c0460. Output is a node pointer plus inserted byte.
    [[nodiscard]] static std::uint32_t InsertOrFind(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t key,
        IUiFrameManagerCursorTreeRuntime& runtime);

    // IDA 0x4c06c0. Missing keys produce the tree head.
    [[nodiscard]] static std::uint32_t Find(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t key,
        IUiFrameManagerCursorTreeRuntime& runtime);
};

}  // namespace pal4
