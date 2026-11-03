#include "pal4/monster_behavior_tree.h"

namespace pal4 {
namespace {

class BehaviorTreeNodes final {
public:
    BehaviorTreeNodes(MonsterBehaviorTreeState& state,
                      IMonsterBehaviorTreeRuntime& runtime)
        : state_(state), runtime_(runtime),
          header_(state.words[2]), nil_(runtime.SharedNil()) {}

    std::uint32_t Remove(std::uint32_t node) {
        auto next = node;
        (void)MonsterBehaviorTree::AdvanceSuccessor(next, runtime_);
        auto detached = node;
        if (Left(node) != nil_ && Right(node) != nil_) {
            detached = Right(node);
            while (Left(detached) != nil_)
                detached = Left(detached);
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
            const auto color = Color(detached);
            SetColor(detached, Color(node));
            SetColor(node, color);
        } else {
            SetParent(child, Parent(node));
            ReplaceParentLink(node, child);
            if (Left(header_) == node) {
                if (Right(node) == nil_) {
                    SetLeft(header_, Parent(node));
                } else {
                    auto leftmost = child;
                    while (Left(leftmost) != nil_)
                        leftmost = Left(leftmost);
                    SetLeft(header_, leftmost);
                }
            }
            if (Right(header_) == node) {
                if (Left(node) == nil_) {
                    SetRight(header_, Parent(node));
                } else {
                    auto rightmost = child;
                    while (Right(rightmost) != nil_)
                        rightmost = Right(rightmost);
                    SetRight(header_, rightmost);
                }
            }
        }

        if (Color(node) == 1u)
            RepairAfterRemove(child);
        (void)runtime_.FreeMemory(node);
        --state_.words[4];
        return next;
    }

private:
    std::uint32_t Read(std::uint32_t address) const {
        return runtime_.ReadDword(address);
    }
    void Write(std::uint32_t address, std::uint32_t value) {
        runtime_.WriteDword(address, value);
    }
    std::uint32_t Left(std::uint32_t node) const { return Read(node); }
    std::uint32_t Parent(std::uint32_t node) const { return Read(node + 4u); }
    std::uint32_t Right(std::uint32_t node) const { return Read(node + 8u); }
    std::uint32_t Color(std::uint32_t node) const { return Read(node + 20u); }
    std::uint32_t Root() const { return Parent(header_); }
    void SetLeft(std::uint32_t node, std::uint32_t child) {
        Write(node, child);
    }
    void SetParent(std::uint32_t node, std::uint32_t parent) {
        Write(node + 4u, parent);
    }
    void SetRight(std::uint32_t node, std::uint32_t child) {
        Write(node + 8u, child);
    }
    void SetColor(std::uint32_t node, std::uint32_t color) {
        Write(node + 20u, color);
    }
    void ReplaceParentLink(std::uint32_t old_node,
                           std::uint32_t replacement) {
        const auto parent = Parent(old_node);
        if (old_node == Root())
            SetParent(header_, replacement);
        else if (old_node == Left(parent))
            SetLeft(parent, replacement);
        else
            SetRight(parent, replacement);
    }
    void RotateLeft(std::uint32_t node) {
        const auto replacement = Right(node);
        SetRight(node, Left(replacement));
        if (Left(replacement) != nil_)
            SetParent(Left(replacement), node);
        SetParent(replacement, Parent(node));
        ReplaceParentLink(node, replacement);
        SetLeft(replacement, node);
        SetParent(node, replacement);
    }
    void RotateRight(std::uint32_t node) {
        const auto replacement = Left(node);
        SetLeft(node, Right(replacement));
        if (Right(replacement) != nil_)
            SetParent(Right(replacement), node);
        SetParent(replacement, Parent(node));
        ReplaceParentLink(node, replacement);
        SetRight(replacement, node);
        SetParent(node, replacement);
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
                if (Color(Left(sibling)) == 1u &&
                    Color(Right(sibling)) == 1u) {
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
                if (Color(Right(sibling)) == 1u &&
                    Color(Left(sibling)) == 1u) {
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

    MonsterBehaviorTreeState& state_;
    IMonsterBehaviorTreeRuntime& runtime_;
    std::uint32_t header_;
    std::uint32_t nil_;
};

}  // namespace

std::uint32_t& MonsterBehaviorTree::RemoveNode(
    MonsterBehaviorTreeState& state, const std::uint32_t node_address,
    std::uint32_t& output, IMonsterBehaviorTreeRuntime& runtime) {
    output = BehaviorTreeNodes(state, runtime).Remove(node_address);
    return output;
}

}  // namespace pal4
