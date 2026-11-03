#include "pal4/cegui_map_operations.h"

namespace pal4 {
namespace {

class InsertTree final {
public:
    InsertTree(const std::uint32_t map, const std::uint32_t nil,
               ICeguiMapInsertRuntime& runtime)
        : map_(map), header_(runtime.ReadDword(map + 4u)),
          nil_(nil), runtime_(runtime) {}

    [[nodiscard]] std::uint32_t AllocateNode() {
        return runtime_.Allocate(0x18u);
    }
    [[nodiscard]] std::uint32_t Header() const { return header_; }
    [[nodiscard]] std::uint32_t Nil() const { return nil_; }
    [[nodiscard]] std::uint32_t Read(const std::uint32_t address) const {
        return runtime_.ReadDword(address);
    }
    void Write(const std::uint32_t address, const std::uint32_t value) {
        runtime_.WriteDword(address, value);
    }
    [[nodiscard]] std::uint32_t Left(const std::uint32_t node) const {
        return Read(node);
    }
    [[nodiscard]] std::uint32_t Parent(const std::uint32_t node) const {
        return Read(node + 4u);
    }
    [[nodiscard]] std::uint32_t Right(const std::uint32_t node) const {
        return Read(node + 8u);
    }
    [[nodiscard]] std::uint32_t Color(const std::uint32_t node) const {
        return Read(node + 0x14u);
    }
    void SetLeft(const std::uint32_t node, const std::uint32_t value) {
        Write(node, value);
    }
    void SetParent(const std::uint32_t node, const std::uint32_t value) {
        Write(node + 4u, value);
    }
    void SetRight(const std::uint32_t node, const std::uint32_t value) {
        Write(node + 8u, value);
    }
    void SetColor(const std::uint32_t node, const std::uint32_t value) {
        Write(node + 0x14u, value);
    }
    [[nodiscard]] std::uint32_t Root() const { return Parent(header_); }
    void SetRoot(const std::uint32_t node) { SetParent(header_, node); }
    void IncrementSize() { Write(map_ + 0x0cu, Read(map_ + 0x0cu) + 1u); }

    void ReplaceParentLink(const std::uint32_t old_node,
                           const std::uint32_t replacement) {
        const auto parent = Parent(old_node);
        if (Root() == old_node) {
            SetRoot(replacement);
        } else if (Left(parent) == old_node) {
            SetLeft(parent, replacement);
        } else {
            SetRight(parent, replacement);
        }
    }

    void RotateLeft(const std::uint32_t node) {
        const auto right = Right(node);
        SetRight(node, Left(right));
        if (Left(right) != nil_) SetParent(Left(right), node);
        SetParent(right, Parent(node));
        ReplaceParentLink(node, right);
        SetLeft(right, node);
        SetParent(node, right);
    }

    void RotateRight(const std::uint32_t node) {
        const auto left = Left(node);
        SetLeft(node, Right(left));
        if (Right(left) != nil_) SetParent(Right(left), node);
        SetParent(left, Parent(node));
        ReplaceParentLink(node, left);
        SetRight(left, node);
        SetParent(node, left);
    }

private:
    std::uint32_t map_;
    std::uint32_t header_;
    std::uint32_t nil_;
    ICeguiMapInsertRuntime& runtime_;
};

void RepairAfterInsert(InsertTree& tree, std::uint32_t node) {
    while (node != tree.Root() && tree.Color(tree.Parent(node)) == 0u) {
        const auto parent = tree.Parent(node);
        const auto grandparent = tree.Parent(parent);
        if (parent == tree.Left(grandparent)) {
            const auto uncle = tree.Right(grandparent);
            if (tree.Color(uncle) == 0u) {
                tree.SetColor(parent, 1u);
                tree.SetColor(uncle, 1u);
                tree.SetColor(grandparent, 0u);
                node = grandparent;
            } else {
                if (node == tree.Right(parent)) {
                    node = parent;
                    tree.RotateLeft(node);
                }
                tree.SetColor(tree.Parent(node), 1u);
                tree.SetColor(tree.Parent(tree.Parent(node)), 0u);
                tree.RotateRight(tree.Parent(tree.Parent(node)));
            }
        } else {
            const auto uncle = tree.Left(grandparent);
            if (tree.Color(uncle) == 0u) {
                tree.SetColor(parent, 1u);
                tree.SetColor(uncle, 1u);
                tree.SetColor(grandparent, 0u);
                node = grandparent;
            } else {
                if (node == tree.Left(parent)) {
                    node = parent;
                    tree.RotateRight(node);
                }
                tree.SetColor(tree.Parent(node), 1u);
                tree.SetColor(tree.Parent(tree.Parent(node)), 0u);
                tree.RotateLeft(tree.Parent(tree.Parent(node)));
            }
        }
    }
    tree.SetColor(tree.Root(), 1u);
}

}  // namespace

std::uint32_t CeguiMapOperations::InsertNode(
    const std::uint32_t map, const std::uint32_t child,
    const std::uint32_t parent, const CeguiKeyMapping mapping,
    const CeguiTreeGlobals& globals, ICeguiMapInsertRuntime& runtime) {
    InsertTree tree(map, globals.nil, runtime);
    const auto node = tree.AllocateNode();
    tree.SetParent(node, parent);
    tree.SetColor(node, 0u);
    tree.SetLeft(node, tree.Nil());
    tree.SetRight(node, tree.Nil());
    tree.Write(node + 0x0cu, mapping.windows_code);
    tree.Write(node + 0x10u, mapping.cegui_code);
    tree.IncrementSize();

    if (parent == tree.Header() || child != tree.Nil() ||
        mapping.windows_code < tree.Read(parent + 0x0cu)) {
        tree.SetLeft(parent, node);
        if (parent == tree.Header()) {
            tree.SetRoot(node);
            tree.SetRight(tree.Header(), node);
        } else if (parent == tree.Left(tree.Header())) {
            tree.SetLeft(tree.Header(), node);
        }
    } else {
        tree.SetRight(parent, node);
        if (parent == tree.Right(tree.Header())) {
            tree.SetRight(tree.Header(), node);
        }
    }
    RepairAfterInsert(tree, node);
    return node;
}

}  // namespace pal4
