#pragma once

#include <cstdint>

namespace pal4 {

struct PropertyTreeNode {
    std::int32_t key = 0;
    PropertyTreeNode* right = nullptr;
    PropertyTreeNode* left = nullptr;
};

class IPropertyTreeComparator {
public:
    virtual ~IPropertyTreeComparator() = default;
    [[nodiscard]] virtual int Compare(std::int32_t query,
                                      std::int32_t node_key) const = 0;
};

enum class TreeSearchStatus { Found, NotFound };

struct TreeSearchResult {
    TreeSearchStatus status;
    PropertyTreeNode* node;
};

class PropertyTree final {
public:
    explicit PropertyTree(PropertyTreeNode* root = nullptr) noexcept;

    [[nodiscard]] TreeSearchResult Search(
        std::int32_t query, const IPropertyTreeComparator& comparator) const;

private:
    PropertyTreeNode* root_;
};

}  // namespace pal4
