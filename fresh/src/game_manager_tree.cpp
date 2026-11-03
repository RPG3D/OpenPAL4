#include "pal4/game_manager_tree.h"

namespace pal4 {
namespace {

class Tree final {
public:
    Tree(const std::uint32_t address, IGameManagerTreeMemory& memory)
        : address_(address), header_(memory.ReadDword(address + 4u)), memory_(memory) {}

    [[nodiscard]] std::uint32_t Header() const { return header_; }
    [[nodiscard]] std::uint32_t Root() const { return Parent(header_); }
    [[nodiscard]] std::uint32_t Left(std::uint32_t node) const { return Read(node); }
    [[nodiscard]] std::uint32_t Parent(std::uint32_t node) const { return Read(node + 4u); }
    [[nodiscard]] std::uint32_t Right(std::uint32_t node) const { return Read(node + 8u); }
    [[nodiscard]] std::int32_t Key(std::uint32_t node) const {
        return static_cast<std::int32_t>(Read(node + 12u));
    }
    [[nodiscard]] std::uint32_t Color(std::uint32_t node) const { return Read(node + 20u); }
    [[nodiscard]] std::uint32_t Read(std::uint32_t address) const {
        return memory_.ReadDword(address);
    }
    void Write(std::uint32_t address, std::uint32_t value) {
        memory_.WriteDword(address, value);
    }
    void SetLeft(std::uint32_t node, std::uint32_t child) { Write(node, child); }
    void SetParent(std::uint32_t node, std::uint32_t parent) { Write(node + 4u, parent); }
    void SetRight(std::uint32_t node, std::uint32_t child) { Write(node + 8u, child); }
    void SetColor(std::uint32_t node, std::uint32_t color) { Write(node + 20u, color); }

    [[nodiscard]] std::uint32_t Predecessor(std::uint32_t node) const {
        if (Left(node) != GameManagerTreeInsert::kNilNode) {
            node = Left(node);
            while (Right(node) != GameManagerTreeInsert::kNilNode) node = Right(node);
            return node;
        }
        auto parent = Parent(node);
        while (node == Left(parent)) {
            node = parent;
            parent = Parent(parent);
        }
        return parent;
    }

    [[nodiscard]] std::uint32_t InsertNode(
        std::uint32_t child, std::uint32_t parent,
        std::int32_t key, std::uint32_t value) {
        const auto node = memory_.AllocateNode();
        SetLeft(node, GameManagerTreeInsert::kNilNode);
        SetParent(node, parent);
        SetRight(node, GameManagerTreeInsert::kNilNode);
        Write(node + 12u, static_cast<std::uint32_t>(key));
        Write(node + 16u, value);
        SetColor(node, 0u);
        Write(address_ + 12u, Read(address_ + 12u) + 1u);

        if (parent == header_ || child != GameManagerTreeInsert::kNilNode ||
            key < Key(parent)) {
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

    [[nodiscard]] std::uint32_t RemoveNode(std::uint32_t node) {
        const auto next = GameManagerTreeCursor::Next(node, memory_);
        auto detached = node;
        if (Left(node) != GameManagerTreeInsert::kNilNode &&
            Right(node) != GameManagerTreeInsert::kNilNode) {
            detached = Right(node);
            while (Left(detached) != GameManagerTreeInsert::kNilNode)
                detached = Left(detached);
        }
        const auto child = Left(detached) != GameManagerTreeInsert::kNilNode
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
            const auto color = Color(detached);
            SetColor(detached, Color(node));
            SetColor(node, color);
        } else {
            SetParent(child, Parent(node));
            ReplaceParentLink(node, child);
            if (Left(header_) == node) {
                if (Right(node) == GameManagerTreeInsert::kNilNode)
                    SetLeft(header_, Parent(node));
                else {
                    auto leftmost = child;
                    while (Left(leftmost) != GameManagerTreeInsert::kNilNode)
                        leftmost = Left(leftmost);
                    SetLeft(header_, leftmost);
                }
            }
            if (Right(header_) == node) {
                if (Left(node) == GameManagerTreeInsert::kNilNode)
                    SetRight(header_, Parent(node));
                else {
                    auto rightmost = child;
                    while (Right(rightmost) != GameManagerTreeInsert::kNilNode)
                        rightmost = Right(rightmost);
                    SetRight(header_, rightmost);
                }
            }
        }
        if (Color(node) == 1u) RepairAfterRemove(child);
        memory_.FreeNode(node);
        Write(address_ + 12u, Read(address_ + 12u) - 1u);
        return next;
    }

    [[nodiscard]] std::uint32_t RotateLeftAt(std::uint32_t node) {
        const auto replacement = Right(node);
        RotateLeft(node);
        return replacement;
    }
    [[nodiscard]] std::uint32_t RotateRightAt(std::uint32_t node) {
        const auto replacement = Left(node);
        RotateRight(node);
        return replacement;
    }

private:
    void ReplaceParentLink(std::uint32_t old_node, std::uint32_t replacement) {
        const auto parent = Parent(old_node);
        if (old_node == Root()) SetParent(header_, replacement);
        else if (old_node == Left(parent)) SetLeft(parent, replacement);
        else SetRight(parent, replacement);
    }
    void RotateLeft(std::uint32_t node) {
        const auto right = Right(node);
        SetRight(node, Left(right));
        if (Left(right) != GameManagerTreeInsert::kNilNode)
            SetParent(Left(right), node);
        SetParent(right, Parent(node));
        ReplaceParentLink(node, right);
        SetLeft(right, node);
        SetParent(node, right);
    }
    void RotateRight(std::uint32_t node) {
        const auto left = Left(node);
        SetLeft(node, Right(left));
        if (Right(left) != GameManagerTreeInsert::kNilNode)
            SetParent(Right(left), node);
        SetParent(left, Parent(node));
        ReplaceParentLink(node, left);
        SetRight(left, node);
        SetParent(node, left);
    }
    void Repair(std::uint32_t node) {
        while (node != Root() && Color(Parent(node)) == 0u) {
            const auto parent = Parent(node);
            const auto grandparent = Parent(parent);
            if (parent == Left(grandparent)) {
                const auto uncle = Right(grandparent);
                if (Color(uncle) == 0u) {
                    SetColor(parent, 1u);
                    SetColor(uncle, 1u);
                    SetColor(grandparent, 0u);
                    node = grandparent;
                } else {
                    if (node == Right(parent)) {
                        node = parent;
                        RotateLeft(node);
                    }
                    SetColor(Parent(node), 1u);
                    SetColor(Parent(Parent(node)), 0u);
                    RotateRight(Parent(Parent(node)));
                }
            } else {
                const auto uncle = Left(grandparent);
                if (Color(uncle) == 0u) {
                    SetColor(parent, 1u);
                    SetColor(uncle, 1u);
                    SetColor(grandparent, 0u);
                    node = grandparent;
                } else {
                    if (node == Left(parent)) {
                        node = parent;
                        RotateRight(node);
                    }
                    SetColor(Parent(node), 1u);
                    SetColor(Parent(Parent(node)), 0u);
                    RotateLeft(Parent(Parent(node)));
                }
            }
        }
        SetColor(Root(), 1u);
    }

    void RepairAfterRemove(std::uint32_t node) {
        while (node != Root() && Color(node) == 1u) {
            const auto parent = Parent(node);
            if (node == Left(parent)) {
                auto sibling = Right(parent);
                if (Color(sibling) == 0u) {
                    SetColor(sibling, 1u);
                    SetColor(parent, 0u);
                    RotateLeft(parent);
                    sibling = Right(Parent(node));
                }
                if (Color(Left(sibling)) == 1u && Color(Right(sibling)) == 1u) {
                    SetColor(sibling, 0u);
                    node = Parent(node);
                } else {
                    if (Color(Right(sibling)) == 1u) {
                        SetColor(Left(sibling), 1u);
                        SetColor(sibling, 0u);
                        RotateRight(sibling);
                        sibling = Right(Parent(node));
                    }
                    SetColor(sibling, Color(Parent(node)));
                    SetColor(Parent(node), 1u);
                    SetColor(Right(sibling), 1u);
                    RotateLeft(Parent(node));
                    break;
                }
            } else {
                auto sibling = Left(parent);
                if (Color(sibling) == 0u) {
                    SetColor(sibling, 1u);
                    SetColor(parent, 0u);
                    RotateRight(parent);
                    sibling = Left(Parent(node));
                }
                if (Color(Right(sibling)) == 1u && Color(Left(sibling)) == 1u) {
                    SetColor(sibling, 0u);
                    node = Parent(node);
                } else {
                    if (Color(Left(sibling)) == 1u) {
                        SetColor(Right(sibling), 1u);
                        SetColor(sibling, 0u);
                        RotateLeft(sibling);
                        sibling = Left(Parent(node));
                    }
                    SetColor(sibling, Color(Parent(node)));
                    SetColor(Parent(node), 1u);
                    SetColor(Left(sibling), 1u);
                    RotateRight(Parent(node));
                    break;
                }
            }
        }
        SetColor(node, 1u);
    }

    std::uint32_t address_;
    std::uint32_t header_;
    IGameManagerTreeMemory& memory_;
};

}  // namespace

GameManagerTreeInsertResult GameManagerTreeInsert::Insert(
    const std::uint32_t tree, const std::int32_t key,
    const std::uint32_t value, IGameManagerTreeMemory& memory) {
    Tree nodes(tree, memory);
    auto parent = nodes.Header();
    auto child = nodes.Root();
    bool went_left = true;
    while (child != kNilNode) {
        parent = child;
        went_left = key < nodes.Key(child);
        child = went_left ? nodes.Left(child) : nodes.Right(child);
    }
    if (memory.ReadByte(tree + 8u) == 0u) {
        auto candidate = parent;
        if (went_left) {
            if (parent == nodes.Left(nodes.Header()))
                return {InsertAt(tree, child, parent, key, value, memory), true};
            candidate = nodes.Predecessor(parent);
        }
        if (nodes.Key(candidate) >= key) return {candidate, false};
        return {InsertAt(tree, child, parent, key, value, memory), true};
    }
    return {nodes.InsertNode(child, parent, key, value), true};
}

std::uint32_t GameManagerTreeInsert::InsertAt(
    const std::uint32_t tree, const std::uint32_t child,
    const std::uint32_t parent, const std::int32_t key,
    const std::uint32_t value, IGameManagerTreeMemory& memory) {
    return Tree(tree, memory).InsertNode(child, parent, key, value);
}

std::uint32_t GameManagerTreeRemove::Remove(
    const std::uint32_t tree, const std::uint32_t node,
    IGameManagerTreeMemory& memory) {
    return Tree(tree, memory).RemoveNode(node);
}

std::uint32_t GameManagerTreeCursor::Next(
    std::uint32_t node, const IGameManagerTreeMemory& memory) {
    auto right = memory.ReadDword(node + 8u);
    if (right != GameManagerTreeInsert::kNilNode) {
        node = right;
        while (memory.ReadDword(node) != GameManagerTreeInsert::kNilNode)
            node = memory.ReadDword(node);
        return node;
    }
    auto parent = memory.ReadDword(node + 4u);
    while (node == memory.ReadDword(parent + 8u)) {
        node = parent;
        parent = memory.ReadDword(parent + 4u);
    }
    if (memory.ReadDword(node + 8u) != parent) node = parent;
    return node;
}

std::uint32_t GameManagerTreeCursor::Previous(
    std::uint32_t& cursor, const IGameManagerTreeMemory& memory) {
    const auto node = cursor;
    const auto parent = memory.ReadDword(node + 4u);
    if (memory.ReadDword(node + 20u) == 0u &&
        memory.ReadDword(parent + 4u) == node) {
        cursor = memory.ReadDword(node + 8u);
        return cursor;
    }
    auto left = memory.ReadDword(node);
    if (left != GameManagerTreeInsert::kNilNode) {
        auto right = memory.ReadDword(left + 8u);
        while (right != GameManagerTreeInsert::kNilNode) {
            left = right;
            right = memory.ReadDword(left + 8u);
        }
        cursor = left;
        return right;
    }
    auto ancestor = parent;
    while (cursor == memory.ReadDword(ancestor)) {
        cursor = ancestor;
        ancestor = memory.ReadDword(ancestor + 4u);
    }
    cursor = ancestor;
    return ancestor;
}

std::uint32_t GameManagerTreeLookup::LowerBound(
    const std::uint32_t tree, const std::int32_t key,
    const IGameManagerTreeMemory& memory) {
    auto candidate = memory.ReadDword(tree + 4u);
    auto node = memory.ReadDword(candidate + 4u);
    while (node != GameManagerTreeInsert::kNilNode) {
        if (static_cast<std::int32_t>(memory.ReadDword(node + 12u)) >= key) {
            candidate = node;
            node = memory.ReadDword(node);
        } else {
            node = memory.ReadDword(node + 8u);
        }
    }
    return candidate;
}

std::uint32_t GameManagerTreeRotation::Left(
    const std::uint32_t tree, const std::uint32_t node,
    IGameManagerTreeMemory& memory) {
    return Tree(tree, memory).RotateLeftAt(node);
}

std::uint32_t GameManagerTreeRotation::Right(
    const std::uint32_t tree, const std::uint32_t node,
    IGameManagerTreeMemory& memory) {
    return Tree(tree, memory).RotateRightAt(node);
}

GameManagerTreeMemoryStub::GameManagerTreeMemoryStub(const bool allow_duplicates) {
    words_[kTree + 4u] = kHeader;
    words_[kTree + 8u] = allow_duplicates ? 1u : 0u;
    words_[kTree + 12u] = 0u;
    words_[kHeader] = kHeader;
    words_[kHeader + 4u] = GameManagerTreeInsert::kNilNode;
    words_[kHeader + 8u] = kHeader;
    words_[GameManagerTreeInsert::kNilNode + 20u] = 1u;
}

std::uint32_t GameManagerTreeMemoryStub::AllocateNode() {
    const auto node = next_node_;
    next_node_ += 0x20u;
    return node;
}

void GameManagerTreeMemoryStub::FreeNode(const std::uint32_t node) {
    freed_nodes_.push_back(node);
    for (std::uint32_t offset = 0u; offset < 24u; offset += 4u)
        words_.erase(node + offset);
}

std::uint32_t GameManagerTreeMemoryStub::ReadDword(const std::uint32_t address) const {
    const auto entry = words_.find(address);
    return entry == words_.end() ? 0u : entry->second;
}

std::uint8_t GameManagerTreeMemoryStub::ReadByte(const std::uint32_t address) const {
    const auto word = ReadDword(address & ~3u);
    return static_cast<std::uint8_t>(word >> ((address & 3u) * 8u));
}

void GameManagerTreeMemoryStub::WriteDword(
    const std::uint32_t address, const std::uint32_t value) {
    words_[address] = value;
}

}  // namespace pal4
