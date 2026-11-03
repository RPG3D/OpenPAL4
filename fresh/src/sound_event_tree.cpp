#include "pal4/sound_event_tree.h"

namespace pal4 {

std::uint32_t SoundEventTree::AllocateNode(
    const std::uint32_t parent, const std::uint32_t colour,
    ISoundEventTreeRuntime& runtime) {
    const std::uint32_t node = runtime.Allocate(kNodeBytes);
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 180u, colour);
    return node;
}

std::uint32_t SoundEventTree::Construct(
    const std::uint32_t tree, ISoundEventTreeRuntime& runtime) {
    const std::uint32_t candidate = runtime.Allocate(kNodeBytes);
    runtime.WriteWord(candidate + 180u, 1u);
    runtime.WriteWord(candidate + 4u, 0u);
    std::uint32_t nil = runtime.ReadWord(kNilAddress);
    if (nil == 0u) {
        nil = candidate;
        runtime.WriteWord(kNilAddress, nil);
        runtime.WriteWord(nil, 0u);
        runtime.WriteWord(nil + 8u, 0u);
    } else {
        (void)runtime.Free(candidate);
    }
    runtime.WriteWord(kNilReferencesAddress,
                      runtime.ReadWord(kNilReferencesAddress) + 1u);
    const std::uint32_t head = runtime.Allocate(kNodeBytes);
    runtime.WriteWord(head + 4u, nil);
    runtime.WriteWord(head + 180u, 0u);
    runtime.WriteWord(tree + 4u, head);
    runtime.WriteWord(tree + 12u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    return head;
}

std::uint32_t SoundEventTree::RotateLeft(
    const std::uint32_t tree, const std::uint32_t node,
    ISoundEventTreeRuntime& runtime) {
    const std::uint32_t nil = runtime.ReadWord(kNilAddress);
    const std::uint32_t right = runtime.ReadWord(node + 8u);
    const std::uint32_t middle = runtime.ReadWord(right);
    runtime.WriteWord(node + 8u, middle);
    if (middle != nil) runtime.WriteWord(middle + 4u, node);
    const std::uint32_t parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(right + 4u, parent);
    const std::uint32_t head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u)) {
        runtime.WriteWord(head + 4u, right);
    } else if (node == runtime.ReadWord(parent)) {
        runtime.WriteWord(parent, right);
    } else {
        runtime.WriteWord(parent + 8u, right);
    }
    runtime.WriteWord(right, node);
    runtime.WriteWord(node + 4u, right);
    return right;
}

std::uint32_t SoundEventTree::RotateRight(
    const std::uint32_t tree, const std::uint32_t node,
    ISoundEventTreeRuntime& runtime) {
    const std::uint32_t nil = runtime.ReadWord(kNilAddress);
    const std::uint32_t left = runtime.ReadWord(node);
    const std::uint32_t middle = runtime.ReadWord(left + 8u);
    runtime.WriteWord(node, middle);
    if (middle != nil) runtime.WriteWord(middle + 4u, node);
    const std::uint32_t parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(left + 4u, parent);
    const std::uint32_t head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u)) {
        runtime.WriteWord(head + 4u, left);
    } else if (node == runtime.ReadWord(parent + 8u)) {
        runtime.WriteWord(parent + 8u, left);
    } else {
        runtime.WriteWord(parent, left);
    }
    runtime.WriteWord(left + 8u, node);
    runtime.WriteWord(node + 4u, left);
    return left;
}

std::uint32_t SoundEventTree::InsertCore(
    const std::uint32_t tree, const std::uint32_t child,
    const std::uint32_t parent, const std::uint32_t key,
    ISoundEventTreeRuntime& runtime) {
    const std::uint32_t nil = runtime.ReadWord(kNilAddress);
    const std::uint32_t node = AllocateNode(parent, 0u, runtime);
    runtime.WriteWord(node, nil);
    runtime.WriteWord(node + 8u, nil);
    runtime.CopyPayload(node + 12u, key);
    const std::uint32_t head = runtime.ReadWord(tree + 4u);
    runtime.WriteWord(tree + 12u,
                      runtime.ReadWord(tree + 12u) + 1u);
    if (parent == head || child != nil ||
        runtime.Less(key, parent + 12u)) {
        runtime.WriteWord(parent, node);
        if (parent == head) {
            runtime.WriteWord(head + 4u, node);
            runtime.WriteWord(head + 8u, node);
        } else if (parent == runtime.ReadWord(head)) {
            runtime.WriteWord(head, node);
        }
    } else {
        runtime.WriteWord(parent + 8u, node);
        if (parent == runtime.ReadWord(head + 8u))
            runtime.WriteWord(head + 8u, node);
    }

    std::uint32_t current = node;
    while (current != runtime.ReadWord(head + 4u)) {
        const std::uint32_t p = runtime.ReadWord(current + 4u);
        if (runtime.ReadWord(p + 180u) != 0u) break;
        const std::uint32_t grand = runtime.ReadWord(p + 4u);
        if (p == runtime.ReadWord(grand)) {
            const std::uint32_t uncle = runtime.ReadWord(grand + 8u);
            if (runtime.ReadWord(uncle + 180u) == 0u) {
                runtime.WriteWord(p + 180u, 1u);
                runtime.WriteWord(uncle + 180u, 1u);
                runtime.WriteWord(grand + 180u, 0u);
                current = grand;
            } else {
                if (current == runtime.ReadWord(p + 8u)) {
                    current = p;
                    (void)RotateLeft(tree, current, runtime);
                }
                const std::uint32_t new_parent =
                    runtime.ReadWord(current + 4u);
                const std::uint32_t new_grand =
                    runtime.ReadWord(new_parent + 4u);
                runtime.WriteWord(new_parent + 180u, 1u);
                runtime.WriteWord(new_grand + 180u, 0u);
                (void)RotateRight(tree, new_grand, runtime);
            }
        } else {
            const std::uint32_t uncle = runtime.ReadWord(grand);
            if (runtime.ReadWord(uncle + 180u) == 0u) {
                runtime.WriteWord(p + 180u, 1u);
                runtime.WriteWord(uncle + 180u, 1u);
                runtime.WriteWord(grand + 180u, 0u);
                current = grand;
            } else {
                if (current == runtime.ReadWord(p)) {
                    current = p;
                    (void)RotateRight(tree, current, runtime);
                }
                const std::uint32_t new_parent =
                    runtime.ReadWord(current + 4u);
                const std::uint32_t new_grand =
                    runtime.ReadWord(new_parent + 4u);
                runtime.WriteWord(new_parent + 180u, 1u);
                runtime.WriteWord(new_grand + 180u, 0u);
                (void)RotateLeft(tree, new_grand, runtime);
            }
        }
    }
    runtime.WriteWord(runtime.ReadWord(head + 4u) + 180u, 1u);
    return node;
}

std::uint32_t SoundEventTree::InsertNode(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t child, const std::uint32_t parent,
    const std::uint32_t key, ISoundEventTreeRuntime& runtime) {
    const std::uint32_t node =
        InsertCore(tree, child, parent, key, runtime);
    runtime.WriteWord(output, node);
    return output;
}

std::uint32_t SoundEventTree::Previous(
    const std::uint32_t cursor, ISoundEventTreeRuntime& runtime) {
    const std::uint32_t nil = runtime.ReadWord(kNilAddress);
    const std::uint32_t node = runtime.ReadWord(cursor);
    const std::uint32_t parent = runtime.ReadWord(node + 4u);
    if (runtime.ReadWord(node + 180u) == 0u &&
        runtime.ReadWord(parent + 4u) == node) {
        const std::uint32_t result = runtime.ReadWord(node + 8u);
        runtime.WriteWord(cursor, result);
        return result;
    }
    std::uint32_t left = runtime.ReadWord(node);
    if (left != nil) {
        std::uint32_t next = runtime.ReadWord(left + 8u);
        while (next != nil) {
            left = next;
            next = runtime.ReadWord(left + 8u);
        }
        runtime.WriteWord(cursor, left);
        return next;
    }
    std::uint32_t ancestor = parent;
    while (runtime.ReadWord(cursor) == runtime.ReadWord(ancestor)) {
        runtime.WriteWord(cursor, ancestor);
        ancestor = runtime.ReadWord(ancestor + 4u);
    }
    runtime.WriteWord(cursor, ancestor);
    return ancestor;
}

std::uint32_t SoundEventTree::InsertOrGet(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t key, ISoundEventTreeRuntime& runtime) {
    const std::uint32_t nil = runtime.ReadWord(kNilAddress);
    const std::uint32_t head = runtime.ReadWord(tree + 4u);
    std::uint32_t parent = head;
    std::uint32_t child = runtime.ReadWord(head + 4u);
    bool less = true;
    while (child != nil) {
        parent = child;
        less = runtime.Less(key, child + 12u);
        child = runtime.ReadWord(child + (less ? 0u : 8u));
    }

    std::uint32_t selected = parent;
    bool inserted = false;
    if (runtime.ReadByte(tree + 8u) != 0u) {
        selected = InsertCore(tree, child, parent, key, runtime);
        inserted = true;
    } else {
        if (less) {
            if (parent == runtime.ReadWord(head)) {
                selected = InsertCore(tree, child, parent, key, runtime);
                inserted = true;
            } else {
                runtime.WriteWord(output, parent);
                (void)Previous(output, runtime);
                selected = runtime.ReadWord(output);
            }
        }
        if (!inserted && runtime.Less(selected + 12u, key)) {
            selected = InsertCore(tree, child, parent, key, runtime);
            inserted = true;
        }
    }
    runtime.WriteWord(output, selected);
    runtime.WriteByte(output + 4u, inserted ? 1u : 0u);
    return output;
}

std::uint32_t SoundEventTree::LowerBound(
    const std::uint32_t tree, const std::uint32_t key,
    ISoundEventTreeRuntime& runtime) {
    const std::uint32_t nil = runtime.ReadWord(kNilAddress);
    const std::uint32_t head = runtime.ReadWord(tree + 4u);
    std::uint32_t candidate = head;
    std::uint32_t node = runtime.ReadWord(head + 4u);
    while (node != nil) {
        if (runtime.Less(node + 12u, key)) {
            node = runtime.ReadWord(node + 8u);
        } else {
            candidate = node;
            node = runtime.ReadWord(node);
        }
    }
    return candidate;
}

}  // namespace pal4
