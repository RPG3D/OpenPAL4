#include "pal4/binary_tree_iterator.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void InitializeSelectsLeftmostNode() {
    pal4::BinaryTreeNode leftmost;
    pal4::BinaryTreeNode left;
    pal4::BinaryTreeNode root;
    root.left = &left;
    left.left = &leftmost;

    pal4::BinaryTreeIterator iterator;
    Check(&iterator.Initialize(&root) == &iterator,
          "initializer returns the iterator object");
    Check(iterator.Current() == &leftmost,
          "default forward traversal selects the leftmost node");
}

void TraversalHelpersStopAtNullLinks() {
    pal4::BinaryTreeNode left;
    pal4::BinaryTreeNode right;
    pal4::BinaryTreeNode root;
    root.left = &left;
    root.right = &right;

    Check(pal4::BinaryTreeIterator::TraverseListForward(&root) == &left,
          "forward helper follows offset-zero links");
    Check(pal4::BinaryTreeIterator::TraverseListBackward(&root) == &right,
          "backward helper follows offset-four links");
    Check(pal4::BinaryTreeIterator::TraverseListForward(nullptr) == nullptr &&
              pal4::BinaryTreeIterator::TraverseListBackward(nullptr) == nullptr,
          "both helpers accept a null root");
}

void ReverseSelectorSelectsRightmostNode() {
    pal4::BinaryTreeNode root;
    pal4::BinaryTreeNode right;
    root.right = &right;
    pal4::BinaryTreeIterator iterator;
    static_cast<void>(iterator.Initialize(&root));
    Check(iterator.SetTraversal(false) == &right,
          "reverse branch passes the root and selects its rightmost node");
    Check(iterator.Current() == &right,
          "reverse branch stores the selected node");
    Check(iterator.SetTraversal(true) == &root,
          "forward branch selects the leftmost node again");

    pal4::BinaryTreeIterator empty;
    Check(&empty.Initialize(nullptr) == &empty &&
              empty.SetTraversal(false) == nullptr,
          "both selectors accept an empty root");
}

void RemoveElementSelectsLeftmostNodeInRightSubtree() {
    pal4::BinaryTreeNode next;
    pal4::BinaryTreeNode right;
    pal4::BinaryTreeNode current;
    current.right = &right;
    right.left = &next;

    pal4::BinaryTreeIterator iterator;
    static_cast<void>(iterator.Initialize(&current));
    Check(iterator.RemoveElement() == &next,
          "right-subtree path selects its leftmost node");
    Check(iterator.Current() == &next, "stores the selected successor");
}

void RemoveElementAscendsFromLeftAndRightBranches() {
    pal4::BinaryTreeNode parent;
    pal4::BinaryTreeNode current;
    parent.left = &current;
    current.parent = &parent;
    pal4::BinaryTreeIterator left_iterator;
    static_cast<void>(left_iterator.Initialize(&current));
    Check(left_iterator.RemoveElement() == &parent,
          "left child advances directly to parent");

    pal4::BinaryTreeNode ancestor;
    pal4::BinaryTreeNode right_parent;
    pal4::BinaryTreeNode right_current;
    ancestor.left = &right_parent;
    right_parent.parent = &ancestor;
    right_parent.right = &right_current;
    right_current.parent = &right_parent;
    pal4::BinaryTreeIterator right_iterator;
    static_cast<void>(right_iterator.Initialize(&right_current));
    Check(right_iterator.RemoveElement() == &right_parent,
          "right-child chain returns the node before the final ascent");
    Check(right_iterator.Current() == &ancestor,
          "right-child chain stores the first left ancestor");
}

void RemoveElementReturnsNullAtEndAndThunkForwards() {
    pal4::BinaryTreeNode root;
    pal4::BinaryTreeIterator iterator;
    static_cast<void>(iterator.Initialize(&root));
    Check(iterator.RemoveElementThunk(0x1234) == &root,
          "thunk ignores its extra parameter and returns the prior root");
    Check(iterator.Current() == nullptr, "end is stored in current slot");

    pal4::BinaryTreeIterator empty;
    Check(empty.RemoveElement() == nullptr,
          "null current remains a null iterator");
}

void AlternateInitializersAndEndCheckMatchObservedFields() {
    pal4::BinaryTreeNode leaf;
    pal4::BinaryTreeNode root;
    root.left = &leaf;
    pal4::BinaryTreeIterator iterator;
    Check(&iterator.InitializeWithPreferredLeaf(&root) == &iterator,
          "0x4011f0 initializer returns the iterator");
    Check(iterator.Current() == &leaf && !iterator.IsAtEnd(),
          "initializing the root immediately selects its preferred leaf");
    root.left = nullptr;
    Check(iterator.InitializeAtPreferredLeaf() == &root &&
              !iterator.IsAtEnd(),
          "helper refreshes current from the stored root");
    Check(&iterator.InitializeWithPreferredLeaf(nullptr) == &iterator &&
              iterator.Current() == nullptr && iterator.IsAtEnd(),
          "null root initializes to the end state");
}

void PreferredLeafUsesLeftBeforeRight() {
    pal4::BinaryTreeNode leaf;
    pal4::BinaryTreeNode right;
    pal4::BinaryTreeNode left;
    pal4::BinaryTreeNode root;
    root.left = &left;
    root.right = &right;
    left.right = &leaf;
    Check(pal4::BinaryTreeIterator::FindLeftPreferredLeaf(&root) == &leaf,
          "leaf helper takes left when available, then right");
    Check(pal4::BinaryTreeIterator::FindLeftPreferredLeaf(nullptr) == nullptr,
          "leaf helper accepts a null root");
}

void RemoveNextElementSelectsParentOrPreferredRightLeaf() {
    pal4::BinaryTreeNode next_leaf;
    pal4::BinaryTreeNode right_child;
    pal4::BinaryTreeNode right_subtree;
    pal4::BinaryTreeNode current;
    pal4::BinaryTreeNode parent;
    parent.left = &current;
    parent.right = &right_subtree;
    current.parent = &parent;
    right_subtree.left = &right_child;
    right_child.right = &next_leaf;
    pal4::BinaryTreeIterator iterator;
    static_cast<void>(iterator.InitializeWithPreferredLeaf(&parent));
    Check(iterator.RemoveNextElement() == &next_leaf,
          "left child with a right subtree advances to preferred leaf");

    pal4::BinaryTreeNode plain_parent;
    pal4::BinaryTreeNode plain_current;
    plain_parent.right = &plain_current;
    plain_current.parent = &plain_parent;
    static_cast<void>(iterator.InitializeWithPreferredLeaf(&plain_current));
    Check(iterator.RemoveNextElementThunk(99) == &plain_parent,
          "otherwise thunk advances directly to parent");
    Check(iterator.RemoveNextElement() == nullptr,
          "advancing from root clears current");
}

}  // namespace

int main() {
    InitializeSelectsLeftmostNode();
    TraversalHelpersStopAtNullLinks();
    ReverseSelectorSelectsRightmostNode();
    RemoveElementSelectsLeftmostNodeInRightSubtree();
    RemoveElementAscendsFromLeftAndRightBranches();
    RemoveElementReturnsNullAtEndAndThunkForwards();
    AlternateInitializersAndEndCheckMatchObservedFields();
    PreferredLeafUsesLeftBeforeRight();
    RemoveNextElementSelectsParentOrPreferredRightLeaf();
    std::cout << "9 binary-tree iterator tests passed\n";
}
