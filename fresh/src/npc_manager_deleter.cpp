#include "pal4/npc_manager_deleter.h"
#include "monster_manager_temporary_text.h"

namespace pal4 {

void NpcManagerDeleteRuntimeStub::CleanupNpc(std::uint32_t) {}
void NpcManagerDeleteRuntimeStub::DestroyNpc(
    std::uint32_t, std::uint32_t) {}
void NpcManagerDeleteRuntimeStub::RemoveNode(
    std::uint32_t, std::uint32_t) {}

bool NpcManagerDeleter::DeleteByName(
    std::uint32_t manager, std::uint32_t name_address,
    INpcManagerDeleteRuntime& runtime) {
    ModeStringBufferRecord name{};
    name.mode_word = name_address & 0xffu;
    runtime.SetErrorFlag(name, 0u);
    MonsterManagerTemporaryText name_owner{name, runtime};
    const auto length = runtime.CStringLength(name_address);
    if (runtime.AllocateText(name, length, 1u)) {
        runtime.CopyBytes(name.buffer.data, name_address, length);
        name.buffer.length = length;
        runtime.WriteByte(name.buffer.data + length, 0u);
    }

    const auto tree = manager + 0x28u;
    const auto candidate = runtime.SearchNodeAlternative(tree, name);
    const auto header = runtime.ReadDword(tree + 4u);
    const auto node = candidate == header ||
                              runtime.CompareName(name, candidate + 0x0cu) < 0
                          ? header : candidate;
    if (node == runtime.ReadDword(manager + 0x2cu)) {
        name_owner.Release();
        return false;
    }

    const auto npc = runtime.ReadDword(node + 0x1cu);
    runtime.CleanupNpc(npc);
    runtime.DestroyNpc(npc, 1u);
    runtime.RemoveNode(tree, node);
    name_owner.Release();
    return true;
}

}  // namespace pal4
