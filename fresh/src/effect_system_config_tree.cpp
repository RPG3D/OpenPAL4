#include "pal4/effect_system_config_tree.h"
#include "pal4/utilities_text_record_copy.h"

namespace pal4 {

namespace {

class ConfigNodeAccess final {
public:
    ConfigNodeAccess(std::uint32_t tree,
                     IEffectSystemConfigNodeRuntime& runtime)
        : runtime_(runtime),
          head_(runtime.ReadWord(tree + 4u)), nil_(runtime.SharedNil()) {}

    [[nodiscard]] std::uint32_t Head() const { return head_; }
    [[nodiscard]] std::uint32_t Nil() const { return nil_; }
    [[nodiscard]] std::uint32_t Get(std::uint32_t node,
                                    std::uint32_t offset = 0u) const {
        return runtime_.ReadWord(node + offset);
    }
    void Set(std::uint32_t node, std::uint32_t offset,
             std::uint32_t value) const {
        runtime_.WriteWord(node + offset, value);
    }
    [[nodiscard]] std::uint32_t Parent(std::uint32_t node) const {
        return Get(node, 4u);
    }
    [[nodiscard]] std::uint32_t Left(std::uint32_t node) const {
        return Get(node);
    }
    [[nodiscard]] std::uint32_t Right(std::uint32_t node) const {
        return Get(node, 8u);
    }
    [[nodiscard]] std::uint32_t Root() const { return Parent(head_); }
    [[nodiscard]] std::uint32_t Color(std::uint32_t node) const {
        return Get(node, 76u);
    }
    void SetColor(std::uint32_t node, std::uint32_t color) const {
        Set(node, 76u, color);
    }

    [[nodiscard]] std::uint32_t Minimum(std::uint32_t node) const {
        while (Left(node) != nil_) node = Left(node);
        return node;
    }
    [[nodiscard]] std::uint32_t Maximum(std::uint32_t node) const {
        while (Right(node) != nil_) node = Right(node);
        return node;
    }
    void ReplaceLink(std::uint32_t old_node,
                     std::uint32_t replacement) const {
        const auto parent = Parent(old_node);
        if (old_node == Root())
            Set(head_, 4u, replacement);
        else if (old_node == Left(parent))
            Set(parent, 0u, replacement);
        else
            Set(parent, 8u, replacement);
    }

    void RotateLeft(std::uint32_t node) const {
        const auto right = Right(node);
        Set(node, 8u, Left(right));
        if (Left(right) != nil_) Set(Left(right), 4u, node);
        const auto parent = Parent(node);
        Set(right, 4u, parent);
        if (node == Root())
            Set(head_, 4u, right);
        else if (node == Left(parent))
            Set(parent, 0u, right);
        else
            Set(parent, 8u, right);
        Set(right, 0u, node);
        Set(node, 4u, right);
    }

    void RotateRight(std::uint32_t node) const {
        const auto left = Left(node);
        Set(node, 0u, Right(left));
        if (Right(left) != nil_) Set(Right(left), 4u, node);
        const auto parent = Parent(node);
        Set(left, 4u, parent);
        if (node == Root())
            Set(head_, 4u, left);
        else if (node == Right(parent))
            Set(parent, 8u, left);
        else
            Set(parent, 0u, left);
        Set(left, 8u, node);
        Set(node, 4u, left);
    }

    void RepairAfterErase(std::uint32_t child,
                          std::uint32_t parent) const {
        while (child != Root() && Color(child) == 1u) {
            if (child == Left(parent)) {
                auto sibling = Right(parent);
                if (Color(sibling) == 0u) {
                    SetColor(sibling, 1u);
                    SetColor(parent, 0u);
                    RotateLeft(parent);
                    sibling = Right(parent);
                }
                if (Color(Left(sibling)) == 1u &&
                    Color(Right(sibling)) == 1u) {
                    SetColor(sibling, 0u);
                    child = parent;
                    parent = Parent(child);
                } else {
                    if (Color(Right(sibling)) == 1u) {
                        SetColor(Left(sibling), 1u);
                        SetColor(sibling, 0u);
                        RotateRight(sibling);
                        sibling = Right(parent);
                    }
                    SetColor(sibling, Color(parent));
                    SetColor(parent, 1u);
                    SetColor(Right(sibling), 1u);
                    RotateLeft(parent);
                    break;
                }
            } else {
                auto sibling = Left(parent);
                if (Color(sibling) == 0u) {
                    SetColor(sibling, 1u);
                    SetColor(parent, 0u);
                    RotateRight(parent);
                    sibling = Left(parent);
                }
                if (Color(Right(sibling)) == 1u &&
                    Color(Left(sibling)) == 1u) {
                    SetColor(sibling, 0u);
                    child = parent;
                    parent = Parent(child);
                } else {
                    if (Color(Left(sibling)) == 1u) {
                        SetColor(Right(sibling), 1u);
                        SetColor(sibling, 0u);
                        RotateLeft(sibling);
                        sibling = Left(parent);
                    }
                    SetColor(sibling, Color(parent));
                    SetColor(parent, 1u);
                    SetColor(Left(sibling), 1u);
                    RotateRight(parent);
                    break;
                }
            }
        }
        SetColor(child, 1u);
    }

private:
    IEffectSystemConfigNodeRuntime& runtime_;
    std::uint32_t head_;
    std::uint32_t nil_;
};

}  // namespace

std::uint8_t EffectSystemConfigTree::InsertIfAbsent(
    const std::uint32_t owner, const std::uint32_t config,
    IEffectSystemConfigTreeRuntime& runtime) {
    const auto tree = owner + 16u;
    const auto header = runtime.ReadWord(owner + 20u);
    const auto key = config + 16u;
    const auto candidate = runtime.FindLowerBound(tree, key);
    if (candidate != header &&
        runtime.CompareText(key, candidate + 12u) >= 0)
        return 0u;

    const auto key_copy = runtime.CopyKey(key);
    const auto config_copy = runtime.CopyConfig(config);
    runtime.CopyNameField(config_copy, key);
    runtime.CopyTailField(config_copy, config + 32u);
    const auto inserted = runtime.Insert(tree, key_copy, config_copy);
    runtime.DestroyConfigCopy(config_copy);
    runtime.ReleaseKeyCopy(key_copy);
    return inserted;
}

std::uint32_t EffectSystemConfigTree::CleanupValue(
    const std::uint32_t value,
    IEffectSystemConfigCleanupRuntime& runtime) {
    runtime.DestroyConfig(value + 16u);
    auto result = runtime.ReadWord(value + 4u);
    if (result != 0u) {
        const auto reference = result - 1u;
        const auto count = runtime.ReadByte(reference);
        if (count == 0u || count == 0xffu)
            result = runtime.Free(reference);
        else {
            runtime.WriteByte(reference,
                              static_cast<std::uint8_t>(count - 1u));
            result = reference;
        }
    }
    runtime.WriteWord(value + 4u, 0u);
    runtime.WriteWord(value + 8u, 0u);
    runtime.WriteWord(value + 12u, 0u);
    return result;
}

std::uint32_t EffectSystemConfigTree::DeleteValue(
    const std::uint32_t value, const std::uint8_t flags,
    IEffectSystemConfigCleanupRuntime& runtime) {
    (void)CleanupValue(value, runtime);
    if ((flags & 1u) != 0u) (void)runtime.Free(value);
    return value;
}

std::uint32_t EffectSystemConfigTree::CleanupTextField(
    const std::uint32_t value,
    IEffectSystemConfigCleanupRuntime& runtime) {
    auto result = runtime.ReadWord(value + 8u);
    if (result != 0u) {
        const auto reference = result - 1u;
        const auto count = runtime.ReadByte(reference);
        if (count == 0u || count == 0xffu)
            result = runtime.Free(reference);
        else {
            runtime.WriteByte(reference,
                              static_cast<std::uint8_t>(count - 1u));
            result = reference;
        }
    }
    runtime.WriteWord(value + 8u, 0u);
    runtime.WriteWord(value + 12u, 0u);
    runtime.WriteWord(value + 16u, 0u);
    return result;
}

std::uint32_t EffectSystemConfigTree::FindProperty(
    const std::uint32_t owner, const char* name,
    IEffectSystemConfigLookupRuntime& runtime) {
    const std::string_view query(name);
    const auto head = runtime.ReadWord(owner + 20u);
    const auto candidate = FindLowerBoundNode(owner + 16u, query, runtime);
    if (candidate == head ||
        runtime.CompareText(query, candidate + 12u) < 0)
        return 0u;
    return candidate + 28u;
}

std::uint32_t EffectSystemConfigTree::FirstProperty(
    const std::uint32_t owner, std::uint32_t& cursor,
    IEffectSystemConfigLookupRuntime& runtime) {
    cursor = 0u;
    if (runtime.ReadWord(owner + 28u) == 0u) return 0u;
    const auto head = runtime.ReadWord(owner + 20u);
    return runtime.ReadWord(head) + 28u;
}

std::uint32_t EffectSystemConfigTree::NextProperty(
    const std::uint32_t owner, std::uint32_t& cursor,
    IEffectSystemConfigLookupRuntime& runtime) {
    const auto index = ++cursor;
    if (runtime.ReadWord(owner + 28u) <= index) return 0u;
    const auto head = runtime.ReadWord(owner + 20u);
    auto node = runtime.ReadWord(head);
    for (auto remaining = index; remaining != 0u; --remaining) {
        const auto right = runtime.ReadWord(node + 8u);
        if (right != runtime.SharedNil()) {
            node = right;
            for (auto left = runtime.ReadWord(node);
                 left != runtime.SharedNil();
                 left = runtime.ReadWord(node))
                node = left;
        } else {
            auto parent = runtime.ReadWord(node + 4u);
            while (node == runtime.ReadWord(parent + 8u)) {
                node = parent;
                parent = runtime.ReadWord(parent + 4u);
            }
            if (runtime.ReadWord(node + 8u) != parent) node = parent;
        }
    }
    return node + 28u;
}

EffectSystemConfigInsertResult EffectSystemConfigTree::InsertElement(
    const std::uint32_t tree_address, const std::uint32_t payload,
    IEffectSystemConfigNodeRuntime& runtime) {
    ConfigNodeAccess tree(tree_address, runtime);
    const auto head = tree.Head();
    const auto nil = tree.Nil();
    auto parent = head;
    auto node = tree.Root();
    bool less = true;
    while (node != nil) {
        parent = node;
        less = runtime.CompareSearch(payload, node + 12u) < 0;
        node = less ? tree.Left(node) : tree.Right(node);
    }

    if (runtime.ReadByte(tree_address + 8u) == 0u) {
        auto candidate = parent;
        if (less) {
            if (parent != tree.Left(head))
                (void)AdvanceNodeCursorBackward(candidate, runtime);
            else
                candidate = head;
        }
        if (candidate != head &&
            runtime.CompareText(candidate + 12u, payload) >= 0)
            return {candidate, false};
    }

    return {InsertNodeAt(tree_address, node, parent, payload, runtime), true};
}

std::uint32_t EffectSystemConfigTree::InsertNodeAt(
    const std::uint32_t tree_address, const std::uint32_t child,
    const std::uint32_t parent, const std::uint32_t payload,
    IEffectSystemConfigNodeRuntime& runtime) {
    ConfigNodeAccess tree(tree_address, runtime);
    const auto head = tree.Head();
    const auto nil = tree.Nil();

    const auto inserted = CreateNode(parent, 0u, runtime);
    tree.Set(inserted, 0u, nil);
    tree.Set(inserted, 8u, nil);
    runtime.CopyPayload(inserted + 12u, payload);
    tree.Set(tree_address, 12u, tree.Get(tree_address, 12u) + 1u);

    if (parent == head || child != nil ||
        runtime.CompareText(payload, parent + 12u) < 0) {
        tree.Set(parent, 0u, inserted);
        if (parent == head) {
            tree.Set(head, 4u, inserted);
            tree.Set(head, 8u, inserted);
        } else if (parent == tree.Left(head)) {
            tree.Set(head, 0u, inserted);
        }
    } else {
        tree.Set(parent, 8u, inserted);
        if (parent == tree.Right(head)) tree.Set(head, 8u, inserted);
    }

    auto current = inserted;
    while (current != tree.Root()) {
        const auto current_parent = tree.Parent(current);
        if (tree.Color(current_parent) != 0u) break;
        const auto grandparent = tree.Parent(current_parent);
        if (current_parent == tree.Left(grandparent)) {
            const auto uncle = tree.Right(grandparent);
            if (tree.Color(uncle) == 0u) {
                tree.SetColor(current_parent, 1u);
                tree.SetColor(uncle, 1u);
                tree.SetColor(grandparent, 0u);
                current = grandparent;
            } else {
                if (current == tree.Right(current_parent)) {
                    current = current_parent;
                    tree.RotateLeft(current);
                }
                const auto next_parent = tree.Parent(current);
                const auto next_grandparent = tree.Parent(next_parent);
                tree.SetColor(next_parent, 1u);
                tree.SetColor(next_grandparent, 0u);
                tree.RotateRight(next_grandparent);
            }
        } else {
            const auto uncle = tree.Left(grandparent);
            if (tree.Color(uncle) == 0u) {
                tree.SetColor(current_parent, 1u);
                tree.SetColor(uncle, 1u);
                tree.SetColor(grandparent, 0u);
                current = grandparent;
            } else {
                if (current == tree.Left(current_parent)) {
                    current = current_parent;
                    tree.RotateRight(current);
                }
                const auto next_parent = tree.Parent(current);
                const auto next_grandparent = tree.Parent(next_parent);
                tree.SetColor(next_parent, 1u);
                tree.SetColor(next_grandparent, 0u);
                tree.RotateLeft(next_grandparent);
            }
        }
    }
    tree.SetColor(tree.Root(), 1u);
    return inserted;
}

std::uint32_t EffectSystemConfigTree::EraseElement(
    const std::uint32_t tree_address, const std::uint32_t node,
    IEffectSystemConfigNodeRuntime& nodes,
    IEffectSystemConfigCleanupRuntime& cleanup) {
    ConfigNodeAccess tree(tree_address, nodes);
    const auto head = tree.Head();
    const auto nil = tree.Nil();
    auto successor = node;
    (void)AdvanceNodeCursor(successor, nodes);
    const auto left = tree.Left(node);
    const auto right = tree.Right(node);
    auto removed = node;
    std::uint32_t child;
    if (left == nil)
        child = right;
    else if (right == nil)
        child = left;
    else {
        removed = tree.Minimum(right);
        child = tree.Right(removed);
    }
    const auto removed_black = tree.Color(removed) == 1u;

    std::uint32_t child_parent;
    if (removed != node) {
        tree.Set(left, 4u, removed);
        tree.Set(removed, 0u, left);
        if (removed == right) {
            child_parent = removed;
            tree.Set(child, 4u, removed);
        } else {
            child_parent = tree.Parent(removed);
            tree.Set(child, 4u, child_parent);
            tree.Set(child_parent, 0u, child);
            tree.Set(removed, 8u, right);
            tree.Set(right, 4u, removed);
        }
        const auto parent = tree.Parent(node);
        tree.ReplaceLink(node, removed);
        tree.Set(removed, 4u, parent);
        const auto color = tree.Color(removed);
        tree.SetColor(removed, tree.Color(node));
        tree.SetColor(node, color);
    } else {
        child_parent = tree.Parent(node);
        tree.Set(child, 4u, child_parent);
        tree.ReplaceLink(node, child);
        if (node == tree.Left(head))
            tree.Set(head, 0u, right == nil ? child_parent
                                           : tree.Minimum(child));
        if (node == tree.Right(head))
            tree.Set(head, 8u, left == nil ? child_parent
                                          : tree.Maximum(child));
    }

    if (removed_black)
        tree.RepairAfterErase(child, child_parent);
    (void)CleanupValue(node + 12u, cleanup);
    nodes.FreeNode(node);
    tree.Set(tree_address, 12u, tree.Get(tree_address, 12u) - 1u);
    return successor;
}

std::uint32_t EffectSystemConfigTree::AdvanceNodeCursor(
    std::uint32_t& cursor, IEffectSystemConfigNodeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    auto right = runtime.ReadWord(cursor + 8u);
    if (right != nil) {
        auto result = runtime.ReadWord(right);
        while (result != nil) {
            right = result;
            result = runtime.ReadWord(right);
        }
        cursor = right;
        return result;
    }
    auto result = runtime.ReadWord(cursor + 4u);
    while (cursor == runtime.ReadWord(result + 8u)) {
        cursor = result;
        result = runtime.ReadWord(result + 4u);
    }
    if (runtime.ReadWord(cursor + 8u) != result) cursor = result;
    return result;
}

std::uint32_t EffectSystemConfigTree::FindLowerBoundNode(
    const std::uint32_t tree, const std::string_view name,
    IEffectSystemConfigLookupRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto candidate = head;
    auto node = runtime.ReadWord(head + 4u);
    const auto nil = runtime.SharedNil();
    while (node != nil) {
        if (runtime.CompareNodeToName(node + 12u, name) >= 0) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    return candidate;
}

std::uint32_t EffectSystemConfigTree::DestroySubtree(
    std::uint32_t root, IEffectSystemConfigNodeRuntime& nodes,
    IEffectSystemConfigCleanupRuntime& cleanup) {
    const auto nil = nodes.SharedNil();
    while (root != nil) {
        (void)DestroySubtree(nodes.ReadWord(root + 8u), nodes, cleanup);
        const auto left = nodes.ReadWord(root);
        (void)CleanupValue(root + 12u, cleanup);
        nodes.FreeNode(root);
        root = left;
    }
    return nil;
}

void EffectSystemConfigTree::RotateLeft(
    const std::uint32_t tree_address, const std::uint32_t node,
    IEffectSystemConfigNodeRuntime& runtime) {
    ConfigNodeAccess(tree_address, runtime).RotateLeft(node);
}

void EffectSystemConfigTree::RotateRight(
    const std::uint32_t tree_address, const std::uint32_t node,
    IEffectSystemConfigNodeRuntime& runtime) {
    ConfigNodeAccess(tree_address, runtime).RotateRight(node);
}

std::uint32_t& EffectSystemConfigTree::AdvanceNodeCursorPost(
    std::uint32_t& cursor, std::uint32_t& old,
    IEffectSystemConfigNodeRuntime& runtime) {
    const auto previous = cursor;
    (void)AdvanceNodeCursor(cursor, runtime);
    old = previous;
    return old;
}

std::uint32_t EffectSystemConfigTree::FindLowerBoundPayload(
    const std::uint32_t tree_address, const std::uint32_t payload,
    IEffectSystemConfigNodeRuntime& runtime) {
    ConfigNodeAccess tree(tree_address, runtime);
    auto candidate = tree.Head();
    auto node = tree.Root();
    while (node != tree.Nil()) {
        if (runtime.CompareText(node + 12u, payload) >= 0) {
            candidate = node;
            node = tree.Left(node);
        } else {
            node = tree.Right(node);
        }
    }
    return candidate;
}

std::uint32_t EffectSystemConfigTree::CreateNode(
    const std::uint32_t parent, const std::uint32_t color,
    IEffectSystemConfigNodeRuntime& runtime) {
    const auto node = runtime.AllocateNode(80u);
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 76u, color);
    return node;
}

std::uint32_t EffectSystemConfigTree::AdvanceNodeCursorBackward(
    std::uint32_t& cursor, IEffectSystemConfigNodeRuntime& runtime) {
    const auto node = cursor;
    if (runtime.ReadWord(node + 76u) == 0u &&
        runtime.ReadWord(runtime.ReadWord(node + 4u) + 4u) == node) {
        const auto maximum = runtime.ReadWord(node + 8u);
        cursor = maximum;
        return maximum;
    }
    const auto nil = runtime.SharedNil();
    auto left = runtime.ReadWord(node);
    if (left != nil) {
        auto result = runtime.ReadWord(left + 8u);
        while (result != nil) {
            left = result;
            result = runtime.ReadWord(left + 8u);
        }
        cursor = left;
        return result;
    }
    auto result = runtime.ReadWord(node + 4u);
    while (cursor == runtime.ReadWord(result)) {
        cursor = result;
        result = runtime.ReadWord(result + 4u);
    }
    cursor = result;
    return result;
}

void EffectSystemConfigTree::CopyCompositePayload(
    const std::uint32_t destination, const std::uint32_t source,
    IEffectSystemCompositeCopyRuntime& runtime) {
    if (destination == 0u) return;
    runtime.WriteByte(destination, runtime.ReadByte(source));
    for (const auto offset : {4u, 8u, 12u})
        runtime.WriteWord(destination + offset, 0u);
    runtime.AssignText(destination, source);

    runtime.WriteByte(destination + 16u,
                      runtime.ReadByte(source + 16u));
    const auto begin = runtime.ReadWord(source + 20u);
    const auto end = runtime.ReadWord(source + 24u);
    const auto count = begin == 0u || end < begin
                           ? 0u : (end - begin) / 24u;
    const auto copied = runtime.Allocate(count * 24u);
    runtime.WriteWord(destination + 20u, copied);
    auto output = copied;
    for (auto input = begin; input != end; input += 24u) {
        runtime.CopyRecord24(output, input);
        output += 24u;
    }
    runtime.WriteWord(destination + 24u, output);
    runtime.WriteWord(destination + 28u, output);

    runtime.WriteByte(destination + 32u,
                      runtime.ReadByte(source + 32u));
    runtime.SetErrorFlag(destination + 32u, 0u);
    runtime.AssignText(destination + 32u, source + 32u);
    runtime.WriteByte(destination + 48u,
                      runtime.ReadByte(source + 48u));
    for (const auto offset : {52u, 56u, 60u})
        runtime.WriteWord(destination + offset, 0u);
    runtime.AssignText(destination + 48u, source + 48u);
}

int EffectSystemRecordBuffer::CleanupText(
    EffectSystemRecordBufferState& state,
    IEffectSystemRecordBufferRuntime& runtime) {
    for (auto record = state.begin; record != state.end; record += 24u)
        runtime.ReleaseText(record);
    runtime.Free(state.begin);
    state.begin = state.end = state.capacity = 0u;
    return 0;
}

int EffectSystemRecordBuffer::CleanupProperties(
    EffectSystemRecordBufferState& state,
    IEffectSystemRecordBufferRuntime& runtime) {
    for (auto record = state.begin; record != state.end; record += 24u)
        runtime.CleanupProperty(record);
    runtime.Free(state.begin);
    state.begin = state.end = state.capacity = 0u;
    return 0;
}

EffectSystemRecordBufferState& EffectSystemRecordBuffer::Copy(
    EffectSystemRecordBufferState& destination,
    const EffectSystemRecordBufferState& source,
    IEffectSystemRecordBufferRuntime& runtime) {
    destination.flag = source.flag;
    const auto count = source.begin == 0u || source.end < source.begin
                           ? 0u
                           : (source.end - source.begin) / 24u;
    const auto begin = runtime.Allocate(24u * count);
    destination.begin = begin;
    auto output = begin;
    for (auto input = source.begin; input != source.end; input += 24u) {
        runtime.CopyRecord(output, input);
        output += 24u;
    }
    destination.end = destination.capacity = output;
    return destination;
}

void EffectSystemRecordBuffer::CopyRecord24(
    const std::uint32_t destination, const std::uint32_t source,
    IUtilitiesTextRecordCopyRuntime& runtime) {
    if (destination == 0u) return;
    UtilitiesTextRecordCopy::CopyConstruct(destination, source, runtime);
    runtime.WriteWord(destination + 20u,
                      runtime.ReadWord(source + 20u));
}

}  // namespace pal4
