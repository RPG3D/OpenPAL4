#pragma once

namespace pal4 {

struct BinaryTreeNode {
    BinaryTreeNode* left = nullptr;
    BinaryTreeNode* right = nullptr;
    BinaryTreeNode* parent = nullptr;
};

class BinaryTreeIterator final {
public:
    [[nodiscard]] BinaryTreeIterator& Initialize(BinaryTreeNode* root) noexcept;
    [[nodiscard]] BinaryTreeIterator& InitializeWithPreferredLeaf(
        BinaryTreeNode* root) noexcept;
    [[nodiscard]] BinaryTreeNode* InitializeAtPreferredLeaf() noexcept;
    [[nodiscard]] BinaryTreeNode* SetTraversal(bool forward) noexcept;
    [[nodiscard]] BinaryTreeNode* Current() const noexcept;
    [[nodiscard]] bool IsAtEnd() const noexcept;
    [[nodiscard]] BinaryTreeNode* RemoveElement() noexcept;
    [[nodiscard]] BinaryTreeNode* RemoveElementThunk(int ignored) noexcept;
    [[nodiscard]] BinaryTreeNode* RemoveNextElement() noexcept;
    [[nodiscard]] BinaryTreeNode* RemoveNextElementThunk(
        int ignored) noexcept;

    [[nodiscard]] static BinaryTreeNode* TraverseListForward(
        BinaryTreeNode* root) noexcept;
    [[nodiscard]] static BinaryTreeNode* TraverseListBackward(
        BinaryTreeNode* root) noexcept;
    [[nodiscard]] static BinaryTreeNode* FindLeftPreferredLeaf(
        BinaryTreeNode* root) noexcept;

private:
    BinaryTreeNode* root_ = nullptr;
    BinaryTreeNode* current_ = nullptr;
};

}  // namespace pal4
