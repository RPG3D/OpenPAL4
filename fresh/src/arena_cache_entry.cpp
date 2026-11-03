#include "pal4/arena_cache_entry.h"

#include <initializer_list>

namespace pal4 {
namespace {

using Runtime = IArenaCacheEntryRuntime;

std::uint32_t ReleaseCounted(const std::uint32_t pointer, Runtime& runtime) {
    if (pointer == 0u) return 0u;
    const auto allocation = pointer - 1u;
    const auto count = runtime.ReadByte(allocation);
    if (count == 0u || count == 0xffu)
        return runtime.Free(allocation);
    runtime.WriteByte(allocation, static_cast<std::uint8_t>(count - 1u));
    return allocation;
}

void ClearTriple(const std::uint32_t entry, const std::uint32_t offset,
                 Runtime& runtime) {
    runtime.WriteWord(entry + offset, 0u);
    runtime.WriteWord(entry + offset + 4u, 0u);
    runtime.WriteWord(entry + offset + 8u, 0u);
}

std::uint32_t CountChildren(const std::uint32_t entry,
                            const std::uint32_t offset, Runtime& runtime) {
    const auto end = runtime.ReadWord(entry + offset + 4u);
    std::uint32_t count = 0u;
    for (auto cursor = runtime.ReadWord(entry + offset);
         cursor != end; cursor += 4u)
        if (runtime.ReadWord(cursor) != 0u) ++count;
    return count;
}

void WriteChildren(const std::uint32_t entry, const std::uint32_t offset,
                   const std::uint32_t output, Runtime& runtime) {
    (void)runtime.Call(ArenaCacheEntryCall::WriteData,
                       {output, CountChildren(entry, offset, runtime), 4u});
    for (auto cursor = runtime.ReadWord(entry + offset);
         cursor != runtime.ReadWord(entry + offset + 4u); cursor += 4u) {
        const auto child = runtime.ReadWord(cursor);
        if (child == 0u) continue;
        const auto value = runtime.Call(ArenaCacheEntryCall::GetChildValue,
                                        {child});
        (void)runtime.Call(ArenaCacheEntryCall::WriteData,
                           {output, value, 4u});
        (void)runtime.Call(ArenaCacheEntryCall::ProcessChild,
                           {child, output});
    }
}

void AppendPointer(const std::uint32_t entry, const std::uint32_t offset,
                   const std::uint32_t value, Runtime& runtime) {
    const auto begin = runtime.ReadWord(entry + offset);
    const auto end = runtime.ReadWord(entry + offset + 4u);
    const auto capacity_end = runtime.ReadWord(entry + offset + 8u);
    if (end != capacity_end) {
        runtime.WriteWord(end, value);
        runtime.WriteWord(entry + offset + 4u, end + 4u);
        return;
    }

    const auto size = begin == 0u ? 0u : (end - begin) / 4u;
    const auto growth = size > 1u ? size : 1u;
    const auto capacity = size + growth;
    const auto replacement = runtime.Allocate(capacity * 4u);
    if (replacement == 0u) return;
    for (std::uint32_t i = 0u; i < size; ++i)
        runtime.WriteWord(replacement + i * 4u,
                          runtime.ReadWord(begin + i * 4u));
    runtime.WriteWord(replacement + size * 4u, value);
    (void)runtime.Free(begin);
    runtime.WriteWord(entry + offset, replacement);
    runtime.WriteWord(entry + offset + 4u, replacement + (size + 1u) * 4u);
    runtime.WriteWord(entry + offset + 8u, replacement + capacity * 4u);
}

void CopyName(const std::uint32_t record, const std::uint32_t source,
              Runtime& runtime) {
    std::uint32_t length = 0u;
    while (runtime.ReadByte(source + length) != 0u) ++length;
    if (runtime.Call(ArenaCacheEntryCall::AllocateText,
                     {record, length, 1u}) == 0u) return;
    const auto destination = runtime.ReadWord(record + 4u);
    for (std::uint32_t i = 0u; i < length; ++i)
        runtime.WriteByte(destination + i, runtime.ReadByte(source + i));
    runtime.WriteWord(record + 8u, length);
    runtime.WriteByte(destination + length, 0u);
}

void LoadArray(const std::uint32_t entry, const std::uint32_t offset,
               const std::uint32_t data, Runtime& runtime) {
    const auto count = static_cast<std::int32_t>(
        runtime.Call(ArenaCacheEntryCall::ReadDataWord, {data, 4u}));
    for (std::int32_t i = 0; i < count; ++i) {
        const auto id = static_cast<std::int32_t>(
            runtime.Call(ArenaCacheEntryCall::ReadDataWord, {data, 4u}));
        if (id < 0) continue;
        const auto object = runtime.Call(ArenaCacheEntryCall::FindStorageObject,
                                         {static_cast<std::uint32_t>(id)});
        (void)runtime.Call(ArenaCacheEntryCall::DeserializeObject,
                           {object, data});
        AppendPointer(entry, offset, object, runtime);
    }
}

void RestoreArray(const std::uint32_t entry, const std::uint32_t offset,
                  const ArenaCacheEntryCall find,
                  const ArenaCacheEntryCall restore, Runtime& runtime) {
    for (auto cursor = runtime.ReadWord(entry + offset);
         cursor != runtime.ReadWord(entry + offset + 4u); cursor += 4u) {
        const auto child = runtime.ReadWord(cursor);
        if (child == 0u) continue;
        const auto name = runtime.Call(ArenaCacheEntryCall::GetChildName,
                                       {child});
        const auto live = runtime.Call(find, {name});
        if (live != 0u) (void)runtime.Call(restore, {live, child});
    }
}

}  // namespace

std::uint32_t ArenaCacheEntry::Initialize(
    const std::uint32_t entry, Runtime& runtime) {
    const auto high_byte = static_cast<std::uint8_t>(entry >> 24u);
    for (const auto offset : {4u, 20u, 36u, 52u}) {
        runtime.WriteByte(entry + offset, high_byte);
        ClearTriple(entry, offset + 4u, runtime);
    }
    runtime.WriteWord(entry, kVtable);
    runtime.WriteWord(entry + 68u, 0u);
    return entry;
}

std::uint32_t ArenaCacheEntry::Destroy(
    const std::uint32_t entry, const std::uint8_t flags, Runtime& runtime) {
    (void)Cleanup(entry, runtime);
    if ((flags & 1u) != 0u) (void)runtime.Free(entry);
    return entry;
}

std::uint32_t ArenaCacheEntry::Cleanup(
    const std::uint32_t entry, Runtime& runtime) {
    runtime.WriteWord(entry, kVtable);
    (void)runtime.Free(runtime.ReadWord(entry + 56u));
    ClearTriple(entry, 56u, runtime);
    (void)runtime.Free(runtime.ReadWord(entry + 40u));
    ClearTriple(entry, 40u, runtime);
    (void)ReleaseCounted(runtime.ReadWord(entry + 24u), runtime);
    ClearTriple(entry, 24u, runtime);
    const auto result = ReleaseCounted(runtime.ReadWord(entry + 8u), runtime);
    ClearTriple(entry, 8u, runtime);
    return result;
}

std::uint32_t ArenaCacheEntry::ClearChildren(
    const std::uint32_t entry, Runtime& runtime) {
    std::uint32_t last_end = 0u;
    for (const auto offset : {40u, 56u}) {
        for (auto cursor = runtime.ReadWord(entry + offset);
             cursor != runtime.ReadWord(entry + offset + 4u);
             cursor += 4u) {
            const auto child = runtime.ReadWord(cursor);
            if (child != 0u)
                (void)runtime.Call(ArenaCacheEntryCall::DeleteChild,
                                   {child, 1u});
        }
        last_end = runtime.ReadWord(entry + offset + 4u);
        runtime.WriteWord(entry + offset + 4u,
                          runtime.ReadWord(entry + offset));
    }
    const auto extra = runtime.ReadWord(entry + 68u);
    if (extra == 0u) return last_end;
    const auto result = runtime.Call(ArenaCacheEntryCall::DeleteChild,
                                     {extra, 1u});
    runtime.WriteWord(entry + 68u, 0u);
    return result;
}

std::uint32_t ArenaCacheEntry::MatchesNames(
    const std::uint32_t entry, const std::uint32_t first_name,
    const std::uint32_t second_name, Runtime& runtime) {
    if (!runtime.NamesEqualIgnoreCase(runtime.ReadWord(entry + 8u),
                                      first_name))
        return 0u;
    return runtime.NamesEqualIgnoreCase(runtime.ReadWord(entry + 24u),
                                        second_name) ? 1u : 0u;
}

std::uint32_t ArenaCacheEntry::ProcessChildren(
    const std::uint32_t entry, const std::uint32_t output,
    Runtime& runtime) {
    (void)runtime.Call(ArenaCacheEntryCall::PrepareData,
                       {output, entry + 4u});
    (void)runtime.Call(ArenaCacheEntryCall::PrepareData,
                       {output, entry + 20u});
    WriteChildren(entry, 40u, output, runtime);
    WriteChildren(entry, 56u, output, runtime);
    const auto extra = runtime.ReadWord(entry + 68u);
    if (extra != 0u)
        return runtime.Call(ArenaCacheEntryCall::ProcessChild,
                            {extra, output});
    return runtime.ReadWord(entry + 60u);
}

std::uint32_t ArenaCacheEntry::Load(
    const std::uint32_t entry, const std::uint32_t data, Runtime& runtime) {
    (void)ClearChildren(entry, runtime);
    (void)runtime.Call(ArenaCacheEntryCall::ReadTextRecord,
                       {data, entry + 4u});
    (void)runtime.Call(ArenaCacheEntryCall::ReadTextRecord,
                       {data, entry + 20u});
    LoadArray(entry, 40u, data, runtime);
    LoadArray(entry, 56u, data, runtime);
    const auto minimap = runtime.Call(ArenaCacheEntryCall::FindStorageObject,
                                      {12u});
    runtime.WriteWord(entry + 68u, minimap);
    return runtime.Call(ArenaCacheEntryCall::DeserializeObject,
                        {minimap, data});
}

std::uint32_t ArenaCacheEntry::Capture(
    const std::uint32_t entry, const std::uint32_t first_name,
    const std::uint32_t second_name, Runtime& runtime) {
    (void)ClearChildren(entry, runtime);
    CopyName(entry + 4u, first_name, runtime);
    CopyName(entry + 20u, second_name, runtime);
    (void)runtime.Call(ArenaCacheEntryCall::IterateGameObjects, {entry});
    (void)runtime.Call(ArenaCacheEntryCall::IterateMonsters, {entry});
    const auto minimap = runtime.Call(ArenaCacheEntryCall::CaptureMinimap, {});
    runtime.WriteWord(entry + 68u, minimap);
    return minimap;
}

void ArenaCacheEntry::Restore(const std::uint32_t entry, Runtime& runtime) {
    RestoreArray(entry, 40u, ArenaCacheEntryCall::FindGameObjectByName,
                 ArenaCacheEntryCall::RestoreGameObject, runtime);
    RestoreArray(entry, 56u, ArenaCacheEntryCall::FindMonsterByName,
                 ArenaCacheEntryCall::RestoreMonster, runtime);
    const auto minimap = runtime.ReadWord(entry + 68u);
    if (minimap != 0u)
        (void)runtime.Call(ArenaCacheEntryCall::RestoreMinimap, {minimap});
}

std::uint32_t ArenaCacheEntry::CaptureGameObject(
    const std::uint32_t object, const std::uint32_t entry,
    Runtime& runtime) {
    const auto child = runtime.Call(ArenaCacheEntryCall::SnapshotGameObject,
                                    {object});
    AppendPointer(entry, 40u, child, runtime);
    return object;
}

std::uint32_t ArenaCacheEntry::CaptureMonster(
    const std::uint32_t monster, const std::uint32_t entry,
    Runtime& runtime) {
    const auto child = runtime.Call(ArenaCacheEntryCall::SnapshotMonster,
                                    {monster});
    AppendPointer(entry, 56u, child, runtime);
    return monster;
}

void ArenaCacheEntry::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    Runtime& runtime) {
    if (destination == 0u) return;
    for (const auto offset : {4u, 20u}) {
        runtime.WriteByte(destination + offset,
                          runtime.ReadByte(source + offset));
        ClearTriple(destination, offset + 4u, runtime);
        (void)runtime.Call(ArenaCacheEntryCall::AssignTextRecord,
                           {destination + offset, source + offset});
    }
    for (const auto offset : {36u, 52u}) {
        runtime.WriteByte(destination + offset,
                          runtime.ReadByte(source + offset));
        const auto source_begin = runtime.ReadWord(source + offset + 4u);
        const auto source_end = runtime.ReadWord(source + offset + 8u);
        const auto size = source_begin == 0u ? 0u :
                          (source_end - source_begin) / 4u;
        const auto begin = runtime.Allocate(size * 4u);
        runtime.WriteWord(destination + offset + 4u, begin);
        for (std::uint32_t i = 0u; i < size; ++i)
            runtime.WriteWord(begin + i * 4u,
                              runtime.ReadWord(source_begin + i * 4u));
        runtime.WriteWord(destination + offset + 8u, begin + size * 4u);
        runtime.WriteWord(destination + offset + 12u, begin + size * 4u);
    }
    runtime.WriteWord(destination + 68u, runtime.ReadWord(source + 68u));
    runtime.WriteWord(destination, kVtable);
}

}  // namespace pal4
