#include "pal4/property_tree.h"

namespace pal4 {

PropertyTree::PropertyTree(PropertyTreeNode* root) noexcept : root_(root) {}

TreeSearchResult PropertyTree::Search(
    const std::int32_t query,
    const IPropertyTreeComparator& comparator) const {
    auto* node = root_;
    while (node != nullptr) {
        const auto comparison = comparator.Compare(query, node->key);
        if (comparison == 0) return {TreeSearchStatus::Found, node};
        node = comparison < 0 ? node->left : node->right;
    }
    return {TreeSearchStatus::NotFound, nullptr};
}

}  // namespace pal4
