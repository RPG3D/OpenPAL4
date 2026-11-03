#include "pal4/ui_frame_manager_cursor_tree.h"

namespace pal4 {
namespace {

class TreeLinks final {
public:
    TreeLinks(const std::uint32_t tree,
              IUiFrameManagerCursorTreeRuntime& runtime)
        : tree_(tree), runtime_(runtime),
          head_(runtime.ReadWord(tree + 4u)), nil_(runtime.SharedNil()) {}

    [[nodiscard]] std::uint32_t Head() const { return head_; }
    [[nodiscard]] std::uint32_t Nil() const { return nil_; }
    [[nodiscard]] std::uint32_t Root() { return Read(head_ + 4u); }
    [[nodiscard]] std::uint32_t Left(std::uint32_t node) { return Read(node); }
    [[nodiscard]] std::uint32_t Parent(std::uint32_t node) {
        return Read(node + 4u);
    }
    [[nodiscard]] std::uint32_t Right(std::uint32_t node) {
        return Read(node + 8u);
    }
    [[nodiscard]] std::uint32_t Color(std::uint32_t node) {
        return Read(node + 0x20u);
    }
    void SetColor(std::uint32_t node, std::uint32_t color) {
        Write(node + 0x20u, color);
    }

    [[nodiscard]] std::uint32_t InsertDuplicate(std::uint32_t child,
                                                 std::uint32_t parent,
                                                 std::uint32_t key) {
        const auto node = runtime_.AllocateNode(tree_, parent, 0u);
        Write(node, nil_);
        Write(node + 8u, nil_);
        runtime_.CopyText(node + 0x0cu, key);
        Write(tree_ + 0x0cu, Read(tree_ + 0x0cu) + 1u);
        Link(node, child, parent, key);
        Repair(node);
        return node;
    }

private:
    [[nodiscard]] std::uint32_t Read(std::uint32_t address) {
        return runtime_.ReadWord(address);
    }
    void Write(std::uint32_t address, std::uint32_t value) {
        runtime_.WriteWord(address, value);
    }
    void Link(std::uint32_t node, std::uint32_t child,
              std::uint32_t parent, std::uint32_t key) {
        if (parent == head_ || child != nil_ ||
            runtime_.CompareText(key, parent + 0x0cu) < 0) {
            Write(parent, node);
            if (parent == head_) {
                Write(head_ + 4u, node);
                Write(head_ + 8u, node);
            } else if (parent == Left(head_)) {
                Write(head_, node);
            }
        } else {
            Write(parent + 8u, node);
            if (parent == Right(head_)) Write(head_ + 8u, node);
        }
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
                        runtime_.RotateLeft(tree_, node);
                    }
                    SetColor(Parent(node), 1u);
                    SetColor(Parent(Parent(node)), 0u);
                    runtime_.RotateRight(tree_, Parent(Parent(node)));
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
                        runtime_.RotateRight(tree_, node);
                    }
                    SetColor(Parent(node), 1u);
                    SetColor(Parent(Parent(node)), 0u);
                    runtime_.RotateLeft(tree_, Parent(Parent(node)));
                }
            }
        }
        SetColor(Root(), 1u);
    }

    std::uint32_t tree_;
    IUiFrameManagerCursorTreeRuntime& runtime_;
    std::uint32_t head_;
    std::uint32_t nil_;
};

}  // namespace

std::uint32_t UiFrameManagerCursorTree::InsertOrFind(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t key,
    IUiFrameManagerCursorTreeRuntime& runtime) {
    TreeLinks links(tree, runtime);
    auto parent = links.Head();
    auto child = links.Root();
    bool went_left = true;
    while (child != links.Nil()) {
        parent = child;
        auto text = runtime.ReadWord(child + 0x10u);
        const auto length = runtime.ReadWord(child + 0x14u);
        if (text == 0u) text = runtime.EmptyText();
        went_left = runtime.CompareRange(key, text, length) < 0;
        child = went_left ? links.Left(child) : links.Right(child);
    }

    if (runtime.ReadByte(tree + 8u) != 0u) {
        const auto node = links.InsertDuplicate(child, parent, key);
        runtime.WriteWord(output, node);
        runtime.WriteByte(output + 4u, 1u);
        return output;
    }

    auto candidate = parent;
    if (went_left) {
        if (parent == links.Left(links.Head())) {
            const auto node = runtime.InsertNode(tree, child, parent, key);
            runtime.WriteWord(output, node);
            runtime.WriteByte(output + 4u, 1u);
            return output;
        }
        candidate = runtime.Predecessor(candidate);
    }
    if (runtime.CompareText(candidate + 0x0cu, key) >= 0) {
        runtime.SetExistingResult(output, candidate);
        return output;
    }
    const auto node = runtime.InsertNode(tree, child, parent, key);
    runtime.WriteWord(output, node);
    runtime.WriteByte(output + 4u, 1u);
    return output;
}

std::uint32_t UiFrameManagerCursorTree::Find(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t key,
    IUiFrameManagerCursorTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto candidate = head;
    auto node = runtime.ReadWord(head + 4u);
    while (node != runtime.SharedNil()) {
        if (runtime.CompareText(node + 0x0cu, key) >= 0) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }

    if (candidate != head) {
        const auto node_length = runtime.ReadWord(candidate + 0x14u);
        auto node_text = runtime.ReadWord(candidate + 0x10u);
        if (node_text == 0u) node_text = runtime.EmptyText();
        const auto key_length = runtime.ReadWord(key + 8u);
        const auto key_text = runtime.ReadWord(key + 4u);
        const auto common = key_length < node_length
                                ? key_length : node_length;
        const auto comparison = runtime.CompareBytes(
            key_text, node_text, common);
        if (comparison < 0 ||
            (comparison == 0 && key_length < node_length))
            candidate = head;
    }
    runtime.WriteWord(output, candidate);
    return output;
}

}  // namespace pal4
