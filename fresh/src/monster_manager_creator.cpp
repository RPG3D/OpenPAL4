#include "pal4/monster_manager_creator.h"
#include "monster_manager_temporary_text.h"

#include <new>

namespace pal4 {

std::uint32_t MonsterManagerCreateRuntimeStub::CStringLength(
    std::uint32_t) { return 0u; }
void MonsterManagerCreateRuntimeStub::SetErrorFlag(
    ModeStringBufferRecord& text, std::uint8_t) { text.buffer = {}; }
bool MonsterManagerCreateRuntimeStub::AllocateText(
    ModeStringBufferRecord&, std::uint32_t, std::uint8_t) { return false; }
void MonsterManagerCreateRuntimeStub::CopyBytes(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void MonsterManagerCreateRuntimeStub::SetTextSize(
    ModeStringBufferRecord&, std::uint32_t) {}
std::uint32_t MonsterManagerCreateRuntimeStub::SearchNode(
    std::uint32_t, const ModeStringBufferRecord&) { return 0u; }
std::uint32_t MonsterManagerCreateRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
std::uint8_t MonsterManagerCreateRuntimeStub::ReadByte(
    std::uint32_t) { return 0u; }
void MonsterManagerCreateRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
void MonsterManagerCreateRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
void MonsterManagerCreateRuntimeStub::FreeTextHeader(std::uint32_t) {}
void MonsterManagerCreateRuntimeStub::LoadScripts() {}
void MonsterManagerCreateRuntimeStub::CleanupScripts() {}
std::uint32_t MonsterManagerCreateRuntimeStub::AllocateObject(
    std::uint32_t) { return 0u; }
void MonsterManagerCreateRuntimeStub::FreeObjectStorage(std::uint32_t) {}
std::uint32_t MonsterManagerCreateRuntimeStub::ConstructObject(
    std::uint32_t storage) { return storage; }
void MonsterManagerCreateRuntimeStub::CallSlot30(
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {}
void MonsterManagerCreateRuntimeStub::CallSlot4(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void MonsterManagerCreateRuntimeStub::CallSlot8(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void MonsterManagerCreateRuntimeStub::SetStateFlag(
    std::uint32_t, std::uint32_t) {}
void MonsterManagerCreateRuntimeStub::AssignName(
    ModeStringBufferRecord&, const ModeStringBufferRecord&,
    std::uint32_t, std::uint32_t) {}
void MonsterManagerCreateRuntimeStub::InsertNode(
    std::uint32_t, const ModeStringBufferRecord&, std::uint32_t) {}
void MonsterManagerCreateRuntimeStub::InitializeComponent(
    std::uint32_t, std::uint32_t, std::uint32_t) {}

std::uint32_t MonsterManagerCreator::Create(
    std::uint32_t manager, const MonsterCreationArguments& arguments,
    IMonsterManagerCreateRuntime& runtime) {
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
    const auto found = runtime.SearchNode(tree, name);
    if (found != runtime.ReadDword(manager + 0x2cu)) {
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
        runtime.ReadDword(arguments.source_record + 0x30u));

    ModeStringBufferRecord key{};
    key.mode_word = name.mode_word & 0xffu;
    runtime.SetErrorFlag(key, 0u);
    MonsterManagerTemporaryText key_owner{key, runtime};
    runtime.AssignName(key, name, 0u, 0xffffffffu);
    runtime.InsertNode(tree, key, object);
    key_owner.Release();

    runtime.WriteDword(object + 0x204u, manager + 0x38u);
    runtime.InitializeComponent(object, arguments.source_record, 0u);
    name_owner.Release();
    return object;
}

}  // namespace pal4
