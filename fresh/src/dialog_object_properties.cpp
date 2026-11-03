#include "pal4/dialog_object.h"

namespace pal4 {

bool DialogObject::GetStoredVector(
    const DialogObjectState& state, std::string_view key,
    std::array<float, 3u>& output,
    IDialogObjectPropertyRuntime& runtime) {
    const auto node = runtime.SecondTreeCandidate(state, key);
    if (node == state.words[40u] ||
        runtime.CompareKeyToNode(key, node) < 0)
        return false;
    output = runtime.ReadStoredVector(node);
    return true;
}

bool DialogObject::GetVector(
    const DialogObjectState& state, std::string_view key,
    std::array<float, 3u>& output,
    IDialogObjectPropertyRuntime& runtime) {
    const auto node = runtime.FirstTreeCandidate(state, key);
    if (node != state.words[36u] &&
        runtime.CompareKeyToNode(key, node) >= 0) {
        runtime.ReadFacetVector(runtime.ReadFacetHandle(node), output);
        return true;
    }
    return GetStoredVector(state, key, output, runtime);
}

void DialogObject::SetFacet(DialogObjectState& state, std::string_view key,
                            std::uint32_t facet,
                            IDialogObjectPropertyRuntime& runtime) {
    if (facet == 0u)
        return;
    const auto node = runtime.FirstTreeCandidate(state, key);
    if (node != state.words[36u] &&
        runtime.CompareKeyToNode(key, node) >= 0) {
        runtime.WriteFacetHandle(node, facet);
    } else {
        runtime.InsertFacet(state, key, facet);
    }
}

void DialogObject::ClearFacets(
    DialogObjectState& state, IDialogObjectFacetCleanupRuntime& runtime) {
    const auto head = state.words[36u];
    if (state.words[38u] != 0u) {
        const auto shared = runtime.SharedFirstTreeSentinel();
        for (auto node = runtime.ReadNodeWord(head, 4u); node != shared;) {
            const auto next = runtime.ReadNodeWord(node, 0u);
            runtime.DeleteFirstTreeSubtree(runtime.ReadNodeWord(node, 8u));
            runtime.ReleaseNodeText(node + 12u);
            runtime.FreeNode(node);
            node = next;
        }
        runtime.WriteNodeWord(head, 4u, shared);
        state.words[38u] = 0u;
        runtime.WriteNodeWord(head, 0u, head);
        runtime.WriteNodeWord(head, 8u, head);
    } else {
        for (auto node = runtime.ReadNodeWord(head, 0u); node != head;) {
            const auto next = runtime.NextFirstTreeNode(node);
            runtime.EraseFirstTreeNode(node);
            node = next;
        }
    }
}

}  // namespace pal4
