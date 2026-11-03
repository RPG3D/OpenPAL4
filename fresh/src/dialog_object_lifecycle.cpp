#include "pal4/dialog_object.h"

#include <bit>
#include <cstddef>

namespace pal4 {
namespace {

struct SharedTree final {
    std::uint32_t sentinel = 0u;
    std::uint32_t references = 0u;
};
SharedTree first_tree;
SharedTree second_tree;

void WriteByte(DialogObjectState& state, std::size_t offset,
               std::uint8_t value) {
    const auto shift = static_cast<unsigned>(8u * (offset % 4u));
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) | (std::uint32_t{value} << shift);
}

std::uint32_t InitializeTree(SharedTree& shared, std::uint32_t node_size,
                             IDialogObjectRuntime& runtime) {
    const auto candidate = runtime.Allocate(node_size);
    runtime.WriteHeapWord(candidate, 4u, 0u);
    runtime.WriteHeapWord(candidate, node_size - 4u, 1u);
    if (shared.sentinel == 0u) {
        shared.sentinel = candidate;
        runtime.WriteHeapWord(candidate, 0u, 0u);
        runtime.WriteHeapWord(candidate, 8u, 0u);
    } else {
        runtime.Free(candidate);
    }
    ++shared.references;
    const auto head = runtime.Allocate(node_size);
    runtime.WriteHeapWord(head, 4u, shared.sentinel);
    runtime.WriteHeapWord(head, node_size - 4u, 0u);
    runtime.WriteHeapWord(head, 0u, head);
    runtime.WriteHeapWord(head, 8u, head);
    return head;
}

void ReleaseTree(SharedTree& shared, std::uint32_t head,
                 IDialogObjectRuntime& runtime) {
    runtime.Free(head);
    if (--shared.references == 0u) {
        runtime.Free(shared.sentinel);
        shared.sentinel = 0u;
    }
}
}  // namespace

std::uint32_t DialogObject::CreateInstance(
    DialogObjectState& state, IDialogObjectRuntime& runtime) {
    const auto address = runtime.Allocate(0xdcu);
    if (address != 0u)
        Construct(state, runtime);
    return address;
}

std::uint32_t DialogObject::DestroyInstance(
    std::uint32_t object, std::uint32_t first, std::uint32_t second,
    IDialogObjectRuntime& runtime) {
    if (object == 0u)
        return 0u;
    runtime.CleanupBeforeDelete(object, first, second);
    return runtime.VirtualDelete(object, 1u);
}

DialogObjectState& DialogObject::Construct(
    DialogObjectState& state, IDialogObjectRuntime& runtime) {
    WriteByte(state, 4u, 0u);
    state.words[2u] = 0u;
    state.words[3u] = 0u;
    const auto text_memory = runtime.Allocate(0x10u);
    state.words[4u] = text_memory != 0u
        ? runtime.InitializeTextManager(text_memory) : 0u;
    runtime.InitializeXmlDocument(state);
    state.words[20u] = 0u;
    state.words[21u] = std::bit_cast<std::uint32_t>(1.0f);
    const auto stack_byte = runtime.ConstructorStackByte();
    WriteByte(state, 92u, stack_byte);
    for (const auto index : {24u, 25u, 26u}) state.words[index] = 0u;
    WriteByte(state, 108u, stack_byte);
    for (const auto index : {28u, 29u, 30u}) state.words[index] = 0u;
    WriteByte(state, 124u, stack_byte);
    for (const auto index : {32u, 33u, 34u}) state.words[index] = 0u;
    WriteByte(state, 140u, stack_byte);
    WriteByte(state, 141u, stack_byte);
    WriteByte(state, 148u, 0u);
    state.words[36u] = InitializeTree(first_tree, 0x24u, runtime);
    state.words[38u] = 0u;
    WriteByte(state, 156u, stack_byte);
    WriteByte(state, 157u, stack_byte);
    WriteByte(state, 164u, 0u);
    state.words[40u] = InitializeTree(second_tree, 0x2cu, runtime);
    state.words[42u] = 0u;
    for (const auto index : {47u, 48u, 49u})
        state.words[index] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[43u] = 0u;
    WriteByte(state, 212u, 0u);
    state.words[54u] = 0u;
    state.words[0u] = kVtable;
    for (const auto index : {44u, 45u, 46u, 50u, 51u, 52u})
        state.words[index] = 0u;
    return state;
}

DialogObjectState& DialogObject::Destroy(
    DialogObjectState& state, IDialogObjectRuntime& runtime) {
    state.words[0u] = kVtable;
    if (state.words[4u] != 0u) {
        runtime.DestroyTextManager(state.words[4u]);
        runtime.Free(state.words[4u]);
        state.words[4u] = 0u;
    }
    ClearSecondTree(state, runtime);
    ClearFirstTree(state, runtime);
    runtime.Free(state.words[32u]);
    for (const auto index : {32u, 33u, 34u}) state.words[index] = 0u;
    for (auto address = state.words[28u]; address != state.words[29u];
         address += 20u)
        runtime.DestroyStringRecord(address);
    runtime.Free(state.words[28u]);
    for (const auto index : {28u, 29u, 30u}) state.words[index] = 0u;
    runtime.Free(state.words[24u]);
    for (const auto index : {24u, 25u, 26u}) state.words[index] = 0u;
    runtime.DestroyXmlDocument(state);
    return state;
}

void DialogObject::ClearFirstTree(DialogObjectState& state,
                                  IDialogObjectRuntime& runtime) {
    const auto head = state.words[36u];
    if (state.words[38u] != 0u) {
        runtime.DestroyFirstTreeBranch(runtime.ReadHeapWord(head, 4u));
        runtime.WriteHeapWord(head, 4u, first_tree.sentinel);
        state.words[38u] = 0u;
        runtime.WriteHeapWord(head, 0u, head);
        runtime.WriteHeapWord(head, 8u, head);
    } else {
        for (auto node = runtime.ReadHeapWord(head, 0u); node != head;) {
            const auto next = runtime.NextFirstTreeNode(node);
            runtime.EraseFirstTreeNode(node);
            node = next;
        }
    }
    ReleaseTree(first_tree, head, runtime);
    state.words[36u] = 0u;
    state.words[38u] = 0u;
}

void DialogObject::ClearSecondTree(DialogObjectState& state,
                                   IDialogObjectRuntime& runtime) {
    const auto head = state.words[40u];
    if (state.words[42u] != 0u) {
        runtime.DestroySecondTreeBranch(runtime.ReadHeapWord(head, 4u));
        runtime.WriteHeapWord(head, 4u, second_tree.sentinel);
        state.words[42u] = 0u;
        runtime.WriteHeapWord(head, 0u, head);
        runtime.WriteHeapWord(head, 8u, head);
    } else {
        for (auto node = runtime.ReadHeapWord(head, 0u); node != head;) {
            const auto next = runtime.NextSecondTreeNode(node);
            runtime.EraseSecondTreeNode(node);
            node = next;
        }
    }
    ReleaseTree(second_tree, head, runtime);
    state.words[40u] = 0u;
    state.words[42u] = 0u;
}

}  // namespace pal4
