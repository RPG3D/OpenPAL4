#include "pal4/combat_result_data_copier.h"

#include <array>
#include <new>

namespace pal4 {
namespace {

struct TreeState {
    bool referenced = false;
    bool ready = false;
};

void ReleaseSentinel(ICombatResultDataCopyRuntime& runtime) {
    const auto remaining = runtime.ReadWord(
        CombatResultDataCopier::kSharedRefCountAddress) - 1u;
    runtime.WriteWord(CombatResultDataCopier::kSharedRefCountAddress,
                      remaining);
    if (remaining == 0u) {
        const auto nil = runtime.ReadWord(
            CombatResultDataCopier::kSharedNilAddress);
        runtime.WriteWord(CombatResultDataCopier::kSharedNilAddress, 0u);
        if (nil != 0u) runtime.Free(nil);
    }
}

void CopyTree(std::uint32_t destination, std::uint32_t source,
              TreeState& state, ICombatResultDataCopyRuntime& runtime) {
    runtime.WriteByte(destination, runtime.ReadByte(source));
    runtime.WriteByte(destination + 1u, runtime.ReadByte(source + 1u));
    runtime.WriteByte(destination + 8u, runtime.ReadByte(source + 8u));

    const auto candidate = runtime.CreateNode(0u, 1u);
    if (candidate == 0u) throw std::bad_alloc();
    auto nil = runtime.ReadWord(CombatResultDataCopier::kSharedNilAddress);
    if (nil == 0u) {
        nil = candidate;
        runtime.WriteWord(CombatResultDataCopier::kSharedNilAddress, nil);
        runtime.WriteWord(nil, 0u);
        runtime.WriteWord(nil + 8u, 0u);
    }
    runtime.WriteWord(CombatResultDataCopier::kSharedRefCountAddress,
        runtime.ReadWord(CombatResultDataCopier::kSharedRefCountAddress) + 1u);
    state.referenced = true;
    if (candidate != nil) runtime.Free(candidate);

    const auto head = runtime.CreateNode(nil, 0u);
    if (head == 0u) throw std::bad_alloc();
    runtime.WriteWord(destination + 4u, head);
    runtime.WriteWord(destination + 12u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    state.ready = true;
    runtime.CopyAssignTree(destination, source);
}

void RollbackTree(std::uint32_t tree, const TreeState& state,
                  ICombatResultDataCopyRuntime& runtime) {
    if (!state.referenced) return;
    if (state.ready)
        runtime.DestroyTree(tree);
    else
        ReleaseSentinel(runtime);
}

}  // namespace

std::uint32_t CombatResultDataCopier::Copy(
    std::uint32_t destination, std::uint32_t source,
    ICombatResultDataCopyRuntime& runtime) {
    std::array<TreeState, 2> states{};
    std::uint32_t records = 0u;
    try {
        runtime.WriteByte(destination, runtime.ReadByte(source));
        for (std::uint32_t offset = 0u; offset <= 16u; offset += 4u)
            runtime.WriteWord(destination + offset,
                              runtime.ReadWord(source + offset));
        CopyTree(destination + 20u, source + 20u, states[0], runtime);
        CopyTree(destination + 36u, source + 36u, states[1], runtime);
        for (; records < 4u; ++records)
            (void)CombatRewardRecord::Copy(
                destination + 52u + records * 40u,
                source + 52u + records * 40u, runtime);
        return destination;
    } catch (...) {
        while (records != 0u) {
            --records;
            runtime.CleanupRecord(destination + 52u + records * 40u);
        }
        RollbackTree(destination + 36u, states[1], runtime);
        RollbackTree(destination + 20u, states[0], runtime);
        throw;
    }
}

}  // namespace pal4
