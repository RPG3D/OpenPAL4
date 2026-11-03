#include "pal4/monster_manager_tree_insert.h"
#include "pal4/monster_manager_tree_cursor.h"
#include "pal4/monster_manager_tree_node_allocator.h"
#include "pal4/monster_manager_tree_rotation.h"

namespace pal4 {
namespace {

class TreeLinks final {
public:
    TreeLinks(std::uint32_t tree, IMonsterManagerTreeInsertRuntime& runtime)
        : tree_(tree), runtime_(runtime), header_(runtime.ReadDword(tree + 4u)),
          nil_(runtime.SharedNil()) {}

    std::uint32_t Header() const { return header_; }
    std::uint32_t Nil() const { return nil_; }
    std::uint32_t Root() const { return Parent(header_); }
    std::uint32_t Left(std::uint32_t node) const { return Read(node); }
    std::uint32_t Parent(std::uint32_t node) const { return Read(node + 4u); }
    std::uint32_t Right(std::uint32_t node) const { return Read(node + 8u); }
    std::uint32_t Color(std::uint32_t node) const {
        return Read(node + runtime_.ColorOffset());
    }

    std::uint32_t InsertAt(std::uint32_t child, std::uint32_t parent,
                           std::uint32_t input) {
        const auto node = MonsterManagerTreeNodeAllocator::Allocate(
            parent, 0u, runtime_, runtime_.NodeSize(), runtime_.ColorOffset());
        return LinkAndRepair(node, child, parent, input);
    }
    std::uint32_t InsertRawAt(std::uint32_t child, std::uint32_t parent,
                              std::uint32_t input) {
        const auto node = runtime_.AllocateMemory(runtime_.NodeSize());
        SetParent(node, parent);
        SetColor(node, 0u);
        return LinkAndRepair(node, child, parent, input);
    }

private:
    std::uint32_t LinkAndRepair(std::uint32_t node, std::uint32_t child,
                                std::uint32_t parent, std::uint32_t input) {
        SetLeft(node, nil_);
        SetRight(node, nil_);
        runtime_.AssignText(node + 0x0cu, input);
        Write(tree_ + 0x0cu, Read(tree_ + 0x0cu) + 1u);

        if (parent == header_ || child != nil_ ||
            runtime_.CompareText(input, parent + 0x0cu) < 0) {
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
    std::uint32_t Read(std::uint32_t address) const {
        return runtime_.ReadDword(address);
    }
    void Write(std::uint32_t address, std::uint32_t value) {
        runtime_.WriteDword(address, value);
    }
    void SetLeft(std::uint32_t node, std::uint32_t child) { Write(node, child); }
    void SetParent(std::uint32_t node, std::uint32_t parent) {
        Write(node + 4u, parent);
    }
    void SetRight(std::uint32_t node, std::uint32_t child) {
        Write(node + 8u, child);
    }
    void SetColor(std::uint32_t node, std::uint32_t color) {
        Write(node + runtime_.ColorOffset(), color);
    }
    // Conventional directions; the IDA entrypoint names are reversed.
    void RotateLeft(std::uint32_t node) {
        (void)MonsterManagerTreeRotation::RotateRight(tree_, node, runtime_);
    }
    void RotateRight(std::uint32_t node) {
        (void)MonsterManagerTreeRotation::RotateLeft(tree_, node, runtime_);
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

    std::uint32_t tree_;
    IMonsterManagerTreeInsertRuntime& runtime_;
    std::uint32_t header_;
    std::uint32_t nil_;
};

}  // namespace

MonsterManagerTreeInsertResult MonsterManagerTreeInsert::Insert(
    std::uint32_t tree_address, std::uint32_t input,
    IMonsterManagerTreeInsertRuntime& runtime) {
    TreeLinks tree(tree_address, runtime);
    auto parent = tree.Header();
    auto child = tree.Root();
    bool went_left = true;
    while (child != tree.Nil()) {
        parent = child;
        went_left = runtime.CompareRange(input, child + 0x0cu) < 0;
        child = went_left ? tree.Left(child) : tree.Right(child);
    }

    if (runtime.ReadByte(tree_address + 8u) != 0u)
        return {tree.InsertAt(child, parent, input), true};

    auto candidate = parent;
    if (went_left) {
        if (parent == tree.Left(tree.Header()))
            return {InsertHelper(tree_address, child, parent, input, runtime), true};
        candidate = MonsterManagerTreeCursor::Predecessor(
            candidate, tree.Nil(), [&](std::uint32_t address) {
                return runtime.ReadDword(address);
            }, runtime.ColorOffset()).next;
    }
    if (runtime.CompareText(candidate + 0x0cu, input) >= 0) {
        MonsterManagerTreeInsertResult result{};
        runtime.SetNodeData(result, candidate);
        result.inserted = false;
        return result;
    }
    return {InsertHelper(tree_address, child, parent, input, runtime), true};
}

std::uint32_t MonsterManagerTreeInsert::InsertHelper(
    std::uint32_t tree, std::uint32_t child, std::uint32_t parent,
    std::uint32_t input, IMonsterManagerTreeInsertRuntime& runtime) {
    return TreeLinks(tree, runtime).InsertRawAt(child, parent, input);
}

}  // namespace pal4
