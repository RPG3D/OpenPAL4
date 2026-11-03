#include "pal4/monster_manager_cleanup.h"
#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {

std::uint32_t MonsterManagerCleanupRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
void MonsterManagerCleanupRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
void MonsterManagerCleanupRuntimeStub::CallMonsterSlot2c(std::uint32_t) {}
void MonsterManagerCleanupRuntimeStub::DeleteMonster(
    std::uint32_t, std::uint32_t) {}
void MonsterManagerCleanupRuntimeStub::CleanupTreeNodes(
    std::uint32_t, std::uint32_t) {}
void MonsterManagerCleanupRuntimeStub::ReleaseText(
    std::uint32_t, std::uint32_t) {}
void MonsterManagerCleanupRuntimeStub::FreeNode(std::uint32_t) {}
std::uint32_t MonsterManagerCleanupRuntimeStub::NextTreeNode(
    std::uint32_t) { return 0u; }
void MonsterManagerCleanupRuntimeStub::DeleteTreeNode(
    std::uint32_t, std::uint32_t) {}
void MonsterManagerCleanupRuntimeStub::CleanupAuxiliaryTree(std::uint32_t) {}
void MonsterManagerCleanupRuntimeStub::ClearGridElements(std::uint32_t) {}
void MonsterManagerCleanupRuntimeStub::GetPreviousGridElement(
    std::uint32_t) {}
std::uint32_t MonsterManagerCleanupRuntimeStub::ResetGrid(std::uint32_t) {
    return 0u;
}

std::uint32_t MonsterManagerCleanup::Run(
    std::uint32_t manager, const MonsterManagerTreeGlobals& globals,
    IMonsterManagerCleanupRuntime& runtime) {
    const auto tree = manager + 0x28u;
    const auto header = runtime.ReadDword(manager + 0x2cu);
    auto node = runtime.ReadDword(header);
    while (node != header) {
        const auto monster = runtime.ReadDword(node + 0x1cu);
        runtime.CallMonsterSlot2c(monster);
        runtime.DeleteMonster(monster, 1u);
        node = MonsterManagerTreeCursor::Successor(
            node, globals.nil_address,
            [&runtime](std::uint32_t address) {
                return runtime.ReadDword(address);
            }).next;
    }

    node = runtime.ReadDword(header);
    if (runtime.ReadDword(manager + 0x34u) != 0u) {
        auto root = runtime.ReadDword(header + 4u);
        while (root != globals.nil_address) {
            runtime.CleanupTreeNodes(tree, runtime.ReadDword(root + 8u));
            const auto next = runtime.ReadDword(root);
            runtime.ReleaseText(root + 0x0cu, 0u);
            runtime.FreeNode(root);
            root = next;
        }
        runtime.WriteDword(header + 4u, globals.nil_address);
        runtime.WriteDword(manager + 0x34u, 0u);
        runtime.WriteDword(header, header);
        runtime.WriteDword(header + 8u, header);
    } else {
        while (node != header) {
            const auto current = node;
            node = runtime.NextTreeNode(node);
            runtime.DeleteTreeNode(tree, current);
        }
    }

    runtime.CleanupAuxiliaryTree(manager);
    const auto grid = manager + 0x38u;
    runtime.ClearGridElements(grid);
    runtime.GetPreviousGridElement(grid);
    return runtime.ResetGrid(grid);
}

}  // namespace pal4
