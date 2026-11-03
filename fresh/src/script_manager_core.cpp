#include "pal4/script_manager_core.h"

namespace pal4 {
namespace {

std::uint32_t FindExact(
    const std::uint32_t tree, const std::uint32_t key,
    IScriptManagerCoreRuntime& runtime) {
    const auto header = runtime.ReadWord(tree + 4u);
    const auto node = PalScriptTree::LowerBoundPk(tree, key, runtime);
    return node == header || key < runtime.ReadWord(node + 12u)
               ? header : node;
}

}  // namespace

std::uint32_t ScriptManagerCore::Construct(
    const std::uint32_t object, ScriptManagerTreeGlobals& globals,
    IScriptManagerCoreRuntime& runtime) {
    runtime.InitializePackage(object + kPackageOffset);
    runtime.WriteByte(object + kTreeOffset + 8u, 0u);

    const auto candidate = runtime.AllocateMemory(24u);
    if (candidate == 0) {
        (void)runtime.ReleasePackageBlocks(object + kPackageOffset);
        return 0;
    }
    runtime.WriteWord(candidate + 4u, 0u);
    runtime.WriteWord(candidate + 20u, 1u);
    if (globals.shared_nil == 0) {
        globals.shared_nil = candidate;
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    } else {
        runtime.FreeMemory(candidate);
    }
    ++globals.instance_count;

    const auto header = runtime.AllocateMemory(24u);
    if (header == 0) {
        --globals.instance_count;
        if (globals.instance_count == 0) {
            (void)runtime.ReleaseMemory(globals.shared_nil);
            globals.shared_nil = 0;
        }
        (void)runtime.ReleasePackageBlocks(object + kPackageOffset);
        return 0;
    }
    runtime.WriteWord(header + 4u, globals.shared_nil);
    runtime.WriteWord(header + 20u, 0u);
    runtime.WriteWord(object + kTreeHeaderOffset, header);
    runtime.WriteWord(object + kTreeCountOffset, 0u);
    runtime.WriteWord(header, header);
    runtime.WriteWord(header + 8u, header);
    runtime.WriteWord(object, 0u);
    return object;
}

std::uint32_t ScriptManagerCore::Destruct(
    const std::uint32_t object, ScriptManagerTreeGlobals& globals,
    IScriptManagerCoreRuntime& runtime) {
    (void)PalScriptTree::ClearAll(
        object + kTreeOffset, globals.instance_count,
        globals.shared_nil, runtime);
    return runtime.ReleasePackageBlocks(object + kPackageOffset);
}

std::uint8_t ScriptManagerCore::Load(
    const std::uint32_t object, const char* path,
    const std::uint32_t options, IScriptManagerCoreRuntime& runtime) {
    if (path == nullptr) return 0;
    const std::string requested(path);
    if (runtime.ReadByte(object + kPackageOpenOffset) != 0) {
        if (runtime.ReadCString(object + kPackagePathOffset) == requested)
            return 1;
        (void)CleanupPackage(object, runtime);
    }
    const auto result = runtime.OpenPackage(
        object + kPackageOffset, requested);
    runtime.WriteWord(object, options);
    return result;
}

std::uint8_t ScriptManagerCore::CleanupPackage(
    const std::uint32_t object, IScriptManagerCoreRuntime& runtime) {
    const auto tree = object + kTreeOffset;
    const auto header = runtime.ReadWord(tree + 4u);
    const auto first = runtime.ReadWord(header);
    const auto cursor = runtime.TemporaryCursor();
    (void)PalScriptTree::ClearRange(
        tree, cursor, first, header, runtime);
    return runtime.ClosePackage(object + kPackageOffset);
}

std::uint32_t ScriptManagerCore::InsertScript(
    const std::uint32_t object, const std::uint32_t key,
    const std::uint32_t value, IScriptManagerCoreRuntime& runtime) {
    const auto tree = object + kTreeOffset;
    const auto header = runtime.ReadWord(tree + 4u);
    const auto existing = FindExact(tree, key, runtime);
    if (existing != header) return existing;

    auto parent = header;
    auto child = runtime.ReadWord(header + 4u);
    const auto nil = runtime.SharedNil();
    bool left = true;
    while (child != nil) {
        parent = child;
        left = key < runtime.ReadWord(child + 12u);
        child = runtime.ReadWord(child + (left ? 0u : 8u));
    }
    if (runtime.ReadByte(tree + 8u) != 0)
        return runtime.InsertNodeAndRebalance(
            tree, child, parent, key, value);

    auto candidate = parent;
    if (left) {
        if (parent == runtime.ReadWord(header))
            return runtime.InsertNodeAndRebalance(
                tree, child, parent, key, value);
        candidate = runtime.PredecessorNode(parent);
    }
    if (runtime.ReadWord(candidate + 12u) < key)
        return runtime.InsertNodeAndRebalance(
            tree, child, parent, key, value);
    return candidate;
}

std::uint32_t ScriptManagerCore::RemoveScript(
    const std::uint32_t object, const std::uint32_t key,
    IScriptManagerCoreRuntime& runtime) {
    const auto tree = object + kTreeOffset;
    const auto header = runtime.ReadWord(tree + 4u);
    const auto node = FindExact(tree, key, runtime);
    if (node == header) return header;
    return runtime.DeleteNode(tree, node);
}

std::uint32_t ScriptManagerCore::GetScript(
    const std::uint32_t object, const std::uint32_t key,
    IScriptManagerCoreRuntime& runtime) {
    const auto tree = object + kTreeOffset;
    const auto header = runtime.ReadWord(tree + 4u);
    const auto node = FindExact(tree, key, runtime);
    return node == header ? 0u : runtime.ReadWord(node + 16u);
}

std::uint8_t ScriptManagerCore::CloseScript(
    const std::uint32_t object, const std::uint32_t key,
    IScriptManagerCoreRuntime& runtime) {
    const auto handle = GetScript(object, key, runtime);
    return handle == 0 ? 0 : runtime.CloseScriptHandle(
        object + kPackageOffset, handle);
}

std::uint32_t ScriptManagerCore::ReadScript(
    const std::uint32_t object, const std::uint32_t key,
    void* buffer, const std::uint32_t bytes,
    IScriptManagerCoreRuntime& runtime) {
    const auto handle = GetScript(object, key, runtime);
    if (handle == 0) return 0;
    runtime.ReadPackageData(buffer, bytes, handle);
    return bytes;
}

bool ScriptManagerCore::IsScriptLoaded(
    const std::uint32_t object, const std::uint32_t key,
    IScriptManagerCoreRuntime& runtime) {
    const auto handle = GetScript(object, key, runtime);
    return handle != 0 &&
           runtime.ReadWord(handle + 40u) >=
               runtime.ReadWord(handle + 36u) - 1u;
}

std::int32_t ScriptManagerCore::SeekScript(
    const std::uint32_t object, const std::uint32_t key,
    const std::int32_t offset, const std::int32_t origin,
    IScriptManagerCoreRuntime& runtime) {
    const auto handle = GetScript(object, key, runtime);
    return handle == 0 ? 0 : runtime.SeekPackage(
        object + kPackageOffset, handle, offset, origin);
}

}  // namespace pal4
