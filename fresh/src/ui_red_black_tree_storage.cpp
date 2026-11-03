#include "pal4/ui_red_black_tree_storage.h"

namespace pal4 {
namespace {

template <typename Cleanup>
std::uint32_t FreeNodesImpl(
    std::uint32_t node, IUiRedBlackTreeStorageRuntime& runtime,
    Cleanup cleanup) {
    while (node != runtime.SharedNil()) {
        (void)FreeNodesImpl(runtime.ReadWord(node + 8u),
                            runtime, cleanup);
        const auto left = runtime.ReadWord(node);
        cleanup(node + 12u);
        runtime.FreeMemory(node);
        node = left;
    }
    return runtime.SharedNil();
}

}  // namespace

std::uint32_t UiRedBlackTreeStorage::FreeNodes(
    std::uint32_t node, IUiRedBlackTreeStorageRuntime& runtime) {
    return FreeNodesImpl(node, runtime,
                         [&](std::uint32_t data) {
                             runtime.CleanupData(data);
                         });
}

std::uint32_t UiRedBlackTreeStorage::FreeCeguiNodes(
    std::uint32_t node, IUiCeguiRedBlackTreeStorageRuntime& runtime) {
    return FreeNodesImpl(node, runtime,
                         [&](std::uint32_t data) {
                             runtime.FreeCeguiString(data);
                         });
}

std::uint32_t UiRedBlackTreeStorage::Construct(
    const std::uint32_t tree, IUiRedBlackTreeStorageRuntime& runtime) {
    const auto candidate = runtime.AllocateMemory(0x24u);
    runtime.WriteWord(candidate + 32u, 1u);
    runtime.WriteWord(candidate + 4u, 0u);
    const auto existing_nil = runtime.SharedNil();
    if (existing_nil == 0u) {
        runtime.SetSharedNil(candidate);
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    }
    runtime.SetSharedNilUsers(runtime.SharedNilUsers() + 1u);
    if (existing_nil != 0u) runtime.FreeMemory(candidate);
    const auto head = runtime.AllocateMemory(0x24u);
    runtime.WriteWord(head + 4u, runtime.SharedNil());
    runtime.WriteWord(head + 32u, 0u);
    runtime.WriteWord(tree + 4u, head);
    runtime.WriteWord(tree + 12u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    return head;
}

}  // namespace pal4
