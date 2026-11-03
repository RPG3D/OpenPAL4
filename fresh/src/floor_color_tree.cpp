#include "pal4/floor_color_tree.h"

namespace pal4 {

void FloorColorValue::Copy(std::uint32_t destination, std::uint32_t source,
                           IPalFloorAttributeRuntime& runtime) {
    if (destination == 0u) return;
    runtime.WriteDword(destination, runtime.ReadDword(source));
    CeguiTextRecord::Copy(destination + 4u, source + 4u, runtime);
}

namespace {

class Tree final {
public:
    Tree(FloorColorTreeState& state, IPalFloorAttributeRuntime& runtime)
        : state_(state), runtime_(runtime),
          header_(state.header), nil_(runtime.ColorTreeGlobals().nil) {}

    [[nodiscard]] std::uint32_t Header() const { return header_; }
    [[nodiscard]] std::uint32_t Nil() const { return nil_; }
    [[nodiscard]] std::uint32_t Root() const { return Parent(header_); }
    [[nodiscard]] std::uint32_t Left(std::uint32_t node) const {
        return Read(node);
    }
    [[nodiscard]] std::uint32_t Parent(std::uint32_t node) const {
        return Read(node + 4u);
    }
    [[nodiscard]] std::uint32_t Right(std::uint32_t node) const {
        return Read(node + 8u);
    }
    [[nodiscard]] std::uint32_t Key(std::uint32_t node) const {
        return Read(node + 12u);
    }
    [[nodiscard]] std::uint32_t Black(std::uint32_t node) const {
        return Read(node + 32u);
    }
    void SetLeft(std::uint32_t node, std::uint32_t child) {
        Write(node, child);
    }
    void SetParent(std::uint32_t node, std::uint32_t parent) {
        Write(node + 4u, parent);
    }
    void SetRight(std::uint32_t node, std::uint32_t child) {
        Write(node + 8u, child);
    }
    void SetBlack(std::uint32_t node, std::uint32_t black) {
        Write(node + 32u, black);
    }

    [[nodiscard]] std::uint32_t Find(std::uint32_t key) const {
        auto node = Root();
        auto candidate = header_;
        while (node != nil_) {
            if (Key(node) >= key) {
                candidate = node;
                node = Left(node);
            } else {
                node = Right(node);
            }
        }
        return candidate == header_ || key < Key(candidate)
                   ? header_ : candidate;
    }

    [[nodiscard]] FloorColorInsertResult Insert(std::uint32_t value) {
        const auto key = Read(value);
        if ((state_.reserved & 0xffu) == 0u) {
            const auto existing = Find(key);
            if (existing != header_) return {existing, false};
        }
        auto parent = header_;
        auto child = Root();
        while (child != nil_) {
            parent = child;
            child = key < Key(child) ? Left(child) : Right(child);
        }
        return {InsertAt(child, parent, value), true};
    }

    [[nodiscard]] std::uint32_t InsertAt(std::uint32_t child_hint,
                                          std::uint32_t parent,
                                          std::uint32_t value) {
        const auto key = Read(value);
        const auto node = runtime_.AllocateColorNode(36u);
        SetLeft(node, nil_);
        SetParent(node, parent);
        SetRight(node, nil_);
        FloorColorValue::Copy(node + 12u, value, runtime_);
        SetBlack(node, 0u);
        ++state_.size;

        if (parent == header_ || child_hint != nil_ || key < Key(parent)) {
            SetLeft(parent, node);
            if (parent == header_) {
                SetParent(header_, node);
                SetRight(header_, node);
            } else if (parent == Left(header_)) {
                SetLeft(header_, node);
            }
        } else {
            SetRight(parent, node);
            if (parent == Right(header_)) SetRight(header_, node);
        }
        Repair(node);
        return node;
    }

    [[nodiscard]] std::uint32_t RotateLeftAt(std::uint32_t node) {
        const auto promoted = Right(node);
        RotateLeft(node);
        return promoted;
    }

    [[nodiscard]] std::uint32_t RotateRightAt(std::uint32_t node) {
        const auto promoted = Left(node);
        RotateRight(node);
        return promoted;
    }

    [[nodiscard]] std::uint32_t Leftmost(std::uint32_t node) const {
        while (node != nil_) {
            const auto left = Left(node);
            if (left == nil_) break;
            node = left;
        }
        return node;
    }

    [[nodiscard]] std::uint32_t Previous(std::uint32_t node) const {
        if (Black(node) == 0u && Parent(Parent(node)) == node)
            return Right(node);
        auto child = Left(node);
        if (child != nil_) {
            while (Right(child) != nil_) child = Right(child);
            return child;
        }
        auto parent = Parent(node);
        while (node == Left(parent)) {
            node = parent;
            parent = Parent(parent);
        }
        return parent;
    }

    [[nodiscard]] std::uint32_t Next(std::uint32_t node) const {
        if (Right(node) != nil_) return Leftmost(Right(node));
        auto parent = Parent(node);
        while (node == Right(parent)) {
            node = parent;
            parent = Parent(parent);
        }
        if (Right(node) != parent) node = parent;
        return node;
    }

    [[nodiscard]] std::uint32_t Remove(std::uint32_t node) {
        const auto next = Next(node);
        auto detached = node;
        if (Left(node) != nil_ && Right(node) != nil_) {
            detached = Right(node);
            detached = Leftmost(detached);
        }
        const auto child = Left(detached) != nil_
                               ? Left(detached) : Right(detached);
        if (detached != node) {
            SetParent(Left(node), detached);
            SetLeft(detached, Left(node));
            if (detached == Right(node)) {
                SetParent(child, detached);
            } else {
                SetParent(child, Parent(detached));
                SetLeft(Parent(detached), child);
                SetRight(detached, Right(node));
                SetParent(Right(node), detached);
            }
            ReplaceParentLink(node, detached);
            SetParent(detached, Parent(node));
            const auto removed_color = Black(detached);
            SetBlack(detached, Black(node));
            SetBlack(node, removed_color);
        } else {
            SetParent(child, Parent(node));
            ReplaceParentLink(node, child);
            if (Left(header_) == node) {
                if (Right(node) == nil_) {
                    SetLeft(header_, Parent(node));
                } else {
                    SetLeft(header_, Leftmost(child));
                }
            }
            if (Right(header_) == node) {
                if (Left(node) == nil_) {
                    SetRight(header_, Parent(node));
                } else {
                    auto rightmost = child;
                    while (Right(rightmost) != nil_) rightmost = Right(rightmost);
                    SetRight(header_, rightmost);
                }
            }
        }
        if (Black(node) == 1u) RepairAfterRemove(child);
        ReleaseNodeStorage(node);
        --state_.size;
        return next;
    }

    [[nodiscard]] std::uint32_t DestroySubtree(std::uint32_t node) {
        while (node != nil_) {
            static_cast<void>(DestroySubtree(Right(node)));
            const auto left = Left(node);
            ReleaseNodeStorage(node);
            node = left;
        }
        return nil_;
    }

private:
    void ReleaseNodeStorage(std::uint32_t node) {
        const auto text = Read(node + 20u);
        if (text != 0u) {
            const auto count = runtime_.ReadByte(text - 1u);
            if (count == 0u || count == 0xffu) {
                static_cast<void>(runtime_.FreeAddress(text - 1u));
            } else {
                runtime_.WriteByte(text - 1u,
                                   static_cast<std::uint8_t>(count - 1u));
            }
        }
        Write(node + 20u, 0u);
        Write(node + 24u, 0u);
        Write(node + 28u, 0u);
        static_cast<void>(runtime_.FreeAddress(node));
    }
    [[nodiscard]] std::uint32_t Read(std::uint32_t address) const {
        return runtime_.ReadDword(address);
    }
    void Write(std::uint32_t address, std::uint32_t value) {
        runtime_.WriteDword(address, value);
    }
    void ReplaceParentLink(std::uint32_t old_node,
                           std::uint32_t replacement) {
        const auto parent = Parent(old_node);
        if (old_node == Root()) SetParent(header_, replacement);
        else if (old_node == Left(parent)) SetLeft(parent, replacement);
        else SetRight(parent, replacement);
    }
    void RotateLeft(std::uint32_t node) {
        const auto right = Right(node);
        SetRight(node, Left(right));
        if (Left(right) != nil_) SetParent(Left(right), node);
        SetParent(right, Parent(node));
        if (node == Root()) SetParent(header_, right);
        else if (node == Left(Parent(node))) SetLeft(Parent(node), right);
        else SetRight(Parent(node), right);
        SetLeft(right, node);
        SetParent(node, right);
    }
    void RotateRight(std::uint32_t node) {
        const auto left = Left(node);
        SetLeft(node, Right(left));
        if (Right(left) != nil_) SetParent(Right(left), node);
        SetParent(left, Parent(node));
        if (node == Root()) SetParent(header_, left);
        else if (node == Right(Parent(node))) SetRight(Parent(node), left);
        else SetLeft(Parent(node), left);
        SetRight(left, node);
        SetParent(node, left);
    }
    void Repair(std::uint32_t node) {
        while (node != Root() && Black(Parent(node)) == 0u) {
            const auto parent = Parent(node);
            const auto grandparent = Parent(parent);
            if (parent == Left(grandparent)) {
                const auto uncle = Right(grandparent);
                if (Black(uncle) == 0u) {
                    SetBlack(parent, 1u);
                    SetBlack(uncle, 1u);
                    SetBlack(grandparent, 0u);
                    node = grandparent;
                } else {
                    if (node == Right(parent)) {
                        node = parent;
                        RotateLeft(node);
                    }
                    SetBlack(Parent(node), 1u);
                    SetBlack(Parent(Parent(node)), 0u);
                    RotateRight(Parent(Parent(node)));
                }
            } else {
                const auto uncle = Left(grandparent);
                if (Black(uncle) == 0u) {
                    SetBlack(parent, 1u);
                    SetBlack(uncle, 1u);
                    SetBlack(grandparent, 0u);
                    node = grandparent;
                } else {
                    if (node == Left(parent)) {
                        node = parent;
                        RotateRight(node);
                    }
                    SetBlack(Parent(node), 1u);
                    SetBlack(Parent(Parent(node)), 0u);
                    RotateLeft(Parent(Parent(node)));
                }
            }
        }
        SetBlack(Root(), 1u);
    }
    void RepairAfterRemove(std::uint32_t node) {
        while (node != Root() && Black(node) == 1u) {
            const auto parent = Parent(node);
            if (node == Left(parent)) {
                auto sibling = Right(parent);
                if (Black(sibling) == 0u) {
                    SetBlack(sibling, 1u);
                    SetBlack(parent, 0u);
                    RotateLeft(parent);
                    sibling = Right(Parent(node));
                }
                if (Black(Left(sibling)) == 1u &&
                    Black(Right(sibling)) == 1u) {
                    SetBlack(sibling, 0u);
                    node = Parent(node);
                } else {
                    if (Black(Right(sibling)) == 1u) {
                        SetBlack(Left(sibling), 1u);
                        SetBlack(sibling, 0u);
                        RotateRight(sibling);
                        sibling = Right(Parent(node));
                    }
                    SetBlack(sibling, Black(Parent(node)));
                    SetBlack(Parent(node), 1u);
                    SetBlack(Right(sibling), 1u);
                    RotateLeft(Parent(node));
                    break;
                }
            } else {
                auto sibling = Left(parent);
                if (Black(sibling) == 0u) {
                    SetBlack(sibling, 1u);
                    SetBlack(parent, 0u);
                    RotateRight(parent);
                    sibling = Left(Parent(node));
                }
                if (Black(Right(sibling)) == 1u &&
                    Black(Left(sibling)) == 1u) {
                    SetBlack(sibling, 0u);
                    node = Parent(node);
                } else {
                    if (Black(Left(sibling)) == 1u) {
                        SetBlack(Right(sibling), 1u);
                        SetBlack(sibling, 0u);
                        RotateLeft(sibling);
                        sibling = Left(Parent(node));
                    }
                    SetBlack(sibling, Black(Parent(node)));
                    SetBlack(Parent(node), 1u);
                    SetBlack(Left(sibling), 1u);
                    RotateRight(Parent(node));
                    break;
                }
            }
        }
        SetBlack(node, 1u);
    }

    FloorColorTreeState& state_;
    IPalFloorAttributeRuntime& runtime_;
    std::uint32_t header_;
    std::uint32_t nil_;
};

}  // namespace

void FloorColorTree::Initialize(
    PalFloorAttributeState& state, std::uint8_t first_byte,
    std::uint8_t second_byte, bool allow_duplicates,
    IPalFloorAttributeRuntime& runtime) {
    auto& tree = state.color_tree;
    tree.flags = (tree.flags & 0xffff0000u) |
                 static_cast<std::uint32_t>(first_byte) |
                 (static_cast<std::uint32_t>(second_byte) << 8u);
    tree.reserved = (tree.reserved & 0xffffff00u) |
                    static_cast<std::uint32_t>(allow_duplicates);
    auto& globals = runtime.ColorTreeGlobals();
    const auto candidate = runtime.AllocateColorNode(36u);
    runtime.WriteDword(candidate + 32u, 1u);
    runtime.WriteDword(candidate + 4u, 0u);
    if (globals.nil == 0u) {
        globals.nil = candidate;
        runtime.WriteDword(candidate, 0u);
        runtime.WriteDword(candidate + 8u, 0u);
    } else {
        static_cast<void>(runtime.FreeAddress(candidate));
    }
    ++globals.owners;
    const auto header = runtime.AllocateColorNode(36u);
    runtime.WriteDword(header + 4u, globals.nil);
    runtime.WriteDword(header + 32u, 0u);
    tree.header = header;
    tree.size = 0u;
    runtime.WriteDword(header, header);
    runtime.WriteDword(header + 8u, header);
}

void FloorColorTree::ReleaseSharedNil(IPalFloorAttributeRuntime& runtime) {
    auto& globals = runtime.ColorTreeGlobals();
    if (--globals.owners == 0u) {
        const auto nil = globals.nil;
        globals.nil = 0u;
        if (nil != 0u) static_cast<void>(runtime.FreeAddress(nil));
    }
}

FloorColorInsertResult FloorColorTree::Insert(
    FloorColorTreeState& tree, std::uint32_t value_record,
    IPalFloorAttributeRuntime& runtime) {
    return Tree(tree, runtime).Insert(value_record);
}

std::uint32_t FloorColorTree::InsertAt(
    FloorColorTreeState& tree, std::uint32_t child_hint,
    std::uint32_t parent, std::uint32_t value_record,
    IPalFloorAttributeRuntime& runtime) {
    return Tree(tree, runtime).InsertAt(child_hint, parent, value_record);
}

std::uint32_t FloorColorTree::RotateLeft(
    FloorColorTreeState& tree, std::uint32_t node,
    IPalFloorAttributeRuntime& runtime) {
    return Tree(tree, runtime).RotateLeftAt(node);
}

std::uint32_t FloorColorTree::RotateRight(
    FloorColorTreeState& tree, std::uint32_t node,
    IPalFloorAttributeRuntime& runtime) {
    return Tree(tree, runtime).RotateRightAt(node);
}

std::uint32_t FloorColorTree::Leftmost(
    FloorColorTreeState& tree, std::uint32_t root,
    IPalFloorAttributeRuntime& runtime) {
    return Tree(tree, runtime).Leftmost(root);
}

std::uint32_t FloorColorTree::Previous(
    FloorColorTreeState& tree, std::uint32_t node,
    IPalFloorAttributeRuntime& runtime) {
    return Tree(tree, runtime).Previous(node);
}

std::uint32_t FloorColorTree::Next(
    FloorColorTreeState& tree, std::uint32_t node,
    IPalFloorAttributeRuntime& runtime) {
    return Tree(tree, runtime).Next(node);
}

std::uint32_t FloorColorTree::Find(
    FloorColorTreeState& tree, std::uint32_t key,
    IPalFloorAttributeRuntime& runtime) {
    return Tree(tree, runtime).Find(key);
}

std::uint32_t FloorColorTree::Remove(
    FloorColorTreeState& tree, std::uint32_t node,
    IPalFloorAttributeRuntime& runtime) {
    return Tree(tree, runtime).Remove(node);
}

std::uint32_t FloorColorTree::DestroySubtree(
    FloorColorTreeState& tree, std::uint32_t root,
    IPalFloorAttributeRuntime& runtime) {
    return Tree(tree, runtime).DestroySubtree(root);
}

std::uint32_t FloorColorTree::EraseRange(
    FloorColorTreeState& tree, std::uint32_t first,
    std::uint32_t last, IPalFloorAttributeRuntime& runtime) {
    const auto header = tree.header;
    if (tree.size != 0u && first == runtime.ReadDword(header) &&
        last == header) {
        static_cast<void>(DestroySubtree(
            tree, runtime.ReadDword(header + 4u), runtime));
        runtime.WriteDword(header + 4u, runtime.ColorTreeGlobals().nil);
        tree.size = 0u;
        runtime.WriteDword(header, header);
        runtime.WriteDword(header + 8u, header);
        return header;
    }
    auto node = first;
    while (node != last) {
        node = Tree(tree, runtime).Remove(node);
    }
    return node;
}

}  // namespace pal4
