#include "pal4/cegui_map_operations.h"

namespace pal4 {
namespace {

class EraseTree final {
public:
    EraseTree(const std::uint32_t map, const std::uint32_t nil,
              ICeguiMapEraseRuntime& runtime)
        : map_(map), header_(runtime.ReadDword(map + 4u)),
          nil_(nil), runtime_(runtime) {}

    [[nodiscard]] std::uint32_t Header() const { return header_; }
    [[nodiscard]] std::uint32_t Nil() const { return nil_; }
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
    void DecrementSize() { Write(map_ + 0x0cu, Read(map_ + 0x0cu) - 1u); }

    [[nodiscard]] std::uint32_t Minimum(std::uint32_t node) const {
        while (Left(node) != nil_) node = Left(node);
        return node;
    }
    [[nodiscard]] std::uint32_t Maximum(std::uint32_t node) const {
        while (Right(node) != nil_) node = Right(node);
        return node;
    }

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
    [[nodiscard]] std::uint32_t Read(const std::uint32_t address) const {
        return runtime_.ReadDword(address);
    }
    void Write(const std::uint32_t address, const std::uint32_t value) {
        runtime_.WriteDword(address, value);
    }

    std::uint32_t map_;
    std::uint32_t header_;
    std::uint32_t nil_;
    ICeguiMapEraseRuntime& runtime_;
};

void RepairAfterBlackRemoval(EraseTree& tree, std::uint32_t child) {
    while (child != tree.Root() && tree.Color(child) == 1u) {
        const auto parent = tree.Parent(child);
        if (child == tree.Left(parent)) {
            auto sibling = tree.Right(parent);
            if (tree.Color(sibling) == 0u) {
                tree.SetColor(sibling, 1u);
                tree.SetColor(parent, 0u);
                tree.RotateLeft(parent);
                sibling = tree.Right(tree.Parent(child));
            }
            if (tree.Color(tree.Left(sibling)) == 1u &&
                tree.Color(tree.Right(sibling)) == 1u) {
                tree.SetColor(sibling, 0u);
                child = tree.Parent(child);
            } else {
                if (tree.Color(tree.Right(sibling)) == 1u) {
                    tree.SetColor(tree.Left(sibling), 1u);
                    tree.SetColor(sibling, 0u);
                    tree.RotateRight(sibling);
                    sibling = tree.Right(tree.Parent(child));
                }
                tree.SetColor(sibling, tree.Color(tree.Parent(child)));
                tree.SetColor(tree.Parent(child), 1u);
                tree.SetColor(tree.Right(sibling), 1u);
                tree.RotateLeft(tree.Parent(child));
                break;
            }
        } else {
            auto sibling = tree.Left(parent);
            if (tree.Color(sibling) == 0u) {
                tree.SetColor(sibling, 1u);
                tree.SetColor(parent, 0u);
                tree.RotateRight(parent);
                sibling = tree.Left(tree.Parent(child));
            }
            if (tree.Color(tree.Right(sibling)) == 1u &&
                tree.Color(tree.Left(sibling)) == 1u) {
                tree.SetColor(sibling, 0u);
                child = tree.Parent(child);
            } else {
                if (tree.Color(tree.Left(sibling)) == 1u) {
                    tree.SetColor(tree.Right(sibling), 1u);
                    tree.SetColor(sibling, 0u);
                    tree.RotateLeft(sibling);
                    sibling = tree.Left(tree.Parent(child));
                }
                tree.SetColor(sibling, tree.Color(tree.Parent(child)));
                tree.SetColor(tree.Parent(child), 1u);
                tree.SetColor(tree.Left(sibling), 1u);
                tree.RotateRight(tree.Parent(child));
                break;
            }
        }
    }
    tree.SetColor(child, 1u);
}

}  // namespace

std::uint32_t CeguiMapOperations::Erase(
    const std::uint32_t map, const std::uint32_t node,
    const CeguiTreeGlobals& globals, ICeguiMapEraseRuntime& runtime) {
    const auto next = runtime.Successor(node);
    EraseTree tree(map, globals.nil, runtime);
    auto removed = node;
    std::uint32_t child;

    if (tree.Left(node) == tree.Nil() || tree.Right(node) == tree.Nil()) {
        child = tree.Left(node) == tree.Nil()
                    ? tree.Right(node) : tree.Left(node);
        tree.SetParent(child, tree.Parent(node));
        tree.ReplaceParentLink(node, child);
        if (tree.Left(tree.Header()) == node) {
            tree.SetLeft(tree.Header(), tree.Right(node) == tree.Nil()
                                            ? tree.Parent(node)
                                            : tree.Minimum(child));
        }
        if (tree.Right(tree.Header()) == node) {
            tree.SetRight(tree.Header(), tree.Left(node) == tree.Nil()
                                             ? tree.Parent(node)
                                             : tree.Maximum(child));
        }
    } else {
        const auto successor = tree.Minimum(tree.Right(node));
        child = tree.Right(successor);
        tree.SetParent(tree.Left(node), successor);
        tree.SetLeft(successor, tree.Left(node));
        if (successor == tree.Right(node)) {
            tree.SetParent(child, successor);
        } else {
            tree.SetParent(child, tree.Parent(successor));
            tree.SetLeft(tree.Parent(successor), child);
            tree.SetRight(successor, tree.Right(node));
            tree.SetParent(tree.Right(node), successor);
        }
        tree.ReplaceParentLink(node, successor);
        tree.SetParent(successor, tree.Parent(node));
        const auto successor_color = tree.Color(successor);
        tree.SetColor(successor, tree.Color(node));
        tree.SetColor(node, successor_color);
    }

    if (tree.Color(removed) == 1u) RepairAfterBlackRemoval(tree, child);
    runtime.FreeNode(removed);
    tree.DecrementSize();
    return next;
}

}  // namespace pal4
