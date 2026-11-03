#include "pal4/monster_manager_tree_walker.h"
#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {

std::uint32_t MonsterManagerTreeWalkRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
std::uint32_t MonsterManagerTreeWalkRuntimeStub::InvokeCallback(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
void MonsterManagerTreeWalkRuntimeStub::StopPursuit(
    std::uint32_t, float) {}
void MonsterManagerTreeWalkRuntimeStub::ClearStopPursuit(std::uint32_t) {}

std::uint32_t MonsterManagerTreeWalker::ForEach(
    std::uint32_t manager, std::uint32_t callback,
    std::uint32_t context,
    const MonsterManagerTreeGlobals& globals,
    IMonsterManagerTreeWalkRuntime& runtime) {
    const auto header = runtime.ReadDword(manager + 0x2cu);
    auto node = runtime.ReadDword(header);
    if (node == header) return header;

    std::uint32_t result = header;
    do {
        result = runtime.InvokeCallback(
            callback, runtime.ReadDword(node + 0x1cu), context);
        if (result == 0u) return 0u;
        const auto step = MonsterManagerTreeCursor::Successor(
            node, globals.nil_address,
            [&runtime](std::uint32_t address) {
                return runtime.ReadDword(address);
            });
        node = step.next;
        result = step.result;
    } while (node != runtime.ReadDword(manager + 0x2cu));
    return result;
}

std::uint32_t MonsterManagerTreeWalker::StopPursuitAll(
    std::uint32_t manager, float incoming_st0,
    const MonsterManagerTreeGlobals& globals,
    IMonsterManagerTreeWalkRuntime& runtime) {
    const auto header = runtime.ReadDword(manager + 0x2cu);
    auto node = runtime.ReadDword(header);
    if (node == header) return header;

    std::uint32_t result = header;
    do {
        runtime.StopPursuit(runtime.ReadDword(node + 0x1cu), incoming_st0);
        const auto step = MonsterManagerTreeCursor::Successor(
            node, globals.nil_address,
            [&runtime](std::uint32_t address) {
                return runtime.ReadDword(address);
            });
        node = step.next;
        result = step.result;
    } while (node != runtime.ReadDword(manager + 0x2cu));
    return result;
}

std::uint32_t MonsterManagerTreeWalker::ClearStopPursuitAll(
    std::uint32_t manager, const MonsterManagerTreeGlobals& globals,
    IMonsterManagerTreeWalkRuntime& runtime) {
    const auto header = runtime.ReadDword(manager + 0x2cu);
    auto node = runtime.ReadDword(header);
    if (node == header) return header;

    std::uint32_t result = header;
    do {
        runtime.ClearStopPursuit(runtime.ReadDword(node + 0x1cu));
        const auto step = MonsterManagerTreeCursor::Successor(
            node, globals.nil_address,
            [&runtime](std::uint32_t address) {
                return runtime.ReadDword(address);
            });
        node = step.next;
        result = step.result;
    } while (node != runtime.ReadDword(manager + 0x2cu));
    return result;
}

}  // namespace pal4
