#include "pal4/npc_manager_tree.h"

namespace pal4 {

MonsterManagerTreeInsertResult NpcManagerTree::Insert(
    std::uint32_t tree, std::uint32_t name,
    IMonsterManagerTreeInsertRuntime& runtime) {
    return MonsterManagerTreeInsert::Insert(tree, name, runtime);
}

std::uint32_t NpcManagerTree::Remove(
    std::uint32_t tree, std::uint32_t node,
    IMonsterManagerTreeRemoveRuntime& runtime) {
    return MonsterManagerTreeRemove::Remove(tree, node, runtime);
}

std::uint32_t NpcManagerTree::Search(
    std::uint32_t tree, std::uint32_t name,
    IMonsterManagerTreeLookupRuntime& runtime) {
    return MonsterManagerTreeLookup::Search(tree, name, runtime);
}

MonsterManagerTreeStep NpcManagerTree::Successor(
    std::uint32_t node, std::uint32_t nil,
    IMonsterManagerTreeLookupRuntime& runtime) {
    return MonsterManagerTreeCursor::Successor(
        node, nil, [&runtime](std::uint32_t address) {
            return runtime.ReadDword(address);
        });
}

std::uint32_t NpcManagerTree::LowerBoundAlternative(
    std::uint32_t tree, std::uint32_t name,
    IMonsterManagerTreeLookupRuntime& runtime) {
    return MonsterManagerTreeLookup::LowerBoundAlternative(
        tree, name, runtime);
}

std::uint32_t NpcManagerTree::DestroySubtree(
    std::uint32_t root, IMonsterManagerTreeCleanupRuntime& runtime) {
    return MonsterManagerTreeCleanup::DestroySubtree(root, runtime);
}

std::uint32_t NpcManagerTree::InsertHelper(
    std::uint32_t tree, std::uint32_t child,
    std::uint32_t parent, std::uint32_t name,
    IMonsterManagerTreeInsertRuntime& runtime) {
    return MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, name, runtime);
}

void NpcManagerTree::AssignText(
    std::uint32_t destination, std::uint32_t source,
    IMonsterManagerTreeTextRuntime& runtime) {
    MonsterManagerTreeTextAssignment::Assign(
        destination, source, runtime);
}

}  // namespace pal4
