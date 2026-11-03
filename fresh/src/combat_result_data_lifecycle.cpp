#include "pal4/combat_result_data_lifecycle.h"

#include <array>
#include <new>

namespace pal4 {
namespace {

struct TreeState {
    bool referenced = false;
    bool head_ready = false;
};

std::uint32_t Allocate24(ICombatResultDataConstructionRuntime& runtime) {
    const auto address = runtime.Allocate(24u);
    if (address == 0u) throw std::bad_alloc();
    return address;
}

void InitializeTree(std::uint32_t tree, TreeState& state,
                    ICombatResultDataConstructionRuntime& runtime) {
    const auto candidate = Allocate24(runtime);
    runtime.WriteWord(candidate + 4u, 0u);
    runtime.WriteWord(candidate + 20u, 1u);
    auto nil = runtime.ReadWord(CombatResultDataLifecycle::kSharedNilAddress);
    if (nil == 0u) {
        runtime.WriteWord(CombatResultDataLifecycle::kSharedNilAddress,
                          candidate);
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
        nil = candidate;
    }
    const auto count = runtime.ReadWord(
        CombatResultDataLifecycle::kSharedRefCountAddress);
    runtime.WriteWord(CombatResultDataLifecycle::kSharedRefCountAddress,
                      count + 1u);
    state.referenced = true;
    if (nil != candidate) runtime.Free(candidate);

    const auto head = Allocate24(runtime);
    runtime.WriteWord(head + 4u, nil);
    runtime.WriteWord(head + 20u, 0u);
    runtime.WriteWord(tree + 4u, head);
    runtime.WriteWord(tree + 12u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    state.head_ready = true;
}

void ReleaseReference(ICombatResultDataConstructionRuntime& runtime) {
    const auto remaining = runtime.ReadWord(
        CombatResultDataLifecycle::kSharedRefCountAddress) - 1u;
    runtime.WriteWord(CombatResultDataLifecycle::kSharedRefCountAddress,
                      remaining);
    if (remaining == 0u) {
        const auto nil = runtime.ReadWord(
            CombatResultDataLifecycle::kSharedNilAddress);
        runtime.WriteWord(CombatResultDataLifecycle::kSharedNilAddress, 0u);
        if (nil != 0u) runtime.Free(nil);
    }
}

void RollbackTree(std::uint32_t tree, const TreeState& state,
                  ICombatResultDataConstructionRuntime& runtime) {
    if (!state.referenced) return;
    if (state.head_ready) {
        const auto head = runtime.ReadWord(tree + 4u);
        runtime.Free(head);
        runtime.WriteWord(tree + 4u, 0u);
        runtime.WriteWord(tree + 12u, 0u);
    }
    ReleaseReference(runtime);
}

}  // namespace

void CombatResultDataLifecycle::InitializeRecord(
    std::uint32_t record, std::uint8_t seed,
    ICombatResultFieldRuntime& runtime) {
    runtime.WriteByte(record, seed);
    for (std::uint32_t offset = 4u; offset <= 32u; offset += 4u)
        runtime.WriteWord(record + offset, 0u);
    runtime.WriteByte(record + 36u, seed);
}
void CombatResultDataLifecycle::InitializeScalars(std::uint32_t object,ICombatResultFieldRuntime& runtime){
    for(std::uint32_t offset=0u;offset<16u;offset+=4u)runtime.WriteWord(object+offset,0u);
    runtime.WriteWord(object+16u,3u);
}

std::uint32_t CombatResultDataLifecycle::Construct(
    std::uint32_t object, std::uint8_t object_seed,
    std::uint8_t record_seed,
    ICombatResultDataConstructionRuntime& runtime) {
    const std::array<std::uint32_t, 2> trees{object + 20u, object + 36u};
    std::array<TreeState, 2> states{};
    std::uint32_t records_completed = 0u;
    try {
        for (std::uint32_t i = 0u; i < trees.size(); ++i) {
            runtime.WriteByte(trees[i], object_seed);
            runtime.WriteByte(trees[i] + 1u, object_seed);
            runtime.WriteByte(trees[i] + 8u, 0u);
            InitializeTree(trees[i], states[i], runtime);
        }
        for (std::uint32_t i = 0u; i < 4u; ++i) {
            InitializeRecord(object + 52u + i * 40u, record_seed, runtime);
            ++records_completed;
        }
        InitializeScalars(object,runtime);
        return object;
    } catch (...) {
        while (records_completed != 0u) {
            --records_completed;
            runtime.CleanupRecord(object + 52u + records_completed * 40u);
        }
        RollbackTree(trees[1], states[1], runtime);
        RollbackTree(trees[0], states[0], runtime);
        throw;
    }
}

std::uint32_t CleanupTreeImpl(const std::uint32_t tree,
                              ICombatResultDataLifecycleRuntime& runtime,
                              const bool refresh_first) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        runtime.FreeNodes(tree, runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        if (refresh_first) runtime.FirstElement(tree);
    } else {
        while (node != head) {
            const auto current = node;
            node = runtime.Successor(node);
            runtime.RemoveNode(tree, current);
        }
    }
    (void)runtime.Free(head);
    runtime.WriteWord(tree + 4u, 0u);
    runtime.WriteWord(tree + 12u, 0u);
    const auto remaining = runtime.DecrementSharedRef();
    if (remaining != 0u) return remaining;
    const auto nil = runtime.TakeSharedNil();
    return nil != 0u ? runtime.Free(nil) : 0u;
}

std::uint32_t CombatResultDataLifecycle::DestructTree(
    const std::uint32_t tree,
    ICombatResultDataLifecycleRuntime& runtime) {
    return CleanupTreeImpl(tree, runtime, true);
}

std::uint32_t CombatResultDataLifecycle::CleanupTree(
    const std::uint32_t tree,
    ICombatResultDataLifecycleRuntime& runtime) {
    return CleanupTreeImpl(tree, runtime, false);
}

std::uint32_t CombatResultDataLifecycle::Destruct(
    const std::uint32_t object,
    ICombatResultDataLifecycleRuntime& runtime) {
    for (std::uint32_t index = 4u; index != 0u; --index) {
        runtime.CleanupRecord(object + 52u + (index - 1u) * 40u);
    }
    (void)DestructTree(object + 36u, runtime);
    return DestructTree(object + 20u, runtime);
}

}  // namespace pal4
