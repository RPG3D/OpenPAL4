#include "pal4/binary_tree_iterator.h"

namespace pal4 {

BinaryTreeIterator& BinaryTreeIterator::Initialize(
    BinaryTreeNode* root) noexcept {
    root_ = root;
    static_cast<void>(SetTraversal(true));
    return *this;
}

BinaryTreeIterator& BinaryTreeIterator::InitializeWithPreferredLeaf(
    BinaryTreeNode* root) noexcept {
    root_ = root;
    current_ = nullptr;
    static_cast<void>(InitializeAtPreferredLeaf());
    return *this;
}

BinaryTreeNode* BinaryTreeIterator::InitializeAtPreferredLeaf() noexcept {
    current_ = FindLeftPreferredLeaf(root_);
    return current_;
}

BinaryTreeNode* BinaryTreeIterator::SetTraversal(const bool forward) noexcept {
    current_ = forward ? TraverseListForward(root_)
                       : TraverseListBackward(root_);
    return current_;
}

BinaryTreeNode* BinaryTreeIterator::Current() const noexcept {
    return current_;
}

bool BinaryTreeIterator::IsAtEnd() const noexcept {
    return current_ == nullptr;
}

BinaryTreeNode* BinaryTreeIterator::RemoveElement() noexcept {
    auto* node = current_;
    if (node == nullptr) return nullptr;

    if (node->right != nullptr) {
        current_ = TraverseListForward(node->right);
        return current_;
    }

    auto* parent = node->parent;
    if (parent != nullptr && parent->left == node) {
        current_ = parent;
        return current_;
    }

    while (current_->parent != nullptr &&
           current_->parent->right == current_) {
        current_ = current_->parent;
    }
    auto* result = current_;
    current_ = current_->parent;
    return result;
}

BinaryTreeNode* BinaryTreeIterator::RemoveElementThunk(
    const int /*ignored*/) noexcept {
    return RemoveElement();
}

BinaryTreeNode* BinaryTreeIterator::RemoveNextElement() noexcept {
    auto* node = current_;
    if (node == nullptr) return nullptr;

    auto* parent = node->parent;
    if (parent != nullptr && parent->left == node &&
        parent->right != nullptr) {
        current_ = FindLeftPreferredLeaf(parent->right);
    } else {
        current_ = parent;
    }
    return current_;
}

BinaryTreeNode* BinaryTreeIterator::RemoveNextElementThunk(
    const int /*ignored*/) noexcept {
    return RemoveNextElement();
}

BinaryTreeNode* BinaryTreeIterator::TraverseListForward(
    BinaryTreeNode* root) noexcept {
    if (root == nullptr) return nullptr;

    auto* node = root;
    while (node->left != nullptr) node = node->left;
    return node;
}

BinaryTreeNode* BinaryTreeIterator::TraverseListBackward(
    BinaryTreeNode* root) noexcept {
    if (root == nullptr) return nullptr;

    auto* node = root;
    while (node->right != nullptr) node = node->right;
    return node;
}

BinaryTreeNode* BinaryTreeIterator::FindLeftPreferredLeaf(
    BinaryTreeNode* root) noexcept {
    auto* node = root;
    while (node != nullptr && (node->left != nullptr || node->right != nullptr)) {
        node = node->left != nullptr ? node->left : node->right;
    }
    return node;
}

}  // namespace pal4
