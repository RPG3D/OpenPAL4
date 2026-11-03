#pragma once

#include <cstdint>

namespace pal4 {

struct MonsterManagerTreeInsertResult {
    std::uint32_t node{};
    bool inserted{};
};

class IMonsterManagerTreeInsertRuntime {
public:
    virtual ~IMonsterManagerTreeInsertRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SharedNil() const = 0;
    [[nodiscard]] virtual std::uint32_t NodeSize() const { return 0x24u; }
    [[nodiscard]] virtual std::uint32_t ColorOffset() const { return 0x20u; }
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) const = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) const = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    // CompareRange compares input against node. DialogTextCheck compares its
    // first argument against its second; the two call sites reverse order.
    [[nodiscard]] virtual std::int32_t CompareRange(
        std::uint32_t input, std::uint32_t node_text) = 0;
    [[nodiscard]] virtual std::int32_t CompareText(
        std::uint32_t first, std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void SetNodeData(MonsterManagerTreeInsertResult& result,
                             std::uint32_t candidate) = 0;
};

// IDA 0x4382c0. All addresses are original 32-bit process addresses.
class MonsterManagerTreeInsert final {
public:
    [[nodiscard]] static MonsterManagerTreeInsertResult Insert(
        std::uint32_t tree, std::uint32_t input,
        IMonsterManagerTreeInsertRuntime& runtime);
    // IDA 0x438ae0: the unique-key path allocates a raw 0x24-byte node.
    [[nodiscard]] static std::uint32_t InsertHelper(
        std::uint32_t tree, std::uint32_t child, std::uint32_t parent,
        std::uint32_t input, IMonsterManagerTreeInsertRuntime& runtime);
};

}  // namespace pal4
