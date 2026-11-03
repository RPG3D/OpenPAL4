#include "pal4/npc_manager_access.h"

namespace pal4 {

std::uint32_t NpcManagerSingletonRuntimeStub::SingletonManager() {
    return 0u;
}
std::uint32_t NpcManagerSingletonRuntimeStub::Register(
    std::uint32_t, std::string_view, std::uint32_t,
    std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t NpcManagerAccess::Get(
    NpcManagerAccessState& state,
    INpcManagerSingletonRuntime& runtime) {
    if ((state.guard & 1u) != 0u) return state.instance;
    state.guard |= 1u;
    const auto manager = runtime.SingletonManager();
    state.instance = runtime.Register(manager, "palNpcManager",
        kSingletonId, kFactoryAddress, kTypeInfoAddress);
    return state.instance;
}

std::uint32_t NpcManagerObjectRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
std::uint32_t NpcManagerObjectRuntimeStub::Free(std::uint32_t) {
    return 0u;
}
void NpcManagerObjectRuntimeStub::InitializeTreeBase(std::uint32_t) {}
void NpcManagerObjectRuntimeStub::InitializeGrid(std::uint32_t) {}
void NpcManagerObjectRuntimeStub::CleanupGrid(std::uint32_t) {}
std::uint32_t NpcManagerObjectRuntimeStub::CleanupTreeBase(
    std::uint32_t) { return 0u; }
void NpcManagerObjectRuntimeStub::CleanupTreeNodes(
    std::uint32_t, std::uint32_t) {}
void NpcManagerObjectRuntimeStub::ResetTreeBegin(std::uint32_t) {}
std::uint32_t NpcManagerObjectRuntimeStub::NextTreeNode(
    std::uint32_t) { return 0u; }
void NpcManagerObjectRuntimeStub::DeleteTreeNode(
    std::uint32_t, std::uint32_t) {}
std::uint32_t NpcManagerObjectRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void NpcManagerObjectRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
void NpcManagerObjectRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}

std::uint32_t NpcManagerObject::Create(
    NpcManagerTreeGlobals& globals,
    std::uint8_t incoming_stack_byte,
    INpcManagerObjectRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectBytes);
    if (object == 0u) return 0u;
    return Construct(object, globals, incoming_stack_byte, runtime);
}

std::uint32_t NpcManagerObject::Construct(
    std::uint32_t object, NpcManagerTreeGlobals& globals,
    std::uint8_t incoming_stack_byte,
    INpcManagerObjectRuntime& runtime) {
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

std::uint32_t NpcManagerObject::DestroyTree(
    std::uint32_t tree, NpcManagerTreeGlobals& globals,
    INpcManagerObjectRuntime& runtime) {
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

std::uint32_t NpcManagerObject::Destroy(
    std::uint32_t object, NpcManagerTreeGlobals& globals,
    INpcManagerObjectRuntime& runtime) {
    runtime.WriteDword(object, kVtableAddress);
    runtime.CleanupGrid(object + 0x38u);
    static_cast<void>(DestroyTree(object + 0x28u, globals, runtime));
    return runtime.CleanupTreeBase(object);
}

std::uint32_t NpcManagerActionRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void NpcManagerActionRuntimeStub::CreateNpc(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t) {}

std::uint32_t NpcManagerActionCallback::Process(
    std::uint32_t record, NpcManagerAccessState& access,
    INpcManagerSingletonRuntime& singleton_runtime,
    INpcManagerActionRuntime& action_runtime) {
    const auto resource = action_runtime.ReadDword(record + 0x0cu);
    const auto name = action_runtime.ReadDword(record);
    const auto manager = NpcManagerAccess::Get(access, singleton_runtime);
    action_runtime.CreateNpc(manager, name, resource,
                             record + 0x24u, record + 0x30u,
                             record, 1u);
    return record;
}

void NpcManagerInitializeRuntimeStub::Cleanup(std::uint32_t) {}
void NpcManagerInitializeRuntimeStub::LoadInfo(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void NpcManagerInitializeRuntimeStub::InitializeGrid(std::uint32_t) {}
void NpcManagerInitializeRuntimeStub::ForEachNpc(
    std::uint32_t, std::uint32_t, std::uint32_t) {}

std::uint32_t NpcManagerInitializer::Initialize(
    std::uint32_t manager, std::uint32_t first,
    std::uint32_t second, INpcManagerInitializeRuntime& runtime) {
    runtime.Cleanup(manager);
    runtime.LoadInfo(manager, first, second);
    runtime.InitializeGrid(manager + 0x38u);
    runtime.ForEachNpc(manager, kActionCallbackAddress, 0u);
    return 1u;
}

std::uint32_t NpcManagerCleanupRuntimeStub::Allocate(
    std::uint32_t) { return 0u; }
std::uint32_t NpcManagerCleanupRuntimeStub::Free(
    std::uint32_t) { return 0u; }
void NpcManagerCleanupRuntimeStub::InitializeTreeBase(std::uint32_t) {}
void NpcManagerCleanupRuntimeStub::InitializeGrid(std::uint32_t) {}
void NpcManagerCleanupRuntimeStub::CleanupGrid(std::uint32_t) {}
std::uint32_t NpcManagerCleanupRuntimeStub::CleanupTreeBase(
    std::uint32_t) { return 0u; }
void NpcManagerCleanupRuntimeStub::CleanupTreeNodes(
    std::uint32_t, std::uint32_t) {}
void NpcManagerCleanupRuntimeStub::ResetTreeBegin(std::uint32_t) {}
std::uint32_t NpcManagerCleanupRuntimeStub::NextTreeNode(
    std::uint32_t) { return 0u; }
void NpcManagerCleanupRuntimeStub::DeleteTreeNode(
    std::uint32_t, std::uint32_t) {}
std::uint32_t NpcManagerCleanupRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void NpcManagerCleanupRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
void NpcManagerCleanupRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
void NpcManagerCleanupRuntimeStub::NotifyNpc(std::uint32_t) {}
void NpcManagerCleanupRuntimeStub::DestroyNpc(
    std::uint32_t, std::uint32_t) {}
void NpcManagerCleanupRuntimeStub::FreeTreeText(
    std::uint32_t, std::uint32_t) {}
void NpcManagerCleanupRuntimeStub::CleanupComplexTree(std::uint32_t) {}
void NpcManagerCleanupRuntimeStub::ClearGrid(std::uint32_t) {}
void NpcManagerCleanupRuntimeStub::GetPreviousGridElement(std::uint32_t) {}
std::uint32_t NpcManagerCleanupRuntimeStub::ResetGrid(
    std::uint32_t) { return 0u; }

std::uint32_t NpcManagerCleanup::Clear(
    std::uint32_t manager, const NpcManagerTreeGlobals& globals,
    INpcManagerCleanupRuntime& runtime) {
    const auto tree = manager + 0x28u;
    const auto header = runtime.ReadDword(tree + 4u);
    auto cursor = runtime.ReadDword(header);
    while (cursor != header) {
        const auto npc = runtime.ReadDword(cursor + 0x1cu);
        runtime.NotifyNpc(npc);
        runtime.DestroyNpc(npc, 1u);
        cursor = runtime.NextTreeNode(cursor);
    }

    if (runtime.ReadDword(tree + 0x0cu) != 0u) {
        auto node = runtime.ReadDword(header + 4u);
        while (node != globals.nil_address) {
            runtime.CleanupTreeNodes(tree,
                runtime.ReadDword(node + 8u));
            const auto left = runtime.ReadDword(node);
            runtime.FreeTreeText(node + 0x0cu, 0u);
            static_cast<void>(runtime.Free(node));
            node = left;
        }
        runtime.WriteDword(header + 4u, globals.nil_address);
        runtime.WriteDword(tree + 0x0cu, 0u);
        runtime.WriteDword(header, header);
        runtime.WriteDword(header + 8u, header);
    } else {
        cursor = runtime.ReadDword(header);
        while (cursor != header) {
            const auto node = cursor;
            cursor = runtime.NextTreeNode(cursor);
            runtime.DeleteTreeNode(tree, node);
        }
    }

    runtime.CleanupComplexTree(manager);
    const auto grid = manager + 0x38u;
    runtime.ClearGrid(grid);
    runtime.GetPreviousGridElement(grid);
    return runtime.ResetGrid(grid);
}

}  // namespace pal4
