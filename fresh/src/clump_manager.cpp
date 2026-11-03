#include "pal4/clump_manager.h"

#include <algorithm>

namespace pal4 {

std::uint32_t ClumpManager::Register(IClumpManagerRuntime& runtime) {
    const auto guard = runtime.ReadByte(0x92f570u);
    if ((guard & 1u) != 0u) return runtime.ReadWord(0x92f56cu);
    runtime.WriteByte(0x92f570u, static_cast<std::uint8_t>(guard | 1u));
    const auto manager = runtime.SingletonManager();
    const auto object = runtime.RegisterSingleton(
        manager, "RpHookClumpMgrArena", 1010u,
        ClumpManagerCallback::create, ClumpManagerCallback::destroy);
    runtime.WriteWord(0x92f56cu, object);
    return object;
}

std::uint32_t ClumpManager::Create(IClumpManagerRuntime& runtime) {
    const auto object = runtime.Allocate(48u);
    if (object == 0u) return 0u;
    try {
        return Initialize(object, runtime);
    } catch (...) {
        (void)runtime.Free(object);
        throw;
    }
}

std::uint32_t ClumpManager::Destroy(
    const std::uint32_t object, IClumpManagerRuntime& runtime) {
    if (object == 0u) return 0u;
    (void)Cleanup(object, runtime);
    return runtime.Free(object);
}

int ClumpManager::Compare(const std::uint32_t lhs,
                          const std::uint32_t rhs) {
    return lhs < rhs ? 1 : (lhs == rhs ? 0 : -1);
}

std::uint32_t ClumpManager::FreeRecord(
    const std::uint32_t record, IClumpManagerRuntime& runtime) {
    runtime.WriteWord(record, 0u);
    return runtime.Free(record);
}

std::uint32_t ClumpManager::SetCurrentClump(
    const std::uint32_t slot, const std::uint32_t clump,
    IClumpManagerRuntime& runtime) {
    runtime.WriteWord(slot, clump);
    return clump;
}

std::uint32_t ClumpManager::Initialize(
    const std::uint32_t object, IClumpManagerRuntime& runtime) {
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object + 12u, 0u);
    const auto container = object + 16u;
    runtime.InitializeContainer(container);
    try {
        const auto begin = runtime.ReadWord(object + 4u);
        const auto end = runtime.ReadWord(object + 8u);
        for (auto cursor = begin; cursor != end; cursor += 12u)
            runtime.CleanupString(cursor);
        runtime.WriteWord(object + 8u, begin);
        runtime.SetContainerName(container, "hook-clump-mgr-container");
        runtime.SetContainerFlag(container, 1u);
        runtime.SetContainerCallback(container,
                                     ClumpManagerCallback::free_memory);
        runtime.SetContainerCallback(container, ClumpManagerCallback::compare);
        runtime.SetContainerCallback(container,
                                     ClumpManagerCallback::set_current_clump);
        runtime.SetContainerCallback(container, ClumpManagerCallback::no_op);
    } catch (...) {
        (void)Cleanup(object, runtime);
        throw;
    }
    return object;
}

std::uint32_t ClumpManager::Cleanup(
    const std::uint32_t object, IClumpManagerRuntime& runtime) {
    const auto begin = runtime.ReadWord(object + 4u);
    const auto end = runtime.ReadWord(object + 8u);
    for (auto cursor = begin; cursor != end; cursor += 12u)
        runtime.CleanupString(cursor);
    runtime.WriteWord(object + 8u, begin);
    runtime.DestroyContainer(object + 16u);
    const auto remaining_begin = runtime.ReadWord(object + 4u);
    const auto remaining_end = runtime.ReadWord(object + 8u);
    for (auto cursor = remaining_begin; cursor != remaining_end; cursor += 12u)
        runtime.CleanupString(cursor);
    const auto result = runtime.Free(remaining_begin);
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object + 12u, 0u);
    return result;
}

bool ClumpManager::LoadClump(
    const std::uint32_t object, const std::string_view filename,
    IClumpManagerRuntime& runtime) {
    const auto collection = runtime.CreateLoadCollection();
    try {
        runtime.ScanClumpFile(collection, filename);
        const int count = runtime.LoadCollectionCount(collection);
        if (count <= 0) {
            runtime.DestroyLoadCollection(collection);
            return false;
        }

        constexpr std::uint32_t record_size = 12u;
        for (int index = 0; index < count; ++index) {
            const auto element = runtime.LoadCollectionElement(collection, index);
            const auto source = runtime.LowercaseString(element + record_size);
            const auto begin = runtime.ReadWord(object + 4u);
            const auto end = runtime.ReadWord(object + 8u);
            const auto capacity_end = runtime.ReadWord(object + 12u);
            if (end != capacity_end) {
                runtime.CopyStringRecord(end, source);
                runtime.WriteWord(object + 8u, end + record_size);
                continue;
            }

            const auto size = begin == 0u ? 0u : (end - begin) / record_size;
            const auto new_capacity = size + std::max(size, 1u);
            const auto new_begin = runtime.Allocate(new_capacity * record_size);
            std::uint32_t constructed = 0u;
            try {
                for (; constructed < size; ++constructed)
                    runtime.CopyStringRecord(
                        new_begin + constructed * record_size,
                        begin + constructed * record_size);
                runtime.CopyStringRecord(new_begin + size * record_size,
                                         source);
                ++constructed;
            } catch (...) {
                for (std::uint32_t i = 0; i < constructed; ++i)
                    runtime.CleanupString(new_begin + i * record_size);
                (void)runtime.Free(new_begin);
                throw;
            }
            for (std::uint32_t i = 0; i < size; ++i)
                runtime.CleanupString(begin + i * record_size);
            (void)runtime.Free(begin);
            runtime.WriteWord(object + 12u,
                              new_begin + new_capacity * record_size);
            runtime.WriteWord(object + 4u, new_begin);
            runtime.WriteWord(object + 8u,
                              new_begin + (size + 1u) * record_size);
        }
        runtime.ClearLoadCollection(collection);
        runtime.DestroyLoadCollection(collection);
        return true;
    } catch (...) {
        runtime.DestroyLoadCollection(collection);
        throw;
    }
}

std::uint32_t ClumpManager::FindClump(
    const std::uint32_t object, const std::string_view name,
    IClumpManagerRuntime& runtime) {
    const auto query = runtime.CreateLookupString(name);
    try {
        (void)runtime.LowercaseString(query);
        const auto end = runtime.ReadWord(object + 8u);
        for (auto record = runtime.ReadWord(object + 4u);
             record != end; record += 12u) {
            const auto candidate = runtime.ReadWord(record);
            if (runtime.FindStringSubstring(query, candidate, 0u) == 0) {
                runtime.CleanupLookupString(query);
                return candidate;
            }
        }
        runtime.CleanupLookupString(query);
        return 0u;
    } catch (...) {
        runtime.CleanupLookupString(query);
        throw;
    }
}

std::uint32_t ClumpManager::CleanupContainer(
    const std::uint32_t object, IClumpManagerRuntime& runtime) {
    return runtime.CleanupEmbeddedContainer(object + 16u);
}

}  // namespace pal4
