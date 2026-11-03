#include "pal4/rw_tex_dictionary_tree.h"
#include "pal4/ui_red_black_tree_remove.h"

#include <cstring>

namespace pal4 {
namespace {

class TextureRemovalAdapter final : public IUiRedBlackTreeRemoveRuntime {
public:
    static constexpr std::uint32_t kTreeAddress = 0xffff1000u;
    TextureRemovalAdapter(RwTexDictionaryTreeState& state,
                          IRwTexDictionaryTreeRuntime& runtime)
        : state_(state), runtime_(runtime) {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == kTreeAddress + 4u) return state_.head;
        if (address == kTreeAddress + 12u) return state_.count;
        return runtime_.ReadWord(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        if (address == kTreeAddress + 12u) {
            state_.count = value;
            return;
        }
        runtime_.WriteWord(address, value);
    }
    std::uint32_t SharedNil() override { return runtime_.SharedNil(); }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return runtime_.ReadByte(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        runtime_.WriteByte(address, value);
    }
    void FreeMemory(std::uint32_t address) override {
        (void)runtime_.Free(address);
    }

private:
    RwTexDictionaryTreeState& state_;
    IRwTexDictionaryTreeRuntime& runtime_;
};

RwTexDictionaryName MakeLookupName(
    const char* name, IRwTexDictionaryInsertRuntime& runtime) {
    RwTexDictionaryName lookup;
    const auto tag = static_cast<std::uint8_t>(
        reinterpret_cast<std::uintptr_t>(name));
    runtime.InitializeName(lookup, tag);
    const auto length = static_cast<std::uint32_t>(std::strlen(name));
    if (runtime.AllocateName(lookup, length, true)) {
        runtime.CopyName(lookup, name, length);
        runtime.SetNameSize(lookup, length);
    }
    return lookup;
}

void ReleaseNodeName(const std::uint32_t node,
                     IRwTexDictionaryTreeRuntime& runtime) {
    const auto name = node + 12u;
    const auto buffer = runtime.ReadWord(name + 4u);
    if (buffer != 0u) {
        const auto counter_address = buffer - 1u;
        const auto counter = runtime.ReadByte(counter_address);
        if (counter == 0u || counter == 0xffu) {
            (void)runtime.Free(counter_address);
        } else {
            runtime.WriteByte(counter_address,
                              static_cast<std::uint8_t>(counter - 1u));
        }
    }
    runtime.WriteWord(name + 4u, 0u);
    runtime.WriteWord(name + 8u, 0u);
    runtime.WriteWord(name + 12u, 0u);
}

}  // namespace

RwTexDictionaryTreeState* RwTexDictionaryTree::Construct(
    RwTexDictionaryTreeState& state, const std::uint8_t tag,
    const std::uint8_t second, IRwTexDictionaryTreeRuntime& runtime) {
    state.tag = tag;
    state.second = second;
    state.flag = 0u;
    const auto candidate = runtime.Allocate(40u);
    runtime.WriteWord(candidate + 4u, 0u);
    runtime.WriteWord(candidate + 36u, 1u);
    if (runtime.SharedNil() == 0u) {
        runtime.SetSharedNil(candidate);
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    } else {
        (void)runtime.Free(candidate);
    }
    runtime.SetUsers(runtime.Users() + 1u);
    const auto head = runtime.Allocate(40u);
    runtime.WriteWord(head + 4u, runtime.SharedNil());
    runtime.WriteWord(head + 36u, 0u);
    state.head = head;
    state.count = 0u;
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    return &state;
}

std::uint32_t RwTexDictionaryTree::Destruct(
    RwTexDictionaryTreeState& state,
    IRwTexDictionaryTreeRuntime& runtime) {
    const auto head = state.head;
    auto node = runtime.ReadWord(head);
    if (state.count != 0u) {
        (void)DestroyRecursive(runtime.ReadWord(head + 4u), runtime);
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        state.count = 0u;
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.FirstElement(state);
    } else {
        while (node != head) {
            const auto current = node;
            node = NextNode(current, runtime);
            (void)RemoveNodeAndRebalance(state, current, runtime);
        }
    }
    (void)runtime.Free(head);
    state.head = 0u;
    state.count = 0u;
    const auto users = runtime.Users() - 1u;
    runtime.SetUsers(users);
    if (users == 0u) {
        const auto nil = runtime.SharedNil();
        runtime.SetSharedNil(0u);
        if (nil != 0u) return runtime.Free(nil);
    }
    return users;
}

std::uint32_t RwTexDictionaryTree::Cleanup(
    RwTexDictionaryTreeState& state,
    IRwTexDictionaryTreeRuntime& runtime) {
    const auto head = state.head;
    const auto nil = runtime.SharedNil();
    auto node = runtime.ReadWord(head);
    while (node != head) {
        const auto dictionary = runtime.ReadWord(node + 28u);
        runtime.WriteWord(node + 32u, 0u);
        runtime.DestroyDictionary(dictionary);

        auto right = runtime.ReadWord(node + 8u);
        if (right != nil) {
            node = right;
            while (runtime.ReadWord(node) != runtime.SharedNil()) {
                node = runtime.ReadWord(node);
            }
        } else {
            auto parent = runtime.ReadWord(node + 4u);
            while (node == runtime.ReadWord(parent + 8u)) {
                node = parent;
                parent = runtime.ReadWord(parent + 4u);
            }
            if (runtime.ReadWord(node + 8u) != parent) node = parent;
        }
    }

    node = runtime.ReadWord(head);
    if (state.count != 0u) {
        auto current = runtime.ReadWord(head + 4u);
        while (current != runtime.SharedNil()) {
            (void)DestroyRecursive(runtime.ReadWord(current + 8u), runtime);
            const auto left = runtime.ReadWord(current);
            (void)DestroyNode(current + 12u, 0u, runtime);
            (void)runtime.Free(current);
            current = left;
        }
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        state.count = 0u;
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        return head;
    }
    while (node != head) {
        const auto current = node;
        node = NextNode(current, runtime);
        (void)RemoveNodeAndRebalance(state, current, runtime);
    }
    return node;
}

bool RwTexDictionaryTree::AddTexture(
    RwTexDictionaryTreeState& state, const char* name,
    const std::uint32_t texture, IRwTexDictionaryInsertRuntime& names,
    IRwTexDictionaryNodeInsertRuntime& nodes) {
    auto lookup = MakeLookupName(name, names);
    const auto found = FindNode(state, lookup, names, nodes);
    if (found != state.head) {
        (void)ReleaseName(lookup, names);
        return false;
    }

    RwTexDictionaryName inserted;
    names.InitializeName(inserted, lookup.tag);
    names.AssignName(inserted, lookup, 0u, 0xffffffffu);
    RwTexDictionaryEntry entry{inserted, texture, 0u};
    (void)InsertInternal(state, entry, nodes);
    (void)ReleaseName(inserted, names);
    (void)ReleaseName(lookup, names);
    return true;
}

std::uint32_t RwTexDictionaryTree::ReleaseName(
    RwTexDictionaryName& name,
    IRwTexDictionaryInsertRuntime& runtime) {
    std::uint32_t result = 0u;
    if (name.buffer != 0u) {
        const auto allocation = name.buffer - 1u;
        const auto references = runtime.ReadByte(allocation);
        if (references == 0u || references == 0xffu) {
            result = runtime.FreeNameBuffer(allocation);
        } else {
            runtime.WriteByte(allocation,
                              static_cast<std::uint8_t>(references - 1u));
            result = allocation;
        }
    }
    name.buffer = 0u;
    name.size = 0u;
    name.capacity = 0u;
    return result;
}

bool RwTexDictionaryTree::RemoveTextureByName(
    RwTexDictionaryTreeState& state, const char* name,
    IRwTexDictionaryInsertRuntime& names,
    IRwTexDictionaryTreeRuntime& tree) {
    auto lookup = MakeLookupName(name, names);
    const auto node = FindNode(state, lookup, names, tree);
    if (node == state.head) {
        (void)ReleaseName(lookup, names);
        return false;
    }
    const auto dictionary = tree.ReadWord(node + 28u);
    if (dictionary == 0u) {
        (void)ReleaseName(lookup, names);
        return false;
    }
    const auto references = tree.ReadWord(node + 32u) - 1u;
    tree.WriteWord(node + 32u, references);
    if (references == 0u) {
        tree.DestroyDictionary(dictionary);
        tree.WriteWord(node + 28u, 0u);
        (void)RemoveNodeAndRebalance(state, node, tree);
    }
    (void)ReleaseName(lookup, names);
    return true;
}

bool RwTexDictionaryTree::SetCurrent(
    RwTexDictionaryTreeState& state, const char* name,
    IRwTexDictionaryInsertRuntime& names,
    IRwTexDictionaryTreeRuntime& tree) {
    auto lookup = MakeLookupName(name, names);
    auto node = FindNodeCandidate(state, lookup, names, tree);
    if (node == state.head ||
        names.CompareNodeName(lookup, node + 12u) < 0) {
        node = state.head;
    }
    if (node == state.head) {
        (void)ReleaseName(lookup, names);
        return false;
    }
    const auto dictionary = tree.ReadWord(node + 28u);
    if (dictionary == 0u) {
        (void)ReleaseName(lookup, names);
        return false;
    }
    tree.WriteWord(node + 32u, tree.ReadWord(node + 32u) + 1u);
    names.SetCurrentDictionary(dictionary);
    (void)ReleaseName(lookup, names);
    return true;
}

bool RwTexDictionaryTree::CheckTextureExists(
    RwTexDictionaryTreeState& state, const char* name,
    IRwTexDictionaryInsertRuntime& names,
    IRwTexDictionaryTreeReader& tree) {
    RwTexDictionaryName lookup;
    names.InitializeName(
        lookup, static_cast<std::uint8_t>(
                    reinterpret_cast<std::uintptr_t>(name)));
    names.SetExceptionMessage(
        lookup, name, static_cast<std::uint32_t>(std::strlen(name)));
    const auto node = FindNodeForExists(state, lookup, names, tree);
    const bool exists = node != state.head &&
                        names.CompareNodeName(lookup, node + 12u) >= 0;
    (void)ReleaseName(lookup, names);
    return exists;
}

RwTexDictionaryInsertResult RwTexDictionaryTree::InsertInternal(
    RwTexDictionaryTreeState& state,
    const RwTexDictionaryEntry& entry,
    IRwTexDictionaryNodeInsertRuntime& runtime) {
    const auto head = state.head;
    const auto nil = runtime.SharedNil();
    auto parent = head;
    auto leaf = runtime.ReadWord(head + 4u);
    bool go_left = true;
    while (leaf != nil) {
        parent = leaf;
        go_left = runtime.CompareRange(
            entry.name, runtime.ReadWord(leaf + 16u),
            runtime.ReadWord(leaf + 20u)) < 0;
        leaf = runtime.ReadWord(leaf + (go_left ? 0u : 8u));
    }

    if (state.flag == 0u) {
        auto candidate = parent;
        if (go_left) {
            if (parent == runtime.ReadWord(head)) {
                return {InsertNodeAndRebalance(
                            state, leaf, parent, entry, runtime), true};
            }
            candidate = PreviousNode(parent, runtime);
        }
        if (runtime.CompareNodeToName(candidate + 12u, entry.name) >= 0) {
            return {candidate, false};
        }
        return {InsertNodeAndRebalance(
                    state, leaf, parent, entry, runtime), true};
    }

    return {InsertNodeAndRebalance(state, leaf, parent, entry, runtime),
            true};
}

std::uint32_t RwTexDictionaryTree::InsertNodeAndRebalance(
    RwTexDictionaryTreeState& state, const std::uint32_t leaf,
    const std::uint32_t parent, const RwTexDictionaryEntry& entry,
    IRwTexDictionaryNodeInsertRuntime& runtime) {
    const auto head = state.head;
    const auto nil = runtime.SharedNil();
    const auto created = CreateNode(parent, 0u, runtime);
    runtime.WriteWord(created, nil);
    runtime.WriteWord(created + 8u, nil);
    CopyNodeData(created + 12u, entry, runtime);
    ++state.count;
    const bool attach_left = parent == head || leaf != nil ||
        runtime.CompareNameToNode(entry.name, parent + 12u) < 0;
    if (attach_left) {
        runtime.WriteWord(parent, created);
        if (parent == head) {
            runtime.WriteWord(head + 4u, created);
            runtime.WriteWord(head + 8u, created);
        } else if (parent == runtime.ReadWord(head)) {
            runtime.WriteWord(head, created);
        }
    } else {
        runtime.WriteWord(parent + 8u, created);
        if (parent == runtime.ReadWord(head + 8u)) {
            runtime.WriteWord(head + 8u, created);
        }
    }

    auto current = created;
    while (current != runtime.ReadWord(head + 4u)) {
        auto parent_node = runtime.ReadWord(current + 4u);
        if (runtime.ReadWord(parent_node + 36u) != 0u) break;
        auto grandparent = runtime.ReadWord(parent_node + 4u);
        if (parent_node == runtime.ReadWord(grandparent)) {
            const auto uncle = runtime.ReadWord(grandparent + 8u);
            if (runtime.ReadWord(uncle + 36u) == 0u) {
                runtime.WriteWord(parent_node + 36u, 1u);
                runtime.WriteWord(uncle + 36u, 1u);
                runtime.WriteWord(grandparent + 36u, 0u);
                current = grandparent;
                continue;
            }
            if (current == runtime.ReadWord(parent_node + 8u)) {
                current = parent_node;
                (void)RotateLeft(state, parent_node, runtime);
            }
            parent_node = runtime.ReadWord(current + 4u);
            runtime.WriteWord(parent_node + 36u, 1u);
            grandparent = runtime.ReadWord(parent_node + 4u);
            runtime.WriteWord(grandparent + 36u, 0u);
            (void)RotateRight(state, grandparent, runtime);
        } else {
            const auto uncle = runtime.ReadWord(grandparent);
            if (runtime.ReadWord(uncle + 36u) == 0u) {
                runtime.WriteWord(parent_node + 36u, 1u);
                runtime.WriteWord(uncle + 36u, 1u);
                runtime.WriteWord(grandparent + 36u, 0u);
                current = grandparent;
                continue;
            }
            if (current == runtime.ReadWord(parent_node)) {
                current = parent_node;
                (void)RotateRight(state, parent_node, runtime);
            }
            parent_node = runtime.ReadWord(current + 4u);
            runtime.WriteWord(parent_node + 36u, 1u);
            grandparent = runtime.ReadWord(parent_node + 4u);
            runtime.WriteWord(grandparent + 36u, 0u);
            (void)RotateLeft(state, grandparent, runtime);
        }
    }
    runtime.WriteWord(runtime.ReadWord(head + 4u) + 36u, 1u);
    return created;
}

std::uint32_t RwTexDictionaryTree::RotateLeft(
    RwTexDictionaryTreeState& state, const std::uint32_t node,
    IRwTexDictionaryNodeInsertRuntime& runtime) {
    const auto replacement = runtime.ReadWord(node + 8u);
    const auto moved = runtime.ReadWord(replacement);
    runtime.WriteWord(node + 8u, moved);
    if (moved != runtime.SharedNil()) {
        runtime.WriteWord(moved + 4u, node);
    }
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(replacement + 4u, parent);
    if (node == runtime.ReadWord(state.head + 4u)) {
        runtime.WriteWord(state.head + 4u, replacement);
    } else if (node == runtime.ReadWord(parent)) {
        runtime.WriteWord(parent, replacement);
    } else {
        runtime.WriteWord(parent + 8u, replacement);
    }
    runtime.WriteWord(replacement, node);
    runtime.WriteWord(node + 4u, replacement);
    return replacement;
}

std::uint32_t RwTexDictionaryTree::RotateRight(
    RwTexDictionaryTreeState& state, const std::uint32_t node,
    IRwTexDictionaryNodeInsertRuntime& runtime) {
    const auto replacement = runtime.ReadWord(node);
    const auto moved = runtime.ReadWord(replacement + 8u);
    runtime.WriteWord(node, moved);
    if (moved != runtime.SharedNil()) {
        runtime.WriteWord(moved + 4u, node);
    }
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(replacement + 4u, parent);
    if (node == runtime.ReadWord(state.head + 4u)) {
        runtime.WriteWord(state.head + 4u, replacement);
    } else if (node == runtime.ReadWord(parent + 8u)) {
        runtime.WriteWord(parent + 8u, replacement);
    } else {
        runtime.WriteWord(parent, replacement);
    }
    runtime.WriteWord(replacement + 8u, node);
    runtime.WriteWord(node + 4u, replacement);
    return replacement;
}

std::uint32_t RwTexDictionaryTree::FindNodeForExists(
    RwTexDictionaryTreeState& state,
    const RwTexDictionaryName& name,
    IRwTexDictionaryInsertRuntime& names,
    IRwTexDictionaryTreeReader& tree) {
    auto candidate = state.head;
    auto node = tree.ReadWord(state.head + 4u);
    const auto nil = tree.SharedNil();
    while (node != nil) {
        const auto node_length = tree.ReadWord(node + 20u);
        const auto common_length = node_length < name.size ? node_length :
                                   name.size;
        const auto comparison = names.CompareBytes(
            tree.ReadWord(node + 16u), name.buffer, common_length);
        if (comparison < 0 ||
            (comparison == 0 && node_length < name.size)) {
            node = tree.ReadWord(node + 8u);
        } else {
            candidate = node;
            node = tree.ReadWord(node);
        }
    }
    return candidate;
}

std::uint32_t RwTexDictionaryTree::CreateNode(
    const std::uint32_t parent, const std::uint32_t color,
    IRwTexDictionaryNodeInsertRuntime& runtime) {
    const auto node = runtime.AllocateNode(40u);
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 36u, color);
    return node;
}

std::uint32_t RwTexDictionaryTree::PreviousNode(
    std::uint32_t node, IRwTexDictionaryTreeReader& tree) {
    const auto parent = tree.ReadWord(node + 4u);
    if (tree.ReadWord(node + 36u) == 0u &&
        tree.ReadWord(parent + 4u) == node) {
        return tree.ReadWord(node + 8u);
    }
    const auto nil = tree.SharedNil();
    auto left = tree.ReadWord(node);
    if (left != nil) {
        node = left;
        while (tree.ReadWord(node + 8u) != nil) {
            node = tree.ReadWord(node + 8u);
        }
        return node;
    }
    auto ancestor = parent;
    while (node == tree.ReadWord(ancestor)) {
        node = ancestor;
        ancestor = tree.ReadWord(ancestor + 4u);
    }
    return ancestor;
}

void RwTexDictionaryTree::CopyNodeData(
    const std::uint32_t destination, const RwTexDictionaryEntry& source,
    IRwTexDictionaryNodeInsertRuntime& runtime) {
    if (destination == 0u) return;
    runtime.WriteWord(destination + 4u, 0u);
    runtime.WriteByte(destination, source.name.tag);
    runtime.WriteWord(destination + 8u, 0u);
    runtime.WriteWord(destination + 12u, 0u);

    const auto length = source.name.size;
    const auto source_buffer = source.name.buffer != 0u
        ? source.name.buffer : runtime.EmptyNameBuffer();
    if (length != 0u &&
        runtime.ReadByte(source_buffer - 1u) < 0xfeu) {
        runtime.SetNameErrorFlag(destination, 1u);
        runtime.WriteWord(destination + 4u, source_buffer);
        runtime.WriteWord(destination + 8u, source.name.size);
        runtime.WriteWord(destination + 12u, source.name.capacity);
        const auto references = runtime.ReadByte(source_buffer - 1u);
        runtime.WriteByte(source_buffer - 1u,
                          static_cast<std::uint8_t>(references + 1u));
    } else if (runtime.AllocateNameBuffer(destination, length, true)) {
        const auto target_buffer = runtime.ReadWord(destination + 4u);
        runtime.CopyNameBytes(target_buffer, source_buffer, length);
        runtime.WriteWord(destination + 8u, length);
        runtime.WriteByte(target_buffer + length, 0u);
    }

    runtime.WriteWord(destination + 16u, source.dictionary);
    runtime.WriteWord(destination + 20u, source.references);
}

void RwTexDictionaryTree::CopyNodeData(
    const std::uint32_t destination, const std::uint32_t source,
    IRwTexDictionaryNodeInsertRuntime& runtime) {
    if (destination == 0u) return;
    if (destination == source) {
        const auto tag = runtime.ReadByte(source);
        runtime.WriteWord(destination + 4u, 0u);
        runtime.WriteByte(destination, tag);
        runtime.WriteWord(destination + 8u, 0u);
        runtime.WriteWord(destination + 12u, 0u);
        runtime.DetachNameBuffer(destination);
        runtime.DetachNameBuffer(destination);
        return;
    }
    RwTexDictionaryEntry entry;
    entry.name.tag = runtime.ReadByte(source);
    entry.name.buffer = runtime.ReadWord(source + 4u);
    entry.name.size = runtime.ReadWord(source + 8u);
    entry.name.capacity = runtime.ReadWord(source + 12u);
    entry.dictionary = runtime.ReadWord(source + 16u);
    entry.references = runtime.ReadWord(source + 20u);
    CopyNodeData(destination, entry, runtime);
}

std::uint32_t RwTexDictionaryTree::DestroyNode(
    const std::uint32_t payload, const std::uint32_t flags,
    IRwTexDictionaryTreeRuntime& runtime) {
    ReleaseNodeName(payload - 12u, runtime);
    if ((flags & 1u) != 0u) (void)runtime.Free(payload);
    return payload;
}

std::uint32_t RwTexDictionaryTree::RemoveNodeAndRebalance(
    RwTexDictionaryTreeState& state, const std::uint32_t node,
    IRwTexDictionaryTreeRuntime& runtime) {
    TextureRemovalAdapter adapter(state, runtime);
    return UiRedBlackTreeRemove::RemoveTextureNode(
        TextureRemovalAdapter::kTreeAddress, node, adapter);
}

std::uint32_t RwTexDictionaryTree::FindNode(
    RwTexDictionaryTreeState& state,
    const RwTexDictionaryName& name,
    IRwTexDictionaryInsertRuntime& names,
    IRwTexDictionaryTreeReader& tree) {
    const auto head = state.head;
    const auto nil = tree.SharedNil();
    auto candidate = head;
    auto node = tree.ReadWord(head + 4u);
    while (node != nil) {
        if (names.CompareNodeAgainstName(node + 12u, name) >= 0) {
            candidate = node;
            node = tree.ReadWord(node);
        } else {
            node = tree.ReadWord(node + 8u);
        }
    }
    if (candidate == head) return head;
    const auto node_length = tree.ReadWord(candidate + 20u);
    const auto common_length = name.size < node_length ? name.size :
                               node_length;
    const auto comparison = names.CompareBytes(
        name.buffer, tree.ReadWord(candidate + 16u), common_length);
    if (comparison < 0 ||
        (comparison == 0 && name.size < node_length)) {
        return head;
    }
    return candidate;
}

std::uint32_t RwTexDictionaryTree::NextNode(
    std::uint32_t node, IRwTexDictionaryTreeReader& tree) {
    const auto nil = tree.SharedNil();
    auto right = tree.ReadWord(node + 8u);
    if (right != nil) {
        node = right;
        while (tree.ReadWord(node) != nil) {
            node = tree.ReadWord(node);
        }
        return node;
    }

    auto parent = tree.ReadWord(node + 4u);
    while (node == tree.ReadWord(parent + 8u)) {
        node = parent;
        parent = tree.ReadWord(parent + 4u);
    }
    if (tree.ReadWord(node + 8u) != parent) node = parent;
    return node;
}

std::uint32_t RwTexDictionaryTree::FindNodeCandidate(
    RwTexDictionaryTreeState& state,
    const RwTexDictionaryName& name,
    IRwTexDictionaryInsertRuntime& names,
    IRwTexDictionaryTreeReader& tree) {
    auto candidate = state.head;
    auto node = tree.ReadWord(state.head + 4u);
    const auto nil = tree.SharedNil();
    while (node != nil) {
        if (names.CompareRange(tree.ReadWord(node + 20u),
                               name.buffer, name.size) >= 0) {
            candidate = node;
            node = tree.ReadWord(node);
        } else {
            node = tree.ReadWord(node + 8u);
        }
    }
    return candidate;
}

std::uint32_t RwTexDictionaryTree::DestroyRecursive(
    std::uint32_t root, IRwTexDictionaryTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    while (root != nil) {
        const auto right = runtime.ReadWord(root + 8u);
        (void)DestroyRecursive(right, runtime);
        const auto left = runtime.ReadWord(root);
        ReleaseNodeName(root, runtime);
        (void)runtime.Free(root);
        root = left;
    }
    return nil;
}

}  // namespace pal4
