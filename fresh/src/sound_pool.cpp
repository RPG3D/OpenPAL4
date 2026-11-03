#include "pal4/sound_pool.h"

#include <cstring>

namespace pal4 {

std::uint32_t SoundPool::GetInstance(
    SoundPoolSingletonState& state,
    ISoundPoolSingletonRuntime& runtime) {
    if ((state.guard & 1u) != 0u)
        return state.instance;
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    runtime.GetRegistryInstance();
    state.instance = runtime.Register(
        "auSoundPool", 1005u, &SoundPool::Factory,
        &SoundPool::DestroyInstance);
    return state.instance;
}

std::uint32_t SoundPool::Factory(ISoundPoolObjectRuntime& runtime) {
    const auto pool = runtime.Allocate(kObjectSize);
    if (pool == 0u)
        return 0u;
    runtime.WriteByte(pool, 8u, 0u);

    auto& tree = runtime.TreeGlobals();
    const auto candidate = runtime.Allocate(kTreeNodeSize);
    runtime.WriteWord(candidate, 4u, 0u);
    runtime.WriteWord(candidate, 24u, 1u);
    if (tree.sentinel == 0u) {
        tree.sentinel = candidate;
        runtime.WriteWord(candidate, 0u, 0u);
        runtime.WriteWord(candidate, 8u, 0u);
    } else {
        (void)runtime.Free(candidate);
    }
    ++tree.users;

    const auto root = runtime.Allocate(kTreeNodeSize);
    runtime.WriteWord(root, 4u, tree.sentinel);
    runtime.WriteWord(root, 24u, 0u);
    runtime.WriteWord(pool, 4u, root);
    runtime.WriteWord(pool, 12u, 0u);
    runtime.WriteWord(root, 0u, root);
    runtime.WriteWord(root, 8u, root);
    runtime.WriteByte(pool, 16u, 1u);
    return pool;
}

std::uint32_t SoundPool::DestroyInstance(
    const std::uint32_t address, ISoundPoolObjectRuntime& runtime) {
    if (address == 0u)
        return 0u;
    (void)Destroy(address, runtime);
    return runtime.Free(address);
}

std::uint32_t SoundPool::Destroy(
    const std::uint32_t address, ISoundPoolObjectRuntime& runtime) {
    const auto root = runtime.ReadWord(address, 4u);
    const auto count = runtime.ReadWord(address, 12u);
    auto first = runtime.ReadWord(root, 0u);
    auto& tree = runtime.TreeGlobals();
    if (count != 0u) {
        runtime.FreeNodesRecursive(runtime.ReadWord(root, 4u));
        runtime.WriteWord(root, 4u, tree.sentinel);
        runtime.WriteWord(address, 12u, 0u);
        runtime.WriteWord(root, 0u, root);
        runtime.WriteWord(root, 8u, root);
        runtime.FirstElement(address);
    } else {
        while (first != root) {
            const auto current = first;
            first = runtime.NextNode(current);
            runtime.RemoveNode(address, current);
        }
    }
    (void)runtime.Free(root);
    runtime.WriteWord(address, 4u, 0u);
    runtime.WriteWord(address, 12u, 0u);
    --tree.users;
    if (tree.users != 0u)
        return tree.users;
    const auto sentinel = tree.sentinel;
    tree.sentinel = 0u;
    return sentinel == 0u ? 0u : runtime.Free(sentinel);
}

std::uint32_t SoundPool::CleanupTree(
    const std::uint32_t address, ISoundPoolObjectRuntime& runtime) {
    return Destroy(address, runtime);
}

std::uint32_t SoundPool::ClearElements(
    const std::uint32_t address, ISoundPoolObjectRuntime& runtime) {
    const auto root = runtime.ReadWord(address, 4u);
    const auto count = runtime.ReadWord(address, 12u);
    auto first = runtime.ReadWord(root, 0u);
    if (count != 0u) {
        auto node = runtime.ReadWord(root, 4u);
        const auto sentinel = runtime.TreeGlobals().sentinel;
        while (node != sentinel) {
            runtime.FreeNodesRecursive(runtime.ReadWord(node, 8u));
            const auto next = runtime.ReadWord(node, 0u);
            runtime.CleanupNodeValue(node + 12u);
            (void)runtime.Free(node);
            node = next;
        }
        runtime.WriteWord(root, 4u, sentinel);
        runtime.WriteWord(address, 12u, 0u);
        runtime.WriteWord(root, 0u, root);
        runtime.WriteWord(root, 8u, root);
        return root;
    }
    while (first != root) {
        const auto current = first;
        first = runtime.NextNode(current);
        runtime.RemoveNode(address, current);
    }
    return first;
}

std::uint32_t SoundPool::AddAudioFile(
    const std::uint32_t pool, const char* path,
    ISoundPoolAddRuntime& runtime) {
    const auto length = static_cast<std::uint32_t>(std::strlen(path));
    const auto crc = runtime.ComputeCrc(path, length);
    const auto node = runtime.FindNode(pool, crc);
    if (node != runtime.Root(pool)) {
        const auto file_address = runtime.NodeFile(node);
        auto& file = runtime.FileAt(file_address);
        if (static_cast<std::int32_t>(file.reference_count) <= 0)
            file.reference_count = 0u;
        ++file.reference_count;
        return file_address;
    }

    const auto file_address = runtime.AllocateFile(28u);
    auto& file = runtime.FileAt(file_address);
    if (file_address != 0u)
        (void)AudioFile::Initialize(file, runtime);
    if (AudioFile::Load(file, path, runtime) == 0u) {
        runtime.Log(100u);
        if (file_address != 0u) {
            (void)AudioFile::Cleanup(file, runtime);
            (void)runtime.FreeFile(file_address);
        }
        return 0u;
    }
    if (runtime.ControlEnabled(pool)) {
        runtime.InsertSoundObject(pool, crc, file_address);
        runtime.DestroyTemporary(file_address, 1u);
        runtime.IncrementReference(file_address);
    }
    return file_address;
}

std::uint8_t SoundPoolEntry::Cleanup(
    const SoundPoolEntryState& entry,
    ISoundPoolEntryRuntime& runtime) {
    if (entry.owns_file == 0u || entry.file == 0u)
        return entry.owns_file;
    (void)AudioFile::Cleanup(runtime.FileAt(entry.file), runtime);
    return static_cast<std::uint8_t>(runtime.FreeFile(entry.file));
}

std::uint32_t SoundPool::ReleaseAudioFile(
    const std::uint32_t pool, const std::uint32_t crc,
    ISoundPoolReleaseRuntime& runtime) {
    const auto node = runtime.FindNode(pool, crc);
    const auto root = runtime.Root(pool);
    if (node == root || crc < runtime.NodeCrc(node))
        return root;
    const auto file_address = runtime.NodeFile(node);
    auto& file = runtime.FileAt(file_address);
    const auto references = static_cast<std::int32_t>(--file.reference_count);
    if (references <= 0) {
        if (runtime.Count(pool) > 32u)
            return CleanupExpired(pool, runtime);
        --file.reference_count;
    }
    return file_address;
}

std::uint32_t SoundPool::CleanupExpired(
    const std::uint32_t pool, ISoundPoolReleaseRuntime& runtime) {
    const auto root = runtime.Root(pool);
    auto node = runtime.FirstNode(pool);
    auto selected = root;
    while (node != runtime.Root(pool)) {
        const auto file_address = runtime.NodeFile(node);
        auto& file = runtime.FileAt(file_address);
        if (static_cast<std::int32_t>(file.reference_count) <= 0) {
            const auto new_count = static_cast<std::int32_t>(
                --file.reference_count);
            if (selected == runtime.Root(pool) ||
                static_cast<std::int32_t>(
                    runtime.FileAt(runtime.NodeFile(selected))
                        .reference_count) > new_count)
                selected = node;
        }
        node = runtime.NextNode(node);
    }
    if (selected != runtime.Root(pool))
        return runtime.RemoveNode(pool, selected);
    return node;
}

std::uint8_t SoundPool::SetControlFlag(
    const std::uint32_t pool, const std::uint8_t enabled,
    ISoundPoolObjectRuntime& runtime) {
    runtime.WriteByte(pool, 16u, enabled);
    if (enabled == 0u)
        return static_cast<std::uint8_t>(ClearElements(pool, runtime));
    return enabled;
}

}  // namespace pal4
