#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct MonsterBehaviorTreeState {
    std::array<std::uint32_t, 6> words{};
};
static_assert(sizeof(MonsterBehaviorTreeState) == 0x18u);

struct MonsterBehaviorTreeInsertResult {
    std::uint32_t node{};
    bool inserted{};
};

class IMonsterBehaviorTreeRuntime {
public:
    virtual ~IMonsterBehaviorTreeRuntime() = default;
    virtual void InitializeRedBlackTree(MonsterBehaviorTreeState& state,
                                        std::uint32_t tree_address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(std::uint32_t bytes) = 0;
    virtual void DestroySubtree(std::uint32_t tree_address,
                                std::uint32_t root_address) = 0;
    virtual void GetFirstElement(std::uint32_t tree_address) = 0;
    virtual void RemoveNode(std::uint32_t tree_address,
                            std::uint32_t node_address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void SetSharedNil(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedReferenceCount() = 0;
    virtual void SetSharedReferenceCount(std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t FreeMemory(
        std::uint32_t address) = 0;
};

class MonsterBehaviorTreeRuntimeStub final : public IMonsterBehaviorTreeRuntime {
public:
    void InitializeRedBlackTree(MonsterBehaviorTreeState& state,
                                std::uint32_t tree_address) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t AllocateMemory(std::uint32_t bytes) override;
    void DestroySubtree(std::uint32_t tree_address,
                        std::uint32_t root_address) override;
    void GetFirstElement(std::uint32_t tree_address) override;
    void RemoveNode(std::uint32_t tree_address,
                    std::uint32_t node_address) override;
    std::uint32_t SharedNil() override;
    void SetSharedNil(std::uint32_t address) override;
    std::uint32_t SharedReferenceCount() override;
    void SetSharedReferenceCount(std::uint32_t count) override;
    std::uint32_t FreeMemory(std::uint32_t address) override;

private:
    std::uint32_t nil_{};
    std::uint32_t references_{1u};
};

class MonsterBehaviorTree final {
public:
    static constexpr std::uint32_t kVtable = 0x842d64u;

    // IDA 0x4326b0: initializes the embedded tree at owner+4.
    [[nodiscard]] static std::uint32_t Initialize(
        MonsterBehaviorTreeState& state, std::uint32_t owner_address,
        std::uint8_t first, std::uint8_t second, std::uint8_t third,
        IMonsterBehaviorTreeRuntime& runtime);
    // IDA 0x4325d0: returns the final EAX value.
    [[nodiscard]] static std::uint32_t Destroy(
        MonsterBehaviorTreeState& state, std::uint32_t owner_address,
        IMonsterBehaviorTreeRuntime& runtime);
    // IDA 0x432690: the low bit requests object storage release.
    [[nodiscard]] static std::uint32_t Delete(
        MonsterBehaviorTreeState& state, std::uint32_t owner_address,
        std::uint8_t flags, IMonsterBehaviorTreeRuntime& runtime);
    // IDA 0x432ad0: output receives the successor before removal.
    [[nodiscard]] static std::uint32_t& RemoveNode(
        MonsterBehaviorTreeState& state, std::uint32_t node_address,
        std::uint32_t& output, IMonsterBehaviorTreeRuntime& runtime);
    // IDA 0x432eb0: recursively releases right branches, then walks left.
    [[nodiscard]] static std::uint32_t DestroySubtreeNodes(
        std::uint32_t root_address, IMonsterBehaviorTreeRuntime& runtime);
    // IDA 0x432ef0: initializes the embedded tree at owner+4.
    [[nodiscard]] static std::uint32_t InitializeNodes(
        MonsterBehaviorTreeState& state, IMonsterBehaviorTreeRuntime& runtime);
    // IDA 0x432f70: advances the cursor and writes its old address to output.
    [[nodiscard]] static std::uint32_t& SuccessorCursor(
        std::uint32_t& cursor, std::uint32_t& output,
        IMonsterBehaviorTreeRuntime& runtime);
    // IDA 0x432fd0: advances a cursor; the raw EAX result differs by path.
    [[nodiscard]] static std::uint32_t AdvanceSuccessor(
        std::uint32_t& cursor, IMonsterBehaviorTreeRuntime& runtime);
    // IDA 0x433020: signed classID key and factory payload.
    [[nodiscard]] static MonsterBehaviorTreeInsertResult Insert(
        MonsterBehaviorTreeState& state, std::int32_t class_id,
        std::uint32_t factory, IMonsterBehaviorTreeRuntime& runtime);
    // IDA 0x433240 and 0x4332a0: return the promoted node.
    [[nodiscard]] static std::uint32_t RotateLeft(
        MonsterBehaviorTreeState& state, std::uint32_t node,
        IMonsterBehaviorTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateRight(
        MonsterBehaviorTreeState& state, std::uint32_t node,
        IMonsterBehaviorTreeRuntime& runtime);
    // IDA 0x433300: initializes only the parent and color fields.
    [[nodiscard]] static std::uint32_t CreateNode(
        std::uint32_t parent, std::uint32_t color,
        IMonsterBehaviorTreeRuntime& runtime);
    // IDA 0x433320: result aliases the caller's output slot.
    [[nodiscard]] static std::uint32_t& InsertNodeAt(
        MonsterBehaviorTreeState& state, std::uint32_t child,
        std::uint32_t parent, std::int32_t class_id,
        std::uint32_t factory, std::uint32_t& output,
        IMonsterBehaviorTreeRuntime& runtime);
    // IDA 0x433590: in/out cursor to previous node or rightmost header child.
    [[nodiscard]] static std::uint32_t AdvancePredecessor(
        std::uint32_t& cursor, IMonsterBehaviorTreeRuntime& runtime);
};

}  // namespace pal4
