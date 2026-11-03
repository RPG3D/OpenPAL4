#include "pal4/npc_manager_object_processor.h"
#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {
namespace {

std::uint32_t ProcessVariant(
    std::uint32_t manager, std::uint32_t first,
    std::uint32_t second, std::uint32_t vtable_slot,
    const NpcManagerTreeGlobals& globals,
    INpcManagerProcessRuntime& runtime) {
    const auto header = runtime.ReadDword(manager + 0x2cu);
    auto cursor = runtime.ReadDword(header);
    auto result = header;
    while (cursor != header) {
        const auto npc = runtime.ReadDword(cursor + 0x1cu);
        if (runtime.ReadDword(npc + 0x1f0u) != 0u)
            runtime.Dispatch(npc, vtable_slot, first, second);
        const auto step = MonsterManagerTreeCursor::Successor(
            cursor, globals.nil_address,
            [&runtime](std::uint32_t address) {
                return runtime.ReadDword(address);
            });
        cursor = step.next;
        result = step.result;
    }
    return result;
}

}  // namespace

std::uint32_t NpcManagerProcessRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void NpcManagerProcessRuntimeStub::Dispatch(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t) {}
void NpcManagerProcessRuntimeStub::UpdateGrid(
    std::uint32_t, std::uint32_t) {}

std::uint32_t NpcManagerObjectProcessor::Process(
    std::uint32_t manager, std::uint32_t argument,
    const NpcManagerTreeGlobals& globals,
    INpcManagerProcessRuntime& runtime) {
    return ProcessVariant(manager, argument, 0u, 0x34u,
                          globals, runtime);
}

std::uint32_t NpcManagerObjectProcessor::ProcessAlternative(
    std::uint32_t manager, std::uint32_t argument,
    const NpcManagerTreeGlobals& globals,
    INpcManagerProcessRuntime& runtime) {
    return ProcessVariant(manager, argument, 0u, 0x24u,
                          globals, runtime);
}

std::uint32_t NpcManagerObjectProcessor::ProcessExtended(
    std::uint32_t manager, std::uint32_t first,
    std::uint32_t second,
    const NpcManagerTreeGlobals& globals,
    INpcManagerProcessRuntime& runtime) {
    runtime.UpdateGrid(manager + 0x38u, first);
    return ProcessVariant(manager, first, second, 0x38u,
                          globals, runtime);
}

std::uint32_t NpcManagerForEachRuntimeStub::ForEachNpc(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t NpcManagerForEach::Apply(
    std::uint32_t manager, std::uint32_t callback,
    std::uint32_t context,
    INpcManagerForEachRuntime& runtime) {
    return runtime.ForEachNpc(manager, callback, context);
}

std::uint32_t NpcManagerCallbackRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
std::uint32_t NpcManagerCallbackRuntimeStub::Invoke(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }

void NpcManagerCallbackWalker::Walk(
    std::uint32_t manager, std::uint32_t callback,
    std::uint32_t context, const NpcManagerTreeGlobals& globals,
    INpcManagerCallbackRuntime& runtime) {
    const auto header = runtime.ReadDword(manager + 0x2cu);
    auto node = runtime.ReadDword(header);
    while (node != header) {
        const auto npc = runtime.ReadDword(node + 0x1cu);
        if (runtime.Invoke(callback, npc, context) == 0u) return;
        node = MonsterManagerTreeCursor::Successor(
            node, globals.nil_address,
            [&runtime](std::uint32_t address) {
                return runtime.ReadDword(address);
            }).next;
    }
}

}  // namespace pal4
