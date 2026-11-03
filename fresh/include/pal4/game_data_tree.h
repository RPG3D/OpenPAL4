#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataTreeRuntime {
public:
    virtual ~IGameDataTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void SetSharedNil(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNilUsers() = 0;
    virtual void SetSharedNilUsers(std::uint32_t users) = 0;
    virtual void GetFirstElement(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t FreeMemory(
        std::uint32_t address) = 0;
};

class GameDataTree final {
public:
    // IDA 0x4c5710. Releases right subtrees before each node and returns nil.
    [[nodiscard]] static std::uint32_t FreeNodes(
        std::uint32_t root, IGameDataTreeRuntime& runtime);
    // IDA 0x4c5750. Returns the newly allocated tree head.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t tree, IGameDataTreeRuntime& runtime);
    // IDA 0x4c4ba0. Returns the observed final EAX value.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t tree, IGameDataTreeRuntime& runtime);
    // IDA 0x4c50c0. Writes node and inserted byte to output.
    [[nodiscard]] static std::uint32_t InsertOrGet(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t key, IGameDataTreeRuntime& runtime);
    // IDA 0x4c57d0. Direct node insertion writes the new node to output.
    [[nodiscard]] static std::uint32_t InsertNode(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t child, std::uint32_t parent,
        std::uint32_t key, IGameDataTreeRuntime& runtime);
    // IDA 0x4c52e0. Mutates cursor; the raw return may differ from it.
    [[nodiscard]] static std::uint32_t NextCursor(
        std::uint32_t& cursor, IGameDataTreeRuntime& runtime);
    // IDA 0x4c5a40. Returns the old cursor in output, then advances it.
    [[nodiscard]] static std::uint32_t PostIncrement(
        std::uint32_t& cursor, std::uint32_t output,
        IGameDataTreeRuntime& runtime);
    // IDA 0x4c5b60. Mutates cursor and preserves the observed EAX result.
    [[nodiscard]] static std::uint32_t PreviousCursor(
        std::uint32_t& cursor, IGameDataTreeRuntime& runtime);
    // IDA 0x4c5aa0 / 0x4c5b00. Return the promoted child.
    static std::uint32_t RotateLeft(
        std::uint32_t tree, std::uint32_t node,
        IGameDataTreeRuntime& runtime);
    static std::uint32_t RotateRight(
        std::uint32_t tree, std::uint32_t node,
        IGameDataTreeRuntime& runtime);
    // IDA 0x4c5330. Writes the successor to output and frees node.
    [[nodiscard]] static std::uint32_t DeleteNode(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t node, IGameDataTreeRuntime& runtime);

private:
    static void DeleteCore(std::uint32_t tree, std::uint32_t node,
                           IGameDataTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertCore(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t key,
        IGameDataTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Previous(
        std::uint32_t node, IGameDataTreeRuntime& runtime);
};

}  // namespace pal4
