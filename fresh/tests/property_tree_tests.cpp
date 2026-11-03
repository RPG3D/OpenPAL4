#include "pal4/property_tree.h"

#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class IntegerComparator final : public pal4::IPropertyTreeComparator {
public:
    [[nodiscard]] int Compare(const std::int32_t query,
                              const std::int32_t node_key) const override {
        calls.emplace_back(query, node_key);
        return (query > node_key) - (query < node_key);
    }

    mutable std::vector<std::pair<std::int32_t, std::int32_t>> calls;
};

void EmptyTreeReturnsNotFound() {
    const pal4::PropertyTree tree;
    const IntegerComparator comparator;
    const auto result = tree.Search(7, comparator);
    Check(result.status == pal4::TreeSearchStatus::NotFound &&
              result.node == nullptr,
          "empty root returns null");
    Check(comparator.calls.empty(), "empty root does not call comparator");
}

void ExactMatchReturnsCurrentNode() {
    pal4::PropertyTreeNode root{17, nullptr};
    const pal4::PropertyTree tree(&root);
    const IntegerComparator comparator;
    const auto result = tree.Search(17, comparator);
    Check(result.status == pal4::TreeSearchStatus::Found &&
              result.node == &root,
          "zero comparison returns matching node");
    Check(comparator.calls ==
              std::vector<std::pair<std::int32_t, std::int32_t>>{{17, 17}},
          "comparison receives query then node key");
}

void NonnegativeComparisonAdvancesRight() {
    pal4::PropertyTreeNode match{20, nullptr};
    pal4::PropertyTreeNode root{10, &match};
    const pal4::PropertyTree tree(&root);
    const IntegerComparator comparator;
    const auto result = tree.Search(20, comparator);
    Check(result.status == pal4::TreeSearchStatus::Found &&
              result.node == &match,
          "positive comparison follows node offset +4");
    Check(comparator.calls.size() == 2,
          "comparator runs once at each visited node");

    pal4::PropertyTreeNode right{30, nullptr};
    match.right = &right;
    const auto missing = tree.Search(35, comparator);
    Check(missing.status == pal4::TreeSearchStatus::NotFound &&
              missing.node == nullptr,
          "right chain ending at null returns not found");
}

void NegativeComparisonAdvancesLeft() {
    pal4::PropertyTreeNode match{10, nullptr};
    pal4::PropertyTreeNode root{20, nullptr, &match};
    const pal4::PropertyTree tree(&root);
    const IntegerComparator comparator;
    const auto result = tree.Search(10, comparator);
    Check(result.status == pal4::TreeSearchStatus::Found &&
              result.node == &match,
          "negative comparison follows the left child");
    Check(comparator.calls ==
              std::vector<std::pair<std::int32_t, std::int32_t>>{{10, 20}, {10, 10}},
          "left traversal compares the query with each visited key");

    comparator.calls.clear();
    const auto missing = tree.Search(5, comparator);
    Check(missing.status == pal4::TreeSearchStatus::NotFound &&
              missing.node == nullptr,
          "left chain ending at null returns not found");
    Check(comparator.calls ==
              std::vector<std::pair<std::int32_t, std::int32_t>>{{5, 20}, {5, 10}},
          "left miss visits each node once");
}

}  // namespace

int main() {
    EmptyTreeReturnsNotFound();
    ExactMatchReturnsCurrentNode();
    NonnegativeComparisonAdvancesRight();
    NegativeComparisonAdvancesLeft();
    std::cout << "4 property-tree tests passed\n";
}
