#include "pal4/ui_event_tree.h"

namespace pal4 {
namespace {

void ResetHeader(const std::uint32_t tree, const std::uint32_t header,
                 IUiEventTreeRuntime& runtime) {
    runtime.WriteWord(header + 4u, runtime.SharedNil());
    runtime.WriteWord(tree + 0xcu, 0u);
    runtime.WriteWord(header, header);
    runtime.WriteWord(header + 8u, header);
    (void)runtime.FirstElement(tree);
}

std::uint32_t ReleaseShared(const std::uint32_t tree,
                            IUiEventTreeRuntime& runtime) {
    (void)runtime.Free(runtime.ReadWord(tree + 4u));
    runtime.WriteWord(tree + 4u, 0u);
    runtime.WriteWord(tree + 0xcu, 0u);
    const auto remaining = runtime.ReferenceCount() - 1u;
    runtime.SetReferenceCount(remaining);
    if (remaining != 0u) return remaining;

    const auto nil = runtime.SharedNil();
    runtime.SetSharedNil(0u);
    return nil == 0u ? 0u : runtime.Free(nil);
}

void EraseAltChain(const std::uint32_t tree, const std::uint32_t header,
                   std::uint32_t node, IUiEventTreeRuntime& runtime) {
    while (node != header) {
        auto next = node;
        UiEventTree::SuccessorAlt(next, runtime);
        (void)UiEventTree::Erase(tree, node, runtime);
        node = next;
    }
}

void EraseChain(const std::uint32_t tree, const std::uint32_t header,
                std::uint32_t node, IUiEventTreeRuntime& runtime) {
    while (node != header) {
        const auto current = UiEventTree::Successor(node, runtime);
        (void)UiEventTree::Erase(tree, current, runtime);
    }
}

std::uint32_t Minimum(std::uint32_t node, const std::uint32_t nil,
                      IUiEventTreeRuntime& runtime) {
    while (runtime.ReadWord(node) != nil) node = runtime.ReadWord(node);
    return node;
}

std::uint32_t Maximum(std::uint32_t node, const std::uint32_t nil,
                      IUiEventTreeRuntime& runtime) {
    while (runtime.ReadWord(node + 8u) != nil)
        node = runtime.ReadWord(node + 8u);
    return node;
}

void ReplaceParentLink(const std::uint32_t header,
                       const std::uint32_t old_node,
                       const std::uint32_t new_node,
                       IUiEventTreeRuntime& runtime) {
    if (runtime.ReadWord(header + 4u) == old_node) {
        runtime.WriteWord(header + 4u, new_node);
        return;
    }
    const auto parent = runtime.ReadWord(old_node + 4u);
    const auto offset = runtime.ReadWord(parent) == old_node ? 0u : 8u;
    runtime.WriteWord(parent + offset, new_node);
}

void RestoreBlackHeight(const std::uint32_t tree,
                        const std::uint32_t header,
                        std::uint32_t node,
                        IUiEventTreeRuntime& runtime) {
    while (node != runtime.ReadWord(header + 4u) &&
           runtime.ReadWord(node + 0x14u) == 1u) {
        const auto parent = runtime.ReadWord(node + 4u);
        if (node == runtime.ReadWord(parent)) {
            auto sibling = runtime.ReadWord(parent + 8u);
            if (runtime.ReadWord(sibling + 0x14u) == 0u) {
                runtime.WriteWord(sibling + 0x14u, 1u);
                runtime.WriteWord(parent + 0x14u, 0u);
                (void)UiEventTree::RotateRight(tree, parent, runtime);
                sibling = runtime.ReadWord(parent + 8u);
            }
            const auto near = runtime.ReadWord(sibling);
            const auto far = runtime.ReadWord(sibling + 8u);
            if (runtime.ReadWord(near + 0x14u) == 1u &&
                runtime.ReadWord(far + 0x14u) == 1u) {
                runtime.WriteWord(sibling + 0x14u, 0u);
                node = parent;
                continue;
            }
            if (runtime.ReadWord(far + 0x14u) == 1u) {
                runtime.WriteWord(near + 0x14u, 1u);
                runtime.WriteWord(sibling + 0x14u, 0u);
                (void)UiEventTree::RotateLeft(tree, sibling, runtime);
                sibling = runtime.ReadWord(parent + 8u);
            }
            runtime.WriteWord(sibling + 0x14u,
                              runtime.ReadWord(parent + 0x14u));
            runtime.WriteWord(parent + 0x14u, 1u);
            runtime.WriteWord(runtime.ReadWord(sibling + 8u) + 0x14u, 1u);
            (void)UiEventTree::RotateRight(tree, parent, runtime);
            break;
        }

        auto sibling = runtime.ReadWord(parent);
        if (runtime.ReadWord(sibling + 0x14u) == 0u) {
            runtime.WriteWord(sibling + 0x14u, 1u);
            runtime.WriteWord(parent + 0x14u, 0u);
            (void)UiEventTree::RotateLeft(tree, parent, runtime);
            sibling = runtime.ReadWord(parent);
        }
        const auto near = runtime.ReadWord(sibling + 8u);
        const auto far = runtime.ReadWord(sibling);
        if (runtime.ReadWord(near + 0x14u) == 1u &&
            runtime.ReadWord(far + 0x14u) == 1u) {
            runtime.WriteWord(sibling + 0x14u, 0u);
            node = parent;
            continue;
        }
        if (runtime.ReadWord(far + 0x14u) == 1u) {
            runtime.WriteWord(near + 0x14u, 1u);
            runtime.WriteWord(sibling + 0x14u, 0u);
            (void)UiEventTree::RotateRight(tree, sibling, runtime);
            sibling = runtime.ReadWord(parent);
        }
        runtime.WriteWord(sibling + 0x14u,
                          runtime.ReadWord(parent + 0x14u));
        runtime.WriteWord(parent + 0x14u, 1u);
        runtime.WriteWord(runtime.ReadWord(sibling) + 0x14u, 1u);
        (void)UiEventTree::RotateLeft(tree, parent, runtime);
        break;
    }
    runtime.WriteWord(node + 0x14u, 1u);
}

std::uint32_t LinkAndBalance(const std::uint32_t tree,
                             const std::uint32_t child,
                             const std::uint32_t parent,
                             const std::uint32_t key,
                             const std::uint32_t node,
                             IUiEventTreeRuntime& runtime) {
    const auto header = runtime.ReadWord(tree + 4u);
    const auto nil = runtime.SharedNil();
    runtime.WriteWord(tree + 0xcu, runtime.ReadWord(tree + 0xcu) + 1u);
    if (parent == header || child != nil ||
        key < runtime.ReadWord(parent + 0xcu)) {
        runtime.WriteWord(parent, node);
        if (parent == header) {
            runtime.WriteWord(header + 4u, node);
            runtime.WriteWord(header + 8u, node);
        } else if (parent == runtime.ReadWord(header)) {
            runtime.WriteWord(header, node);
        }
    } else {
        runtime.WriteWord(parent + 8u, node);
        if (parent == runtime.ReadWord(header + 8u))
            runtime.WriteWord(header + 8u, node);
    }

    auto current = node;
    while (current != runtime.ReadWord(header + 4u)) {
        const auto parent_node = runtime.ReadWord(current + 4u);
        if (runtime.ReadWord(parent_node + 0x14u) != 0u) break;
        const auto grandparent = runtime.ReadWord(parent_node + 4u);
        if (parent_node == runtime.ReadWord(grandparent)) {
            const auto uncle = runtime.ReadWord(grandparent + 8u);
            if (runtime.ReadWord(uncle + 0x14u) == 0u) {
                runtime.WriteWord(parent_node + 0x14u, 1u);
                runtime.WriteWord(uncle + 0x14u, 1u);
                runtime.WriteWord(grandparent + 0x14u, 0u);
                current = grandparent;
            } else {
                if (current == runtime.ReadWord(parent_node + 8u)) {
                    current = parent_node;
                    (void)UiEventTree::RotateRight(tree, parent_node, runtime);
                }
                const auto raised = runtime.ReadWord(current + 4u);
                const auto old_root = runtime.ReadWord(raised + 4u);
                runtime.WriteWord(raised + 0x14u, 1u);
                runtime.WriteWord(old_root + 0x14u, 0u);
                (void)UiEventTree::RotateLeft(tree, old_root, runtime);
            }
        } else {
            const auto uncle = runtime.ReadWord(grandparent);
            if (runtime.ReadWord(uncle + 0x14u) == 0u) {
                runtime.WriteWord(parent_node + 0x14u, 1u);
                runtime.WriteWord(uncle + 0x14u, 1u);
                runtime.WriteWord(grandparent + 0x14u, 0u);
                current = grandparent;
            } else {
                if (current == runtime.ReadWord(parent_node)) {
                    current = parent_node;
                    (void)UiEventTree::RotateLeft(tree, parent_node, runtime);
                }
                const auto raised = runtime.ReadWord(current + 4u);
                const auto old_root = runtime.ReadWord(raised + 4u);
                runtime.WriteWord(raised + 0x14u, 1u);
                runtime.WriteWord(old_root + 0x14u, 0u);
                (void)UiEventTree::RotateRight(tree, old_root, runtime);
            }
        }
    }
    runtime.WriteWord(runtime.ReadWord(header + 4u) + 0x14u, 1u);
    return node;
}

}  // namespace

std::uint32_t UiEventTree::Construct(
    const std::uint32_t tree, IUiEventTreeRuntime& runtime) {
    runtime.WriteByte(tree + 8u, 0u);
    const auto candidate = runtime.Allocate(0x18u);
    runtime.WriteWord(candidate + 4u, 0u);
    runtime.WriteWord(candidate + 0x14u, 1u);
    const auto reused_nil = runtime.SharedNil() != 0u;
    if (!reused_nil) {
        runtime.SetSharedNil(candidate);
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    }
    runtime.SetReferenceCount(runtime.ReferenceCount() + 1u);
    if (reused_nil) (void)runtime.Free(candidate);

    const auto header = runtime.Allocate(0x18u);
    runtime.WriteWord(header + 4u, runtime.SharedNil());
    runtime.WriteWord(header + 0x14u, 0u);
    runtime.WriteWord(tree + 4u, header);
    runtime.WriteWord(tree + 0xcu, 0u);
    runtime.WriteWord(header, header);
    runtime.WriteWord(header + 8u, header);
    return tree;
}

std::uint32_t UiEventTree::DestroyAlt(
    const std::uint32_t tree, IUiEventTreeRuntime& runtime) {
    auto header = runtime.ReadWord(tree + 4u);
    auto first = runtime.ReadWord(header);
    if (runtime.ReadWord(tree + 0xcu) != 0u) {
        for (auto node = runtime.ReadWord(header + 4u);
             node != runtime.SharedNil();) {
            (void)FreeNodes(runtime.ReadWord(node + 8u), runtime);
            const auto next = runtime.ReadWord(node);
            (void)DestroyValue(node + 0xcu, 0u, runtime);
            (void)runtime.Free(node);
            node = next;
        }
        ResetHeader(tree, header, runtime);
    } else if (first != header) {
        EraseAltChain(tree, header, first, runtime);
    }

    header = runtime.ReadWord(tree + 4u);
    first = runtime.ReadWord(header);
    if (runtime.ReadWord(tree + 0xcu) != 0u &&
        first == runtime.FirstElement(tree)) {
        (void)FreeNodes(runtime.ReadWord(header + 4u), runtime);
        ResetHeader(tree, header, runtime);
    } else if (first != header) {
        EraseChain(tree, header, first, runtime);
    }
    return ReleaseShared(tree, runtime);
}

std::uint32_t UiEventTree::ClearAndRelease(
    const std::uint32_t tree, IUiEventTreeRuntime& runtime) {
    const auto header = runtime.ReadWord(tree + 4u);
    const auto first = runtime.ReadWord(header);
    if (runtime.ReadWord(tree + 0xcu) != 0u) {
        (void)FreeNodes(runtime.ReadWord(header + 4u), runtime);
        ResetHeader(tree, header, runtime);
    } else if (first != header) {
        EraseAltChain(tree, header, first, runtime);
    }
    return ReleaseShared(tree, runtime);
}

bool UiEventTree::InsertOrReplace(
    const std::uint32_t tree, const std::uint32_t key_address,
    const std::uint32_t handler_slot, IUiEventTreeRuntime& runtime) {
    const auto key = runtime.ReadWord(key_address);
    const auto candidate = FindNode(tree, key, runtime);
    const auto header = runtime.ReadWord(tree + 4u);
    if (candidate != header &&
        key >= runtime.ReadWord(candidate + 0xcu)) {
        (void)Erase(tree, candidate, runtime);
    }

    const auto clone = runtime.CloneHandler(runtime.ReadWord(handler_slot));
    try {
        const auto inserted = Insert(tree, key, clone, runtime).inserted;
        if (clone != 0u) (void)runtime.DeleteHandler(clone, 1u);
        return inserted;
    } catch (...) {
        if (clone != 0u) (void)runtime.DeleteHandler(clone, 1u);
        throw;
    }
}

UiEventInsertResult UiEventTree::Insert(
    const std::uint32_t tree, const std::uint32_t key,
    const std::uint32_t handler, IUiEventTreeRuntime& runtime) {
    const auto header = runtime.ReadWord(tree + 4u);
    const auto nil = runtime.SharedNil();
    auto parent = header;
    auto child = runtime.ReadWord(header + 4u);
    bool left = true;
    while (child != nil) {
        parent = child;
        left = key < runtime.ReadWord(child + 0xcu);
        child = runtime.ReadWord(child + (left ? 0u : 8u));
    }

    if (runtime.ReadByte(tree + 8u) == 0u) {
        auto candidate = parent;
        if (left) {
            if (parent == runtime.ReadWord(header)) {
                return {InsertNode(tree, child, parent, key, handler, runtime),
                        true};
            }
            (void)Predecessor(candidate, runtime);
        }
        if (runtime.ReadWord(candidate + 0xcu) >= key)
            return {candidate, false};
        return {InsertNode(tree, child, parent, key, handler, runtime), true};
    }

    const auto node = runtime.CreateNode(parent);
    runtime.WriteWord(node, nil);
    runtime.WriteWord(node + 8u, nil);
    CopyValue(node + 0xcu, key, handler, runtime);
    return {LinkAndBalance(tree, child, parent, key, node, runtime), true};
}

std::uint32_t UiEventTree::InsertNode(
    const std::uint32_t tree, const std::uint32_t child,
    const std::uint32_t parent, const std::uint32_t key,
    const std::uint32_t handler, IUiEventTreeRuntime& runtime) {
    const auto node = runtime.Allocate(0x18u);
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 0x14u, 0u);
    const auto nil = runtime.SharedNil();
    runtime.WriteWord(node, nil);
    runtime.WriteWord(node + 8u, nil);
    CopyValue(node + 0xcu, key, handler, runtime);
    return LinkAndBalance(tree, child, parent, key, node, runtime);
}

std::uint32_t UiEventTree::Erase(
    const std::uint32_t tree, const std::uint32_t node,
    IUiEventTreeRuntime& runtime) {
    auto next = node;
    SuccessorAlt(next, runtime);
    const auto header = runtime.ReadWord(tree + 4u);
    const auto nil = runtime.SharedNil();
    const auto left = runtime.ReadWord(node);
    const auto right = runtime.ReadWord(node + 8u);
    auto replacement = nil;

    if (left != nil && right != nil) {
        const auto successor = Minimum(right, nil, runtime);
        replacement = runtime.ReadWord(successor + 8u);
        runtime.WriteWord(left + 4u, successor);
        runtime.WriteWord(successor, left);
        if (successor == right) {
            runtime.WriteWord(replacement + 4u, successor);
        } else {
            const auto successor_parent = runtime.ReadWord(successor + 4u);
            runtime.WriteWord(replacement + 4u, successor_parent);
            runtime.WriteWord(successor_parent, replacement);
            runtime.WriteWord(successor + 8u, right);
            runtime.WriteWord(right + 4u, successor);
        }
        ReplaceParentLink(header, node, successor, runtime);
        runtime.WriteWord(successor + 4u, runtime.ReadWord(node + 4u));
        const auto colour = runtime.ReadWord(node + 0x14u);
        runtime.WriteWord(node + 0x14u,
                          runtime.ReadWord(successor + 0x14u));
        runtime.WriteWord(successor + 0x14u, colour);
    } else {
        replacement = left == nil ? right : left;
        runtime.WriteWord(replacement + 4u,
                          runtime.ReadWord(node + 4u));
        ReplaceParentLink(header, node, replacement, runtime);
        if (runtime.ReadWord(header) == node) {
            runtime.WriteWord(header, right == nil
                ? runtime.ReadWord(node + 4u)
                : Minimum(replacement, nil, runtime));
        }
        if (runtime.ReadWord(header + 8u) == node) {
            runtime.WriteWord(header + 8u, left == nil
                ? runtime.ReadWord(node + 4u)
                : Maximum(replacement, nil, runtime));
        }
    }

    if (runtime.ReadWord(node + 0x14u) == 1u)
        RestoreBlackHeight(tree, header, replacement, runtime);
    const auto handler = runtime.ReadWord(node + 0x10u);
    if (handler != 0u) (void)runtime.DeleteHandler(handler, 1u);
    (void)runtime.Free(node);
    runtime.WriteWord(tree + 0xcu,
                      runtime.ReadWord(tree + 0xcu) - 1u);
    return next;
}

std::uint32_t UiEventTree::FindNode(
    const std::uint32_t tree, const std::uint32_t key,
    IUiEventTreeRuntime& runtime) {
    auto candidate = runtime.ReadWord(tree + 4u);
    auto current = runtime.ReadWord(candidate + 4u);
    const auto nil = runtime.SharedNil();
    while (current != nil) {
        if (runtime.ReadWord(current + 0xcu) >= key) {
            candidate = current;
            current = runtime.ReadWord(current);
        } else {
            current = runtime.ReadWord(current + 8u);
        }
    }
    return candidate;
}

std::uint32_t UiEventTree::FreeNodes(
    std::uint32_t root, IUiEventTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    while (root != nil) {
        (void)FreeNodes(runtime.ReadWord(root + 8u), runtime);
        const auto next = runtime.ReadWord(root);
        (void)ReleaseClone(root + 0xcu, runtime);
        (void)runtime.Free(root);
        root = next;
    }
    return nil;
}

std::uint32_t UiEventTree::RotateRight(
    const std::uint32_t tree, const std::uint32_t node,
    IUiEventTreeRuntime& runtime) {
    const auto pivot = runtime.ReadWord(node + 8u);
    const auto middle = runtime.ReadWord(pivot);
    runtime.WriteWord(node + 8u, middle);
    if (middle != runtime.SharedNil()) runtime.WriteWord(middle + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(pivot + 4u, parent);
    const auto header = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(header + 4u)) {
        runtime.WriteWord(header + 4u, pivot);
    } else {
        runtime.WriteWord(parent +
            (node == runtime.ReadWord(parent) ? 0u : 8u), pivot);
    }
    runtime.WriteWord(pivot, node);
    runtime.WriteWord(node + 4u, pivot);
    return pivot;
}

std::uint32_t UiEventTree::RotateLeft(
    const std::uint32_t tree, const std::uint32_t node,
    IUiEventTreeRuntime& runtime) {
    const auto pivot = runtime.ReadWord(node);
    const auto middle = runtime.ReadWord(pivot + 8u);
    runtime.WriteWord(node, middle);
    if (middle != runtime.SharedNil()) runtime.WriteWord(middle + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(pivot + 4u, parent);
    const auto header = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(header + 4u)) {
        runtime.WriteWord(header + 4u, pivot);
    } else {
        runtime.WriteWord(parent +
            (node == runtime.ReadWord(parent + 8u) ? 8u : 0u), pivot);
    }
    runtime.WriteWord(pivot + 8u, node);
    runtime.WriteWord(node + 4u, pivot);
    return pivot;
}

std::uint32_t UiEventTree::Successor(
    std::uint32_t& iterator, IUiEventTreeRuntime& runtime) {
    const auto previous = iterator;
    const auto nil = runtime.SharedNil();
    auto right = runtime.ReadWord(iterator + 8u);
    if (right != nil) {
        while (runtime.ReadWord(right) != nil)
            right = runtime.ReadWord(right);
        iterator = right;
    } else {
        auto parent = runtime.ReadWord(iterator + 4u);
        while (iterator == runtime.ReadWord(parent + 8u)) {
            iterator = parent;
            parent = runtime.ReadWord(parent + 4u);
        }
        if (runtime.ReadWord(iterator + 8u) != parent)
            iterator = parent;
    }
    return previous;
}

std::uint32_t UiEventTree::Find(
    const std::uint32_t tree, const std::uint32_t key,
    IUiEventTreeRuntime& runtime) {
    return FindNode(tree, key, runtime);
}

std::uint32_t UiEventTree::Predecessor(
    std::uint32_t& iterator, IUiEventTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    const auto parent = runtime.ReadWord(iterator + 4u);
    if (runtime.ReadWord(iterator + 0x14u) == 0u &&
        runtime.ReadWord(parent + 4u) == iterator) {
        iterator = runtime.ReadWord(iterator + 8u);
        return iterator;
    }
    auto left = runtime.ReadWord(iterator);
    if (left != nil) {
        while (runtime.ReadWord(left + 8u) != nil)
            left = runtime.ReadWord(left + 8u);
        iterator = left;
        return iterator;
    }
    auto ancestor = parent;
    while (iterator == runtime.ReadWord(ancestor)) {
        iterator = ancestor;
        ancestor = runtime.ReadWord(ancestor + 4u);
    }
    iterator = ancestor;
    return iterator;
}

void UiEventTree::SuccessorAlt(
    std::uint32_t& iterator, IUiEventTreeRuntime& runtime) {
    (void)Successor(iterator, runtime);
}

void UiEventTree::CopyValue(
    const std::uint32_t destination, const std::uint32_t key,
    const std::uint32_t handler, IUiEventTreeRuntime& runtime) {
    if (destination == 0u) return;
    runtime.WriteWord(destination, key);
    runtime.WriteWord(destination + 4u, runtime.CloneHandler(handler));
}

std::uint32_t UiEventTree::DestroyValue(
    const std::uint32_t record, const std::uint8_t flags,
    IUiEventTreeRuntime& runtime) {
    (void)ReleaseClone(record, runtime);
    if ((flags & 1u) != 0u) (void)runtime.Free(record);
    return record;
}

std::uint32_t UiEventTree::ReleaseClone(
    const std::uint32_t record, IUiEventTreeRuntime& runtime) {
    const auto clone = runtime.ReadWord(record + 4u);
    return clone == 0u ? 0u : runtime.DeleteHandler(clone, 1u);
}

std::uint8_t UiEventTree::Process(
    const std::uint32_t tree, const std::uint32_t key_address,
    const std::uint32_t argument, IUiEventTreeRuntime& runtime) {
    const auto key = runtime.ReadWord(key_address);
    const auto candidate = FindNode(tree, key, runtime);
    const auto header = runtime.ReadWord(tree + 4u);
    if (candidate == header || key < runtime.ReadWord(candidate + 0xcu))
        return 0u;
    return runtime.InvokeHandler(runtime.ReadWord(candidate + 0x10u),
                                 argument);
}

}  // namespace pal4
