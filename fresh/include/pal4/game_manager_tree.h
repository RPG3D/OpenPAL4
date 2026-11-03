#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace pal4 {

class IGameManagerTreeMemory {
public:
    virtual ~IGameManagerTreeMemory() = default;
    [[nodiscard]] virtual std::uint32_t AllocateNode() = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) const = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) const = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
};

struct GameManagerTreeInsertResult {
    std::uint32_t node = 0;
    bool inserted = false;
};

class GameManagerTreeInsert final {
public:
    static constexpr std::uint32_t kNilNode = 0x8e4110u;

    [[nodiscard]] static GameManagerTreeInsertResult Insert(
        std::uint32_t tree, std::int32_t key, std::uint32_t value,
        IGameManagerTreeMemory& memory);
    [[nodiscard]] static std::uint32_t InsertAt(
        std::uint32_t tree, std::uint32_t child, std::uint32_t parent,
        std::int32_t key, std::uint32_t value,
        IGameManagerTreeMemory& memory);
};

class GameManagerTreeRemove final {
public:
    // Returns the in-order successor, or the header node at end.
    [[nodiscard]] static std::uint32_t Remove(
        std::uint32_t tree, std::uint32_t node,
        IGameManagerTreeMemory& memory);
};

class GameManagerTreeCursor final {
public:
    [[nodiscard]] static std::uint32_t Next(
        std::uint32_t node, const IGameManagerTreeMemory& memory);
    // Mutates cursor. Return value reproduces EAX, which may differ from cursor.
    [[nodiscard]] static std::uint32_t Previous(
        std::uint32_t& cursor, const IGameManagerTreeMemory& memory);
};

class GameManagerTreeLookup final {
public:
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::int32_t key,
        const IGameManagerTreeMemory& memory);
};

class GameManagerTreeRotation final {
public:
    [[nodiscard]] static std::uint32_t Left(
        std::uint32_t tree, std::uint32_t node,
        IGameManagerTreeMemory& memory);
    [[nodiscard]] static std::uint32_t Right(
        std::uint32_t tree, std::uint32_t node,
        IGameManagerTreeMemory& memory);
};

// Address-backed stand-in for the original allocator and 32-bit process memory.
class GameManagerTreeMemoryStub final : public IGameManagerTreeMemory {
public:
    static constexpr std::uint32_t kTree = 0x1000u;
    static constexpr std::uint32_t kHeader = 0x1100u;

    explicit GameManagerTreeMemoryStub(bool allow_duplicates = false);
    [[nodiscard]] std::uint32_t AllocateNode() override;
    void FreeNode(std::uint32_t node) override;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) const override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) const override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] const std::vector<std::uint32_t>& FreedNodes() const {
        return freed_nodes_;
    }

private:
    std::unordered_map<std::uint32_t, std::uint32_t> words_;
    std::uint32_t next_node_ = 0x2000u;
    std::vector<std::uint32_t> freed_nodes_;
};

}  // namespace pal4
