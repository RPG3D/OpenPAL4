#include "pal4/monster_behavior_tree.h"

namespace pal4 {
namespace {

class TreeInsertion final {
public:
    TreeInsertion(MonsterBehaviorTreeState& state,
                  IMonsterBehaviorTreeRuntime& runtime)
        : state_(state), runtime_(runtime), header_(state.words[2]),
          nil_(runtime.SharedNil()) {}

    std::uint32_t InsertAt(std::uint32_t child, std::uint32_t parent,
                           std::int32_t class_id, std::uint32_t factory) {
        const auto node = MonsterBehaviorTree::CreateNode(parent, 0u, runtime_);
        SetLeft(node, nil_);
        SetRight(node, nil_);
        Write(node + 12u, static_cast<std::uint32_t>(class_id));
        Write(node + 16u, factory);
        ++state_.words[4];

        if (parent == header_ || child != nil_ || class_id < Key(parent)) {
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

    std::uint32_t RotateLeft(std::uint32_t node) {
        const auto promoted = Right(node);
        SetRight(node, Left(promoted));
        if (Left(promoted) != nil_) SetParent(Left(promoted), node);
        SetParent(promoted, Parent(node));
        ReplaceParentLink(node, promoted);
        SetLeft(promoted, node);
        SetParent(node, promoted);
        return promoted;
    }

    std::uint32_t RotateRight(std::uint32_t node) {
        const auto promoted = Left(node);
        SetLeft(node, Right(promoted));
        if (Right(promoted) != nil_) SetParent(Right(promoted), node);
        SetParent(promoted, Parent(node));
        ReplaceParentLink(node, promoted);
        SetRight(promoted, node);
        SetParent(node, promoted);
        return promoted;
    }

    std::uint32_t Header() const { return header_; }
    std::uint32_t Root() const { return Parent(header_); }
    std::uint32_t Left(std::uint32_t node) const { return Read(node); }
    std::uint32_t Parent(std::uint32_t node) const { return Read(node + 4u); }
    std::uint32_t Right(std::uint32_t node) const { return Read(node + 8u); }
    std::int32_t Key(std::uint32_t node) const {
        return static_cast<std::int32_t>(Read(node + 12u));
    }
    std::uint32_t Read(std::uint32_t address) const {
        return runtime_.ReadDword(address);
    }

private:
    void Write(std::uint32_t address, std::uint32_t value) {
        runtime_.WriteDword(address, value);
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
    std::uint32_t Color(std::uint32_t node) const {
        return Read(node + 20u);
    }
    void SetColor(std::uint32_t node, std::uint32_t color) {
        Write(node + 20u, color);
    }
    void ReplaceParentLink(std::uint32_t node, std::uint32_t replacement) {
        const auto parent = Parent(node);
        if (node == Root()) SetParent(header_, replacement);
        else if (node == Left(parent)) SetLeft(parent, replacement);
        else SetRight(parent, replacement);
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

    MonsterBehaviorTreeState& state_;
    IMonsterBehaviorTreeRuntime& runtime_;
    std::uint32_t header_;
    std::uint32_t nil_;
};

}  // namespace

MonsterBehaviorTreeInsertResult MonsterBehaviorTree::Insert(
    MonsterBehaviorTreeState& state, std::int32_t class_id,
    std::uint32_t factory, IMonsterBehaviorTreeRuntime& runtime) {
    TreeInsertion tree(state, runtime);
    auto parent = tree.Header();
    auto child = tree.Root();
    bool went_left = true;
    while (child != runtime.SharedNil()) {
        parent = child;
        went_left = class_id < tree.Key(child);
        child = went_left ? tree.Left(child) : tree.Right(child);
    }
    if ((state.words[3] & 0xffu) == 0u) {
        auto candidate = parent;
        if (went_left) {
            if (parent == tree.Left(tree.Header()))
                return {tree.InsertAt(child, parent, class_id, factory), true};
            (void)AdvancePredecessor(candidate, runtime);
        }
        if (tree.Key(candidate) >= class_id) return {candidate, false};
    }
    return {tree.InsertAt(child, parent, class_id, factory), true};
}

std::uint32_t MonsterBehaviorTree::RotateLeft(
    MonsterBehaviorTreeState& state, std::uint32_t node,
    IMonsterBehaviorTreeRuntime& runtime) {
    return TreeInsertion(state, runtime).RotateLeft(node);
}

std::uint32_t MonsterBehaviorTree::RotateRight(
    MonsterBehaviorTreeState& state, std::uint32_t node,
    IMonsterBehaviorTreeRuntime& runtime) {
    return TreeInsertion(state, runtime).RotateRight(node);
}

std::uint32_t MonsterBehaviorTree::CreateNode(
    std::uint32_t parent, std::uint32_t color,
    IMonsterBehaviorTreeRuntime& runtime) {
    const auto node = runtime.AllocateMemory(0x18u);
    runtime.WriteDword(node + 4u, parent);
    runtime.WriteDword(node + 20u, color);
    return node;
}

std::uint32_t& MonsterBehaviorTree::InsertNodeAt(
    MonsterBehaviorTreeState& state, std::uint32_t child,
    std::uint32_t parent, std::int32_t class_id,
    std::uint32_t factory, std::uint32_t& output,
    IMonsterBehaviorTreeRuntime& runtime) {
    output = TreeInsertion(state, runtime).InsertAt(
        child, parent, class_id, factory);
    return output;
}

std::uint32_t MonsterBehaviorTree::AdvancePredecessor(
    std::uint32_t& cursor, IMonsterBehaviorTreeRuntime& runtime) {
    const auto node = cursor;
    const auto nil = runtime.SharedNil();
    if (runtime.ReadDword(node + 20u) == 0u &&
        runtime.ReadDword(runtime.ReadDword(node + 4u) + 4u) == node) {
        cursor = runtime.ReadDword(node + 8u);
        return cursor;
    }
    auto previous = runtime.ReadDword(node);
    if (previous != nil) {
        auto right = runtime.ReadDword(previous + 8u);
        while (right != nil) {
            previous = right;
            right = runtime.ReadDword(previous + 8u);
        }
        cursor = previous;
        return right;
    }
    previous = runtime.ReadDword(node + 4u);
    while (cursor == runtime.ReadDword(previous)) {
        cursor = previous;
        previous = runtime.ReadDword(previous + 4u);
    }
    cursor = previous;
    return previous;
}

}  // namespace pal4
