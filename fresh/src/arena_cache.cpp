#include "pal4/arena_cache.h"

namespace pal4 {

std::uint32_t ArenaCache::Register(
    std::uint8_t& guard, std::uint32_t& registered,
    IArenaCacheEntryRuntime& runtime) {
    if ((guard & 1u) != 0u) return registered;
    guard |= 1u;
    registered = runtime.Call(ArenaCacheEntryCall::RegisterArenaCacheSingleton,
                              {kSingletonId, 0x51e540u, 0x51e5a0u});
    return registered;
}

std::uint32_t ArenaCache::Create(IArenaCacheEntryRuntime& runtime) {
    const auto cache = runtime.Allocate(kObjectSize);
    return cache == 0u ? 0u : Initialize(cache, runtime);
}

std::uint32_t ArenaCache::Destroy(
    const std::uint32_t cache, IArenaCacheEntryRuntime& runtime) {
    if (cache == 0u) return 0u;
    (void)ReleaseEntries(cache, runtime);
    return runtime.Free(cache);
}

std::uint32_t ArenaCache::Initialize(
    const std::uint32_t cache, IArenaCacheEntryRuntime& runtime) {
    runtime.WriteByte(cache + 4u, static_cast<std::uint8_t>(cache >> 24u));
    runtime.WriteWord(cache + 8u, 0u);
    runtime.WriteWord(cache + 12u, 0u);
    runtime.WriteWord(cache + 16u, 0u);
    runtime.WriteWord(cache, 1u);
    return cache;
}

std::uint32_t ArenaCache::ReleaseEntries(
    const std::uint32_t cache, IArenaCacheEntryRuntime& runtime) {
    runtime.WriteWord(cache, 1u);
    for (auto entry = runtime.ReadWord(cache + 8u);
         entry != runtime.ReadWord(cache + 12u); entry += kEntrySize)
        (void)ArenaCacheEntry::Destroy(entry, 0u, runtime);
    const auto result = runtime.Free(runtime.ReadWord(cache + 8u));
    runtime.WriteWord(cache + 8u, 0u);
    runtime.WriteWord(cache + 12u, 0u);
    runtime.WriteWord(cache + 16u, 0u);
    return result;
}

std::uint32_t ArenaCache::ClearEntries(
    const std::uint32_t cache, IArenaCacheEntryRuntime& runtime) {
    const auto begin = runtime.ReadWord(cache + 8u);
    const auto end = runtime.ReadWord(cache + 12u);
    for (auto entry = begin; entry != end; entry += kEntrySize)
        (void)ArenaCacheEntry::ClearChildren(entry, runtime);
    std::uint32_t result = end;
    for (auto entry = begin; entry != end; entry += kEntrySize)
        result = ArenaCacheEntry::Destroy(entry, 0u, runtime);
    runtime.WriteWord(cache + 12u, begin);
    return result;
}

std::uint32_t ArenaCache::CaptureOrInsert(
    const std::uint32_t cache, const std::uint32_t first_name,
    const std::uint32_t second_name, const std::uint32_t temporary,
    IArenaCacheEntryRuntime& runtime) {
    if (runtime.ReadWord(cache) == 0u) return 0u;
    for (auto entry = runtime.ReadWord(cache + 8u);
         entry != runtime.ReadWord(cache + 12u); entry += kEntrySize) {
        if (ArenaCacheEntry::MatchesNames(entry, first_name, second_name,
                                          runtime) == 0u) continue;
        (void)ArenaCacheEntry::Capture(entry, first_name, second_name, runtime);
        return 1u;
    }

    (void)ArenaCacheEntry::Initialize(temporary, runtime);
    (void)ArenaCacheEntry::Capture(temporary, first_name, second_name, runtime);
    (void)InsertEntries(cache, runtime.ReadWord(cache + 12u), 1u,
                        temporary, runtime);
    (void)runtime.Call(ArenaCacheEntryCall::LogCacheEvent,
                       {63u, first_name, second_name});
    (void)ArenaCacheEntry::Cleanup(temporary, runtime);
    return 1u;
}

std::uint32_t ArenaCache::Restore(
    const std::uint32_t cache, const std::uint32_t first_name,
    const std::uint32_t second_name, IArenaCacheEntryRuntime& runtime) {
    if (runtime.ReadWord(cache) == 0u) return 0u;
    for (auto entry = runtime.ReadWord(cache + 8u);
         entry != runtime.ReadWord(cache + 12u); entry += kEntrySize) {
        if (ArenaCacheEntry::MatchesNames(entry, first_name, second_name,
                                          runtime) == 0u) continue;
        ArenaCacheEntry::Restore(entry, runtime);
        (void)runtime.Call(ArenaCacheEntryCall::LogCacheEvent,
                           {102u, first_name, second_name});
        return 1u;
    }
    (void)runtime.Call(ArenaCacheEntryCall::LogCacheEvent,
                       {95u, first_name, second_name});
    return 0u;
}

std::uint32_t ArenaCache::Save(
    const std::uint32_t cache, const std::uint32_t first_name,
    const std::uint32_t second_name, const std::uint32_t data,
    const std::uint32_t temporary, IArenaCacheEntryRuntime& runtime) {
    if (runtime.ReadWord(cache) == 0u) return 0u;
    (void)CaptureOrInsert(cache, first_name, second_name, temporary, runtime);
    const auto begin = runtime.ReadWord(cache + 8u);
    const auto end = runtime.ReadWord(cache + 12u);
    const auto count = begin == 0u ? 0u : (end - begin) / kEntrySize;
    (void)runtime.Call(ArenaCacheEntryCall::WriteDataWord, {data, count, 4u});
    std::uint32_t result = end;
    for (auto entry = begin; entry != end; entry += kEntrySize)
        result = ArenaCacheEntry::ProcessChildren(entry, data, runtime);
    return result;
}

std::uint32_t ArenaCache::Load(
    const std::uint32_t cache, const std::uint32_t data,
    const std::uint32_t temporary, IArenaCacheEntryRuntime& runtime) {
    if (runtime.ReadWord(cache) == 0u) return 0u;
    (void)ClearEntries(cache, runtime);
    const auto count = runtime.Call(ArenaCacheEntryCall::ReadDataWord,
                                    {data, 4u});
    for (std::uint32_t i = 0u; i < count; ++i) {
        (void)ArenaCacheEntry::Initialize(temporary, runtime);
        (void)InsertEntries(cache, runtime.ReadWord(cache + 12u), 1u,
                            temporary, runtime);
        (void)ArenaCacheEntry::Cleanup(temporary, runtime);
    }
    for (auto entry = runtime.ReadWord(cache + 8u);
         entry != runtime.ReadWord(cache + 12u); entry += kEntrySize)
        (void)ArenaCacheEntry::Load(entry, data, runtime);
    return runtime.ReadWord(cache + 12u);
}

std::uint32_t ArenaCache::InsertEntries(
    const std::uint32_t cache, const std::uint32_t position,
    const std::uint32_t count, const std::uint32_t source,
    IArenaCacheEntryRuntime& runtime) {
    const auto begin = runtime.ReadWord(cache + 8u);
    const auto end = runtime.ReadWord(cache + 12u);
    const auto capacity_end = runtime.ReadWord(cache + 16u);
    if (count == 0u) return end;
    const auto size = begin == 0u ? 0u : (end - begin) / kEntrySize;
    const auto index = begin == 0u ? 0u : (position - begin) / kEntrySize;
    const auto capacity = begin == 0u ? 0u :
                          (capacity_end - begin) / kEntrySize;
    if (capacity - size >= count) {
        for (auto i = size; i > index; --i) {
            const auto old_entry = begin + (i - 1u) * kEntrySize;
            ArenaCacheEntry::CopyConstruct(
                old_entry + count * kEntrySize, old_entry, runtime);
            (void)ArenaCacheEntry::Destroy(old_entry, 0u, runtime);
        }
        for (std::uint32_t i = 0u; i < count; ++i)
            ArenaCacheEntry::CopyConstruct(
                begin + (index + i) * kEntrySize, source, runtime);
        const auto new_end = end + count * kEntrySize;
        runtime.WriteWord(cache + 12u, new_end);
        return new_end;
    }

    const auto growth = size > count ? size : count;
    const auto new_capacity = size + growth;
    const auto replacement = runtime.Allocate(new_capacity * kEntrySize);
    if (replacement == 0u) return end;
    for (std::uint32_t i = 0u; i < index; ++i)
        ArenaCacheEntry::CopyConstruct(replacement + i * kEntrySize,
                                        begin + i * kEntrySize, runtime);
    for (std::uint32_t i = 0u; i < count; ++i)
        ArenaCacheEntry::CopyConstruct(
            replacement + (index + i) * kEntrySize, source, runtime);
    for (auto i = index; i < size; ++i)
        ArenaCacheEntry::CopyConstruct(
            replacement + (i + count) * kEntrySize,
            begin + i * kEntrySize, runtime);
    for (std::uint32_t i = 0u; i < size; ++i)
        (void)ArenaCacheEntry::Destroy(begin + i * kEntrySize, 0u, runtime);
    (void)runtime.Free(begin);
    const auto new_end = replacement + (size + count) * kEntrySize;
    runtime.WriteWord(cache + 8u, replacement);
    runtime.WriteWord(cache + 12u, new_end);
    runtime.WriteWord(cache + 16u,
                      replacement + new_capacity * kEntrySize);
    return new_end;
}

}  // namespace pal4
