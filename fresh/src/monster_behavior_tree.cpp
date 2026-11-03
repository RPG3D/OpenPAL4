#include "pal4/monster_behavior_tree.h"

namespace pal4 {
namespace {

void SetByte(MonsterBehaviorTreeState& state, std::uint32_t offset,
             std::uint8_t value) {
    auto& word = state.words[offset / 4u];
    const auto shift = (offset & 3u) * 8u;
    word = (word & ~(0xffu << shift)) |
        (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

void MonsterBehaviorTreeRuntimeStub::InitializeRedBlackTree(
    MonsterBehaviorTreeState&, std::uint32_t) {}
std::uint32_t MonsterBehaviorTreeRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
void MonsterBehaviorTreeRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
std::uint32_t MonsterBehaviorTreeRuntimeStub::AllocateMemory(
    std::uint32_t) { return 0u; }
void MonsterBehaviorTreeRuntimeStub::DestroySubtree(
    std::uint32_t, std::uint32_t) {}
void MonsterBehaviorTreeRuntimeStub::GetFirstElement(std::uint32_t) {}
void MonsterBehaviorTreeRuntimeStub::RemoveNode(
    std::uint32_t, std::uint32_t) {}
std::uint32_t MonsterBehaviorTreeRuntimeStub::SharedNil() { return nil_; }
void MonsterBehaviorTreeRuntimeStub::SetSharedNil(
    std::uint32_t address) { nil_ = address; }
std::uint32_t MonsterBehaviorTreeRuntimeStub::SharedReferenceCount() {
    return references_;
}
void MonsterBehaviorTreeRuntimeStub::SetSharedReferenceCount(
    std::uint32_t count) { references_ = count; }
std::uint32_t MonsterBehaviorTreeRuntimeStub::FreeMemory(
    std::uint32_t) { return 0u; }

std::uint32_t MonsterBehaviorTree::Initialize(
    MonsterBehaviorTreeState& state, const std::uint32_t owner_address,
    const std::uint8_t first, const std::uint8_t second,
    const std::uint8_t third, IMonsterBehaviorTreeRuntime& runtime) {
    SetByte(state, 4u, first);
    SetByte(state, 5u, second);
    SetByte(state, 12u, third);
    runtime.InitializeRedBlackTree(state, owner_address + 4u);
    return owner_address + 4u;
}

std::uint32_t MonsterBehaviorTree::Destroy(
    MonsterBehaviorTreeState& state, const std::uint32_t owner_address,
    IMonsterBehaviorTreeRuntime& runtime) {
    state.words[0] = kVtable;
    const auto tree_address = owner_address + 4u;
    const auto header = state.words[2];
    auto cursor = runtime.ReadDword(header);

    if (state.words[4] != 0u) {
        // The machine-code comparison immediately before this branch compares
        // EAX with itself, so every nonempty tree takes the subtree path.
        runtime.DestroySubtree(tree_address, runtime.ReadDword(header + 4u));
        runtime.WriteDword(header + 4u, runtime.SharedNil());
        state.words[4] = 0u;
        runtime.WriteDword(header, header);
        runtime.WriteDword(header + 8u, header);
        runtime.GetFirstElement(tree_address);
    } else {
        while (cursor != header) {
            const auto current = cursor;
            (void)AdvanceSuccessor(cursor, runtime);
            runtime.RemoveNode(tree_address, current);
        }
    }

    (void)runtime.FreeMemory(header);
    state.words[2] = 0u;
    state.words[4] = 0u;
    const auto remaining = runtime.SharedReferenceCount() - 1u;
    runtime.SetSharedReferenceCount(remaining);
    if (remaining != 0u)
        return remaining;
    const auto nil = runtime.SharedNil();
    runtime.SetSharedNil(0u);
    if (nil != 0u)
        return runtime.FreeMemory(nil);
    return 0u;
}

std::uint32_t MonsterBehaviorTree::Delete(
    MonsterBehaviorTreeState& state, const std::uint32_t owner_address,
    const std::uint8_t flags, IMonsterBehaviorTreeRuntime& runtime) {
    (void)Destroy(state, owner_address, runtime);
    if ((flags & 1u) != 0u)
        (void)runtime.FreeMemory(owner_address);
    return owner_address;
}

}  // namespace pal4
