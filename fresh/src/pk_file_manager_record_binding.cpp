#include "pal4/pk_file_manager.h"

#include <array>
#include <cstdint>

namespace pal4 {
namespace {

constexpr std::uint32_t kPageBytes = 0xff0u;
constexpr std::uint32_t kRecordBytes = 24u;
constexpr std::array<std::uint32_t, 6> kFileFields{
    40u, 44u, 48u, 68u, 56u, 76u};
constexpr std::array<std::uint32_t, 6> kScriptCallbacks{
    0x66e9a0u, 0x66e9e0u, 0x66ea10u,
    0x66ea40u, 0x66ea60u, 0x66ead0u};

bool StoreAndReplaceCallbacks(
    const std::uint32_t object, const std::uint32_t file,
    const std::array<std::uint32_t, 6>& replacements,
    IScriptManagerCoreRuntime& memory) {
    std::array<std::uint32_t, 6> previous{};
    for (std::uint32_t index = 0; index < previous.size(); ++index)
        previous[index] = memory.ReadWord(file + kFileFields[index]);

    auto count = memory.ReadWord(object + 72u);
    std::uint32_t destination = 0;
    if (count != 0 && memory.ReadWord(object + 56u) !=
                          memory.ReadWord(object + 52u)) {
        destination = memory.ReadWord(object + 56u);
        memory.WriteWord(object + 56u, destination + kRecordBytes);
        if (destination + kRecordBytes == memory.ReadWord(object + 52u) &&
            PkFileManager::AllocateCallbackPage(object + 28u, memory) == 0u) {
            memory.WriteWord(object + 56u, destination);
            return false;
        }
    } else {
        if (PkFileManager::AllocateCallbackPage(object + 28u, memory) == 0u)
            return false;
        destination = memory.ReadWord(object + 56u);
        memory.WriteWord(object + 56u, destination + kRecordBytes);
    }

    for (std::uint32_t index = 0; index < previous.size(); ++index)
        memory.WriteWord(destination + index * 4u, previous[index]);
    memory.WriteWord(object + 72u, count + 1u);
    for (std::uint32_t index = 0; index < replacements.size(); ++index)
        memory.WriteWord(file + kFileFields[index], replacements[index]);
    return true;
}

}  // namespace

std::uint32_t PkFileManager::AllocatePage(
    const std::uint32_t array, const std::uint32_t page_bytes,
    IScriptManagerCoreRuntime& memory) {
    const auto set_iterator = [&](const std::uint32_t offset,
                                  const std::uint32_t current,
                                  const std::uint32_t cursor) {
        if (page_bytes == kPageBytes)
            (void)SetCallbackPageIterator(array + offset, current,
                                          cursor, memory);
        else
            (void)SetScriptDataPageIterator(array + offset, current,
                                            cursor, memory);
    };
    const auto page = memory.AllocateMemory(page_bytes);
    if (page == 0u) return 0u;

    if (memory.ReadWord(array + 44u) == 0u) {
        const auto map = memory.AllocateMemory(8u);
        if (map == 0u) {
            (void)memory.ReleaseMemory(page);
            return 0u;
        }
        const auto cursor = map + 4u;
        const auto midpoint = page + page_bytes / 2u;
        memory.WriteWord(array + 36u, map);
        memory.WriteWord(array + 40u, 2u);
        memory.WriteWord(cursor, page);
        set_iterator(4u, midpoint, cursor);
        set_iterator(20u, midpoint, cursor);
        return cursor;
    }

    auto cursor = memory.ReadWord(array + 32u);
    const auto map = memory.ReadWord(array + 36u);
    const auto capacity = memory.ReadWord(array + 40u);
    if (cursor >= map + capacity * 4u - 4u) {
        const auto front_cursor = memory.ReadWord(array + 16u);
        const auto slots = (cursor - front_cursor) / 4u + 1u;
        const auto new_capacity = slots * 2u;
        const auto first = GrowPageMap(array, new_capacity, memory);
        if (first == 0u) {
            (void)memory.ReleaseMemory(page);
            return 0u;
        }
        set_iterator(4u, memory.ReadWord(array + 12u), first);
        cursor = first + slots * 4u;
    } else {
        cursor += 4u;
    }
    memory.WriteWord(cursor, page);
    set_iterator(20u, page, cursor);
    return cursor;
}

std::uint32_t PkFileManager::AllocateCallbackPage(
    const std::uint32_t array, IScriptManagerCoreRuntime& memory) {
    return AllocatePage(array, kPageBytes, memory);
}

std::uint32_t PkFileManager::AllocateScriptDataPage(
    const std::uint32_t array, IScriptManagerCoreRuntime& memory) {
    return AllocatePage(array, 4096u, memory);
}

std::uint32_t PkFileManager::GrowPageMap(
    const std::uint32_t array, const std::uint32_t capacity,
    IScriptManagerCoreRuntime& memory) {
    const auto bytes = (capacity & 0x20000000u) != 0u
                           ? 0u : capacity * 4u;
    const auto new_map = memory.AllocateMemory(bytes);
    if (new_map == 0u) return 0u;
    const auto first = new_map + (capacity / 4u) * 4u;
    const auto old_first = memory.ReadWord(array + 16u);
    const auto old_end = memory.ReadWord(array + 32u) + 4u;
    for (auto source = old_first, destination = first;
         source != old_end; source += 4u, destination += 4u)
        memory.WriteWord(destination, memory.ReadWord(source));
    (void)memory.ReleaseMemory(memory.ReadWord(array + 36u));
    memory.WriteWord(array + 36u, new_map);
    memory.WriteWord(array + 40u, capacity);
    return first;
}

std::uint32_t PkFileManager::ResetPageIterator(
    const std::uint32_t iterator, IScriptManagerCoreRuntime& memory) {
    for (std::uint32_t offset = 0u; offset < 16u; offset += 4u)
        memory.WriteWord(iterator + offset, 0u);
    return iterator;
}

std::uint32_t PkFileManager::SetCallbackPageIterator(
    const std::uint32_t iterator, const std::uint32_t current,
    const std::uint32_t cursor, IScriptManagerCoreRuntime& memory) {
    const auto page = memory.ReadWord(cursor);
    memory.WriteWord(iterator, page);
    memory.WriteWord(iterator + 4u, page + kPageBytes);
    memory.WriteWord(iterator + 8u, current);
    memory.WriteWord(iterator + 12u, cursor);
    return iterator;
}

std::uint32_t PkFileManager::SetScriptDataPageIterator(
    const std::uint32_t iterator, const std::uint32_t current,
    const std::uint32_t cursor, IScriptManagerCoreRuntime& memory) {
    const auto page = memory.ReadWord(cursor);
    memory.WriteWord(iterator, page);
    memory.WriteWord(iterator + 4u, page + 4096u);
    memory.WriteWord(iterator + 8u, current);
    memory.WriteWord(iterator + 12u, cursor);
    return iterator;
}

std::uint32_t PkFileManager::RemoveCallbackPage(
    const std::uint32_t array, IScriptManagerCoreRuntime& memory) {
    const auto old_cursor = memory.ReadWord(array + 32u);
    const auto cursor = old_cursor - 4u;
    memory.WriteWord(array + 32u, cursor);
    (void)memory.ReleaseMemory(memory.ReadWord(old_cursor));
    if (memory.ReadWord(array + 44u) != 0u) {
        const auto page = memory.ReadWord(cursor);
        (void)SetCallbackPageIterator(
            array + 20u, page + kPageBytes, cursor, memory);
        return cursor;
    }

    if (memory.ReadWord(array + 16u) == cursor)
        (void)memory.ReleaseMemory(memory.ReadWord(cursor));
    for (std::uint32_t offset = 4u; offset <= 32u; offset += 4u)
        memory.WriteWord(array + offset, 0u);
    return memory.ReleaseMemory(memory.ReadWord(array + 36u));
}

std::uint32_t PkFileManager::InstallScriptCallbacks(
    const std::uint32_t object) {
    auto& memory = runtime_.Scripts();
    const auto file = memory.ReadWord(object);
    if (file == 0) return 0;
    return StoreAndReplaceCallbacks(object, file, kScriptCallbacks, memory)
               ? file : 0u;
}

std::uint32_t PkFileManager::RestoreFileCallbacks(
    const std::uint32_t object) {
    auto& memory = runtime_.Scripts();
    const auto file = memory.ReadWord(object);
    if (file == 0) return 0;
    std::array<std::uint32_t, 6> saved{};
    for (std::uint32_t index = 0; index < saved.size(); ++index)
        saved[index] = memory.ReadWord(object + 4u + index * 4u);
    return StoreAndReplaceCallbacks(object, file, saved, memory)
               ? saved.back() : 0u;
}

std::uint32_t PkFileManager::PopFileCallbacks(
    const std::uint32_t object) {
    auto& memory = runtime_.Scripts();
    const auto file = memory.ReadWord(object);
    if (file == 0) return 0;
    auto count = memory.ReadWord(object + 72u);
    if (count == 0) return 0;

    const auto back = memory.ReadWord(object + 56u);
    const auto start = memory.ReadWord(object + 48u);
    const auto cursor = memory.ReadWord(object + 60u);
    const auto record = back == start
                            ? memory.ReadWord(cursor - 4u) +
                                  kPageBytes - kRecordBytes
                            : back - kRecordBytes;
    for (std::uint32_t index = 0; index < kFileFields.size(); ++index)
        memory.WriteWord(file + kFileFields[index],
                         memory.ReadWord(record + index * 4u));

    if (back == start) {
        const auto previous_cursor = cursor - 4u;
        memory.WriteWord(object + 60u, previous_cursor);
        (void)memory.ReleaseMemory(memory.ReadWord(cursor));
        const auto previous_page = memory.ReadWord(previous_cursor);
        (void)SetCallbackPageIterator(
            object + 48u, previous_page + kPageBytes,
            previous_cursor, memory);
    }
    memory.WriteWord(object + 56u,
                     memory.ReadWord(object + 56u) - kRecordBytes);
    memory.WriteWord(object + 72u, --count);
    if (count != 0) return count;

    const auto old_cursor = memory.ReadWord(object + 60u);
    const auto previous_cursor = old_cursor - 4u;
    memory.WriteWord(object + 60u, previous_cursor);
    (void)memory.ReleaseMemory(memory.ReadWord(old_cursor));
    const auto front_cursor = memory.ReadWord(object + 44u);
    if (front_cursor == previous_cursor)
        (void)memory.ReleaseMemory(memory.ReadWord(front_cursor));
    for (std::uint32_t offset = 32u; offset <= 60u; offset += 4u)
        memory.WriteWord(object + offset, 0u);
    return memory.ReleaseMemory(memory.ReadWord(object + 64u));
}

}  // namespace pal4
