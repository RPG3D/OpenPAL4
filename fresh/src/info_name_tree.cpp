#include "pal4/info_name_tree.h"

#include <string>

namespace pal4 {
namespace {

using Address = std::uint32_t;

Address Word(INpcInfoRuntime& runtime, const Address address) {
    return runtime.ReadWord(address);
}
void Set(INpcInfoRuntime& runtime, const Address address, const Address value) {
    runtime.WriteWord(address, value);
}
Address Left(const Address node, const Address nil, INpcInfoRuntime& runtime) {
    return node == nil ? nil : Word(runtime, node);
}
Address Right(const Address node, const Address nil, INpcInfoRuntime& runtime) {
    return node == nil ? nil : Word(runtime, node + 8u);
}
Address Parent(const Address node, INpcInfoRuntime& runtime) {
    return Word(runtime, node + 4u);
}
bool Black(const Address node, const Address nil, INpcInfoRuntime& runtime) {
    return node == nil || Word(runtime, node + 32u) != 0u;
}
void SetBlack(const Address node, const Address nil,
              const bool black, INpcInfoRuntime& runtime) {
    if (node != nil) Set(runtime, node + 32u, black ? 1u : 0u);
}
Address Minimum(Address node, const Address nil, INpcInfoRuntime& runtime) {
    while (Left(node, nil, runtime) != nil) node = Left(node, nil, runtime);
    return node;
}
Address Maximum(Address node, const Address nil, INpcInfoRuntime& runtime) {
    while (Right(node, nil, runtime) != nil) node = Right(node, nil, runtime);
    return node;
}
void RefreshEnds(const Address head, const Address nil,
                 INpcInfoRuntime& runtime) {
    const auto root = Word(runtime, head + 4u);
    Set(runtime, head, root == nil ? head : Minimum(root, nil, runtime));
    Set(runtime, head + 8u, root == nil ? head : Maximum(root, nil, runtime));
}
void Transplant(const Address head, const Address from, const Address to,
                const Address nil, INpcInfoRuntime& runtime) {
    const auto parent = Parent(from, runtime);
    if (parent == head)
        Set(runtime, head + 4u, to);
    else if (from == Left(parent, nil, runtime))
        Set(runtime, parent, to);
    else
        Set(runtime, parent + 8u, to);
    if (to != nil) Set(runtime, to + 4u, parent);
}

void FixAfterErase(const Address tree, Address node, Address parent,
                   const Address nil, INpcInfoRuntime& runtime) {
    const auto head = Word(runtime, tree + 4u);
    while (node != Word(runtime, head + 4u) && Black(node, nil, runtime)) {
        if (node == Left(parent, nil, runtime)) {
            auto sibling = Right(parent, nil, runtime);
            if (!Black(sibling, nil, runtime)) {
                SetBlack(sibling, nil, true, runtime);
                SetBlack(parent, nil, false, runtime);
                (void)InfoNameTree::RotateLeft(tree, parent, nil, runtime);
                sibling = Right(parent, nil, runtime);
            }
            if (Black(Left(sibling, nil, runtime), nil, runtime) &&
                Black(Right(sibling, nil, runtime), nil, runtime)) {
                SetBlack(sibling, nil, false, runtime);
                node = parent;
                parent = Parent(node, runtime);
            } else {
                if (Black(Right(sibling, nil, runtime), nil, runtime)) {
                    SetBlack(Left(sibling, nil, runtime), nil, true, runtime);
                    SetBlack(sibling, nil, false, runtime);
                    (void)InfoNameTree::RotateRight(tree, sibling, nil, runtime);
                    sibling = Right(parent, nil, runtime);
                }
                SetBlack(sibling, nil, Black(parent, nil, runtime), runtime);
                SetBlack(parent, nil, true, runtime);
                SetBlack(Right(sibling, nil, runtime), nil, true, runtime);
                (void)InfoNameTree::RotateLeft(tree, parent, nil, runtime);
                node = Word(runtime, head + 4u);
            }
        } else {
            auto sibling = Left(parent, nil, runtime);
            if (!Black(sibling, nil, runtime)) {
                SetBlack(sibling, nil, true, runtime);
                SetBlack(parent, nil, false, runtime);
                (void)InfoNameTree::RotateRight(tree, parent, nil, runtime);
                sibling = Left(parent, nil, runtime);
            }
            if (Black(Right(sibling, nil, runtime), nil, runtime) &&
                Black(Left(sibling, nil, runtime), nil, runtime)) {
                SetBlack(sibling, nil, false, runtime);
                node = parent;
                parent = Parent(node, runtime);
            } else {
                if (Black(Left(sibling, nil, runtime), nil, runtime)) {
                    SetBlack(Right(sibling, nil, runtime), nil, true, runtime);
                    SetBlack(sibling, nil, false, runtime);
                    (void)InfoNameTree::RotateLeft(tree, sibling, nil, runtime);
                    sibling = Left(parent, nil, runtime);
                }
                SetBlack(sibling, nil, Black(parent, nil, runtime), runtime);
                SetBlack(parent, nil, true, runtime);
                SetBlack(Left(sibling, nil, runtime), nil, true, runtime);
                (void)InfoNameTree::RotateRight(tree, parent, nil, runtime);
                node = Word(runtime, head + 4u);
            }
        }
    }
    SetBlack(node, nil, true, runtime);
}

void ReleaseBranch(Address node, const Address nil,
                   INpcInfoRuntime& runtime) {
    while (node != nil) {
        const auto right = Right(node, nil, runtime);
        const auto left = Left(node, nil, runtime);
        ReleaseBranch(right, nil, runtime);
        runtime.CleanupString(node + 12u);
        runtime.Free(node);
        node = left;
    }
}

}  // namespace

std::uint32_t InfoNameTree::RotateLeft(const Address tree, const Address node,
                                       const Address nil,
                                       INpcInfoRuntime& runtime) {
    const auto pivot = Right(node, nil, runtime);
    const auto moved = Left(pivot, nil, runtime);
    Set(runtime, node + 8u, moved);
    if (moved != nil) Set(runtime, moved + 4u, node);
    const auto parent = Parent(node, runtime);
    Set(runtime, pivot + 4u, parent);
    const auto head = Word(runtime, tree + 4u);
    if (node == Word(runtime, head + 4u))
        Set(runtime, head + 4u, pivot);
    else if (node == Left(parent, nil, runtime))
        Set(runtime, parent, pivot);
    else
        Set(runtime, parent + 8u, pivot);
    Set(runtime, pivot, node);
    Set(runtime, node + 4u, pivot);
    return pivot;
}

std::uint32_t InfoNameTree::RotateRight(const Address tree, const Address node,
                                        const Address nil,
                                        INpcInfoRuntime& runtime) {
    const auto pivot = Left(node, nil, runtime);
    const auto moved = Right(pivot, nil, runtime);
    Set(runtime, node, moved);
    if (moved != nil) Set(runtime, moved + 4u, node);
    const auto parent = Parent(node, runtime);
    Set(runtime, pivot + 4u, parent);
    const auto head = Word(runtime, tree + 4u);
    if (node == Word(runtime, head + 4u))
        Set(runtime, head + 4u, pivot);
    else if (node == Right(parent, nil, runtime))
        Set(runtime, parent + 8u, pivot);
    else
        Set(runtime, parent, pivot);
    Set(runtime, pivot + 8u, node);
    Set(runtime, node + 4u, pivot);
    return pivot;
}

InfoNameTreeInsertResult InfoNameTree::Insert(
    const Address tree, const std::string_view name, const Address record,
    const Address nil, INpcInfoRuntime& runtime) {
    const auto head = Word(runtime, tree + 4u);
    auto parent = head;
    auto cursor = Word(runtime, head + 4u);
    const bool allow_duplicates = Word(runtime, tree + 8u) != 0u;
    while (cursor != nil) {
        parent = cursor;
        const auto compare = name.compare(runtime.ReadStringValue(cursor + 12u));
        if (compare == 0 && !allow_duplicates) return {cursor, false};
        cursor = Word(runtime, cursor + (compare < 0 ? 0u : 8u));
    }
    const auto node = runtime.Allocate(36u);
    if (!node) return {};
    Set(runtime, node, nil);
    Set(runtime, node + 4u, parent);
    Set(runtime, node + 8u, nil);
    Set(runtime, node + 32u, 0);
    runtime.InitializeString(node + 12u);
    runtime.AssignString(node + 12u, name);
    Set(runtime, node + 28u, record);
    if (parent == head)
        Set(runtime, head + 4u, node);
    else if (name.compare(runtime.ReadStringValue(parent + 12u)) < 0)
        Set(runtime, parent, node);
    else
        Set(runtime, parent + 8u, node);
    Set(runtime, tree + 12u, Word(runtime, tree + 12u) + 1u);

    auto current = node;
    while (current != Word(runtime, head + 4u)) {
        const auto p = Parent(current, runtime);
        if (Black(p, nil, runtime)) break;
        const auto grand = Parent(p, runtime);
        if (p == Left(grand, nil, runtime)) {
            const auto uncle = Right(grand, nil, runtime);
            if (!Black(uncle, nil, runtime)) {
                SetBlack(p, nil, true, runtime);
                SetBlack(uncle, nil, true, runtime);
                SetBlack(grand, nil, false, runtime);
                current = grand;
            } else {
                if (current == Right(p, nil, runtime)) {
                    current = p;
                    (void)RotateLeft(tree, current, nil, runtime);
                }
                const auto new_parent = Parent(current, runtime);
                const auto new_grand = Parent(new_parent, runtime);
                SetBlack(new_parent, nil, true, runtime);
                SetBlack(new_grand, nil, false, runtime);
                (void)RotateRight(tree, new_grand, nil, runtime);
            }
        } else {
            const auto uncle = Left(grand, nil, runtime);
            if (!Black(uncle, nil, runtime)) {
                SetBlack(p, nil, true, runtime);
                SetBlack(uncle, nil, true, runtime);
                SetBlack(grand, nil, false, runtime);
                current = grand;
            } else {
                if (current == Left(p, nil, runtime)) {
                    current = p;
                    (void)RotateRight(tree, current, nil, runtime);
                }
                const auto new_parent = Parent(current, runtime);
                const auto new_grand = Parent(new_parent, runtime);
                SetBlack(new_parent, nil, true, runtime);
                SetBlack(new_grand, nil, false, runtime);
                (void)RotateLeft(tree, new_grand, nil, runtime);
            }
        }
    }
    SetBlack(Word(runtime, head + 4u), nil, true, runtime);
    RefreshEnds(head, nil, runtime);
    return {node, true};
}

std::uint32_t InfoNameTree::LowerBound(
    const Address tree, const std::string_view name, const Address nil,
    INpcInfoRuntime& runtime) {
    const auto head = Word(runtime, tree + 4u);
    auto candidate = head;
    auto cursor = Word(runtime, head + 4u);
    while (cursor != nil) {
        if (runtime.ReadStringValue(cursor + 12u).compare(name) >= 0) {
            candidate = cursor;
            cursor = Left(cursor, nil, runtime);
        } else {
            cursor = Right(cursor, nil, runtime);
        }
    }
    return candidate;
}

std::uint32_t InfoNameTree::Find(
    const Address tree, const std::string_view name, const Address nil,
    INpcInfoRuntime& runtime) {
    const auto node = LowerBound(tree, name, nil, runtime);
    const auto head = Word(runtime, tree + 4u);
    return node != head && runtime.ReadStringValue(node + 12u) == name ?
               node : head;
}

std::uint32_t InfoNameTree::Next(const Address node, const Address head,
                                 const Address nil, INpcInfoRuntime& runtime) {
    if (node == head) return head;
    const auto right = Right(node, nil, runtime);
    if (right != nil) return Minimum(right, nil, runtime);
    auto current = node;
    auto parent = Parent(current, runtime);
    while (parent != head && current == Right(parent, nil, runtime)) {
        current = parent;
        parent = Parent(parent, runtime);
    }
    return parent;
}

std::uint32_t InfoNameTree::Previous(const Address node, const Address head,
                                     const Address nil, INpcInfoRuntime& runtime) {
    if (node == head) return Word(runtime, head + 8u);
    const auto left = Left(node, nil, runtime);
    if (left != nil) return Maximum(left, nil, runtime);
    auto current = node;
    auto parent = Parent(current, runtime);
    while (parent != head && current == Left(parent, nil, runtime)) {
        current = parent;
        parent = Parent(parent, runtime);
    }
    return parent;
}

void InfoNameTree::Erase(const Address tree, const Address node,
                         const Address nil, INpcInfoRuntime& runtime) {
    const auto head = Word(runtime, tree + 4u);
    if (node == head || node == nil) return;
    auto removed = node;
    auto removed_black = Black(removed, nil, runtime);
    Address child;
    Address child_parent;
    if (Left(node, nil, runtime) == nil) {
        child = Right(node, nil, runtime);
        child_parent = Parent(node, runtime);
        Transplant(head, node, child, nil, runtime);
    } else if (Right(node, nil, runtime) == nil) {
        child = Left(node, nil, runtime);
        child_parent = Parent(node, runtime);
        Transplant(head, node, child, nil, runtime);
    } else {
        removed = Minimum(Right(node, nil, runtime), nil, runtime);
        removed_black = Black(removed, nil, runtime);
        child = Right(removed, nil, runtime);
        if (Parent(removed, runtime) == node) {
            child_parent = removed;
        } else {
            child_parent = Parent(removed, runtime);
            Transplant(head, removed, child, nil, runtime);
            Set(runtime, removed + 8u, Right(node, nil, runtime));
            Set(runtime, Right(removed, nil, runtime) + 4u, removed);
        }
        Transplant(head, node, removed, nil, runtime);
        Set(runtime, removed, Left(node, nil, runtime));
        Set(runtime, Left(removed, nil, runtime) + 4u, removed);
        SetBlack(removed, nil, Black(node, nil, runtime), runtime);
    }
    if (removed_black) FixAfterErase(tree, child, child_parent, nil, runtime);
    runtime.CleanupString(node + 12u);
    runtime.Free(node);
    Set(runtime, tree + 12u, Word(runtime, tree + 12u) - 1u);
    RefreshEnds(head, nil, runtime);
}

void InfoNameTree::ClearNodes(const Address tree, const Address nil,
                              INpcInfoRuntime& runtime) {
    const auto head = Word(runtime, tree + 4u);
    if (!head) return;
    ReleaseBranch(Word(runtime, head + 4u), nil, runtime);
    Set(runtime, head, head);
    Set(runtime, head + 4u, nil);
    Set(runtime, head + 8u, head);
    Set(runtime, tree + 12u, 0);
}

void InfoNameTree::CopyEntry(const Address destination, const Address source,
                             INpcInfoRuntime& runtime) {
    if (!destination) return;
    if (destination == source) return;
    const auto length = Word(runtime, source + 8u);
    const auto text = Word(runtime, source + 4u);
    runtime.WriteByte(destination, runtime.ReadByte(source));
    for (const auto offset : {4u, 8u, 12u}) Set(runtime, destination + offset, 0);
    if (length && text && runtime.ReadByte(text - 1u) < 0xfeu) {
        runtime.SetStringFlag(destination, 1u);
        Set(runtime, destination + 4u, text);
        Set(runtime, destination + 8u, length);
        Set(runtime, destination + 12u, Word(runtime, source + 12u));
        runtime.WriteByte(text - 1u,
                          static_cast<std::uint8_t>(runtime.ReadByte(text - 1u) + 1u));
    } else {
        const auto storage = runtime.Allocate(length + 2u);
        if (storage) {
            runtime.WriteByte(storage, 0u);
            for (auto index = 0u; index < length; ++index)
                runtime.WriteByte(storage + 1u + index,
                                  text ? runtime.ReadByte(text + index) : 0u);
            runtime.WriteByte(storage + 1u + length, 0u);
            Set(runtime, destination + 4u, storage + 1u);
            Set(runtime, destination + 8u, length);
            Set(runtime, destination + 12u, length);
        }
    }
    Set(runtime, destination + 16u, Word(runtime, source + 16u));
}

}  // namespace pal4
