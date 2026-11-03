#include "pal4/npc_manager_creator.h"
#include "monster_manager_temporary_text.h"

#include <new>

namespace pal4 {

std::uint32_t NpcManagerCreateRuntimeStub::FindChild(
    std::uint32_t, const char*) { return 0u; }
void NpcManagerCreateRuntimeStub::ReadStringAttribute(
    std::uint32_t, const char*, const char*, ModeStringBufferRecord&) {}
std::uint32_t NpcManagerCreateRuntimeStub::EmptyTextAddress() { return 0u; }
void NpcManagerCreateRuntimeStub::SetDefaultAction(
    std::uint32_t, std::uint32_t) {}
void NpcManagerCreateRuntimeStub::SetupBehavior(
    std::uint32_t, std::uint32_t, std::uint32_t) {}

std::uint32_t NpcManagerCreator::Create(
    std::uint32_t manager, const NpcCreationArguments& arguments,
    INpcManagerCreateRuntime& runtime) {
    ModeStringBufferRecord name{};
    name.mode_word = arguments.name & 0xffu;
    runtime.SetErrorFlag(name, 0u);
    MonsterManagerTemporaryText name_owner{name, runtime};
    const auto length = runtime.CStringLength(arguments.name);
    if (runtime.AllocateText(name, length, 1u)) {
        runtime.CopyBytes(name.buffer.data, arguments.name, length);
        runtime.SetTextSize(name, length);
    }

    const auto tree = manager + 0x28u;
    if (runtime.SearchNode(tree, name) !=
        runtime.ReadDword(manager + 0x2cu)) {
        runtime.SetErrorFlag(name, 1u);
        name_owner.Disable();
        return 0u;
    }

    runtime.LoadScripts();
    const auto storage = runtime.AllocateObject(kObjectBytes);
    if (storage == 0u) throw std::bad_alloc{};
    std::uint32_t object{};
    try {
        object = runtime.ConstructObject(storage);
    } catch (...) {
        runtime.FreeObjectStorage(storage);
        throw;
    }
    runtime.CallSlot30(object, arguments.name,
                       arguments.config, arguments.enabled);
    runtime.CallSlot4(object, arguments.position, 0u);
    runtime.CallSlot8(object, arguments.facing, 0u);
    runtime.CleanupScripts();
    runtime.SetStateFlag(object,
        runtime.ReadDword(arguments.source_record + 0x3cu));

    const auto attributes = runtime.FindChild(
        runtime.ReadDword(arguments.source_record + 0x50u),
        "NPCINFO_BufferCache_Attr");
    if (attributes != 0u) {
        ModeStringBufferRecord action{};
        runtime.ReadStringAttribute(attributes,
            "NPCINFO_BufferCache_Attr_defaultAct", "C01", action);
        MonsterManagerTemporaryText action_owner{action, runtime};
        runtime.SetDefaultAction(object,
            action.buffer.data != 0u ? action.buffer.data :
                                      runtime.EmptyTextAddress());
        runtime.SetErrorFlag(action, 1u);
        action_owner.Disable();
    }

    ModeStringBufferRecord key{};
    key.mode_word = name.mode_word & 0xffu;
    runtime.SetErrorFlag(key, 0u);
    MonsterManagerTemporaryText key_owner{key, runtime};
    runtime.AssignName(key, name, 0u, 0xffffffffu);
    runtime.InsertNode(tree, key, object);
    key_owner.Release();

    runtime.WriteDword(object + 0x204u, manager + 0x38u);
    runtime.SetupBehavior(object, arguments.source_record, 1u);
    name_owner.Release();
    return object;
}

}  // namespace pal4
