#include "pal4/monster_manager_access.h"

namespace pal4 {

std::uint32_t MonsterManagerSingletonRuntimeStub::SingletonManager() {
    return 0u;
}

std::uint32_t MonsterManagerSingletonRuntimeStub::Register(
    std::uint32_t, std::string_view, std::uint32_t,
    std::uint32_t, std::uint32_t) {
    return 0u;
}

std::uint32_t MonsterManagerAccess::Get(
    MonsterManagerAccessState& state,
    IMonsterManagerSingletonRuntime& runtime) {
    if ((state.guard & 1u) != 0u) return state.instance;
    state.guard |= 1u;
    const auto manager = runtime.SingletonManager();
    state.instance = runtime.Register(
        manager, "palMonsterManager", kSingletonId,
        kFactoryAddress, kTypeInfoAddress);
    return state.instance;
}

std::uint32_t MonsterManagerObjectRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
std::uint32_t MonsterManagerObjectRuntimeStub::Free(std::uint32_t) {
    return 0u;
}
void MonsterManagerObjectRuntimeStub::InitializeTreeBase(std::uint32_t) {}
void MonsterManagerObjectRuntimeStub::InitializeGrid(std::uint32_t) {}
void MonsterManagerObjectRuntimeStub::CleanupGrid(std::uint32_t) {}
std::uint32_t MonsterManagerObjectRuntimeStub::CleanupTreeBase(std::uint32_t) {
    return 0u;
}
void MonsterManagerObjectRuntimeStub::CleanupTreeNodes(
    std::uint32_t, std::uint32_t) {}
void MonsterManagerObjectRuntimeStub::ResetTreeBegin(std::uint32_t) {}
std::uint32_t MonsterManagerObjectRuntimeStub::NextTreeNode(std::uint32_t) {
    return 0u;
}
void MonsterManagerObjectRuntimeStub::DeleteTreeNode(
    std::uint32_t, std::uint32_t) {}
std::uint32_t MonsterManagerObjectRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void MonsterManagerObjectRuntimeStub::WriteDword(std::uint32_t,
                                                  std::uint32_t) {}
void MonsterManagerObjectRuntimeStub::WriteByte(std::uint32_t,
                                                 std::uint8_t) {}

std::uint32_t MonsterManagerObject::Create(
    MonsterManagerTreeGlobals& globals,
    std::uint8_t incoming_stack_byte,
    IMonsterManagerObjectRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectBytes);
    if (object == 0u) return 0u;
    return Construct(object, globals, incoming_stack_byte, runtime);
}

std::uint32_t MonsterManagerObject::Construct(
    std::uint32_t object, MonsterManagerTreeGlobals& globals,
    std::uint8_t incoming_stack_byte,
    IMonsterManagerObjectRuntime& runtime) {
    runtime.InitializeTreeBase(object);
    runtime.WriteByte(object + 0x28u, incoming_stack_byte);
    runtime.WriteByte(object + 0x29u, incoming_stack_byte);
    runtime.WriteByte(object + 0x30u, 0u);

    const auto candidate_nil = runtime.Allocate(kTreeNodeBytes);
    runtime.WriteDword(candidate_nil + 4u, 0u);
    runtime.WriteDword(candidate_nil + 0x20u, 1u);
    if (globals.nil_address == 0u) {
        globals.nil_address = candidate_nil;
        runtime.WriteDword(candidate_nil, 0u);
        runtime.WriteDword(candidate_nil + 8u, 0u);
    } else {
        static_cast<void>(runtime.Free(candidate_nil));
    }
    ++globals.instance_count;

    const auto header = runtime.Allocate(kTreeNodeBytes);
    runtime.WriteDword(header + 4u, globals.nil_address);
    runtime.WriteDword(header + 0x20u, 0u);
    runtime.WriteDword(object + 0x2cu, header);
    runtime.WriteDword(object + 0x34u, 0u);
    runtime.WriteDword(header, header);
    runtime.WriteDword(header + 8u, header);
    runtime.InitializeGrid(object + 0x38u);
    runtime.WriteDword(object, kVtableAddress);
    return object;
}

std::uint32_t MonsterManagerObject::DestroyTree(
    std::uint32_t tree, MonsterManagerTreeGlobals& globals,
    IMonsterManagerObjectRuntime& runtime) {
    const auto header = runtime.ReadDword(tree + 4u);
    auto cursor = runtime.ReadDword(header);
    if (runtime.ReadDword(tree + 0x0cu) != 0u) {
        runtime.CleanupTreeNodes(tree, runtime.ReadDword(header + 4u));
        runtime.WriteDword(header + 4u, globals.nil_address);
        runtime.WriteDword(tree + 0x0cu, 0u);
        runtime.WriteDword(header, header);
        runtime.WriteDword(header + 8u, header);
        runtime.ResetTreeBegin(tree);
    } else {
        while (cursor != header) {
            const auto node = cursor;
            cursor = runtime.NextTreeNode(cursor);
            runtime.DeleteTreeNode(tree, node);
        }
    }

    static_cast<void>(runtime.Free(header));
    runtime.WriteDword(tree + 4u, 0u);
    runtime.WriteDword(tree + 0x0cu, 0u);
    --globals.instance_count;
    if (globals.instance_count != 0u) return globals.instance_count;
    const auto nil = globals.nil_address;
    globals.nil_address = 0u;
    return nil != 0u ? runtime.Free(nil) : 0u;
}

std::uint32_t MonsterManagerObject::Destroy(
    std::uint32_t object, MonsterManagerTreeGlobals& globals,
    IMonsterManagerObjectRuntime& runtime) {
    runtime.WriteDword(object, kVtableAddress);
    runtime.CleanupGrid(object + 0x38u);
    static_cast<void>(DestroyTree(object + 0x28u, globals, runtime));
    return runtime.CleanupTreeBase(object);
}

std::uint32_t MonsterManagerActionRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void MonsterManagerActionRuntimeStub::CreateMonster(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t) {}

std::uint32_t MonsterManagerActionCallback::Process(
    std::uint32_t record, MonsterManagerAccessState& access,
    IMonsterManagerSingletonRuntime& singleton_runtime,
    IMonsterManagerActionRuntime& action_runtime) {
    const auto secondary_name = action_runtime.ReadDword(record + 0x0cu);
    const auto name = action_runtime.ReadDword(record);
    const auto manager = MonsterManagerAccess::Get(access, singleton_runtime);
    action_runtime.CreateMonster(manager, name, secondary_name,
                                 record + 0x18u, record + 0x24u,
                                 record, 1u);
    return record;
}

void MonsterManagerInitializeRuntimeStub::Cleanup(std::uint32_t) {}
void MonsterManagerInitializeRuntimeStub::LoadInfo(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void MonsterManagerInitializeRuntimeStub::InitializeGrid(std::uint32_t) {}
void MonsterManagerInitializeRuntimeStub::ForEachMonster(
    std::uint32_t, std::uint32_t, std::uint32_t) {}

std::uint32_t MonsterManagerInitializer::Initialize(
    std::uint32_t manager, std::uint32_t first,
    std::uint32_t second,
    IMonsterManagerInitializeRuntime& runtime) {
    runtime.Cleanup(manager);
    runtime.LoadInfo(manager, first, second);
    runtime.InitializeGrid(manager + 0x38u);
    runtime.ForEachMonster(manager, kActionCallbackAddress, 0u);
    return 1u;
}

}  // namespace pal4
