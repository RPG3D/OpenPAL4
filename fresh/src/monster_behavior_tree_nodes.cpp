#include "pal4/monster_behavior_tree.h"

namespace pal4 {

std::uint32_t MonsterBehaviorTree::DestroySubtreeNodes(
    std::uint32_t root_address, IMonsterBehaviorTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    auto node = root_address;
    while (node != nil) {
        (void)DestroySubtreeNodes(runtime.ReadDword(node + 8u), runtime);
        const auto left = runtime.ReadDword(node);
        (void)runtime.FreeMemory(node);
        node = left;
    }
    return runtime.SharedNil();
}

std::uint32_t MonsterBehaviorTree::InitializeNodes(
    MonsterBehaviorTreeState& state, IMonsterBehaviorTreeRuntime& runtime) {
    const auto candidate = runtime.AllocateMemory(0x18u);
    runtime.WriteDword(candidate + 20u, 1u);
    runtime.WriteDword(candidate + 4u, 0u);
    if (runtime.SharedNil() == 0u) {
        runtime.SetSharedNil(candidate);
        runtime.WriteDword(candidate, 0u);
        runtime.WriteDword(candidate + 8u, 0u);
    } else {
        (void)runtime.FreeMemory(candidate);
    }
    runtime.SetSharedReferenceCount(runtime.SharedReferenceCount() + 1u);

    const auto header = runtime.AllocateMemory(0x18u);
    runtime.WriteDword(header + 4u, runtime.SharedNil());
    runtime.WriteDword(header + 20u, 0u);
    state.words[2] = header;
    state.words[4] = 0u;
    runtime.WriteDword(header, header);
    runtime.WriteDword(header + 8u, header);
    return header;
}

std::uint32_t MonsterBehaviorTree::AdvanceSuccessor(
    std::uint32_t& cursor, IMonsterBehaviorTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    auto node = runtime.ReadDword(cursor + 8u);
    if (node != nil) {
        auto left = runtime.ReadDword(node);
        while (left != nil) {
            node = left;
            left = runtime.ReadDword(node);
        }
        cursor = node;
        return left;
    }

    auto parent = runtime.ReadDword(cursor + 4u);
    while (cursor == runtime.ReadDword(parent + 8u)) {
        cursor = parent;
        parent = runtime.ReadDword(parent + 4u);
    }
    if (runtime.ReadDword(cursor + 8u) != parent)
        cursor = parent;
    return parent;
}

std::uint32_t& MonsterBehaviorTree::SuccessorCursor(
    std::uint32_t& cursor, std::uint32_t& output,
    IMonsterBehaviorTreeRuntime& runtime) {
    const auto original = cursor;
    (void)AdvanceSuccessor(cursor, runtime);
    output = original;
    return output;
}

}  // namespace pal4
