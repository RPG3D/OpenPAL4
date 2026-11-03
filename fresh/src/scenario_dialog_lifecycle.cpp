#include "pal4/scenario_dialog.h"

#include <cstdint>

namespace pal4 {

std::uint32_t ScenarioDialog::GetFirstElement(
    const std::uint32_t tree, const std::uint32_t output,
    IScenarioDialogLifecycleRuntime& runtime) {
    const auto sentinel = runtime.ReadWord(tree + 4u);
    runtime.WriteWord(output, runtime.ReadWord(sentinel));
    return output;
}

std::uint32_t ScenarioDialog::DestructTree(
    const std::uint32_t tree,
    IScenarioDialogLifecycleRuntime& runtime) {
    const auto sentinel = runtime.ReadWord(tree + 4u);
    const auto count = runtime.ReadWord(tree + 12u);
    auto node = runtime.ReadWord(sentinel);
    if (count != 0u) {
        runtime.FreeNodes(tree, runtime.ReadWord(sentinel + 4u));
        runtime.WriteWord(sentinel + 4u, runtime.SharedNil());
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(sentinel, sentinel);
        runtime.WriteWord(sentinel + 8u, sentinel);
        runtime.FirstElement(tree);
    } else {
        while (node != sentinel) {
            const auto next = runtime.Successor(node);
            runtime.RemoveNode(tree, node);
            node = next;
        }
    }
    (void)runtime.Free(sentinel);
    runtime.WriteWord(tree + 4u, 0u);
    runtime.WriteWord(tree + 12u, 0u);
    const auto references = runtime.DecrementSharedRef();
    if (references != 0u) return references;
    const auto nil = runtime.TakeSharedNil();
    return nil != 0u ? runtime.Free(nil) : 0u;
}

std::uint32_t ScenarioDialog::Destruct(
    const std::uint32_t object,
    IScenarioDialogLifecycleRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 40u, kTreeVtable);
    try {
        (void)DestructTree(object + 64u, runtime);
    } catch (...) {
        (void)runtime.DestructBase(object);
        throw;
    }
    return runtime.DestructBase(object);
}

std::uint32_t ScenarioDialog::DeletingDestruct(
    const std::uint32_t object, const std::uint8_t flags,
    IScenarioDialogLifecycleRuntime& runtime) {
    (void)Destruct(object, runtime);
    if ((flags & 1u) != 0u) (void)runtime.Free(object);
    return object;
}

}  // namespace pal4
