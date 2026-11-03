#include "pal4/game_data_role_record_array.h"

namespace pal4 {

std::uint32_t GameDataRoleRecordArray::Count(
    std::uint32_t array, IGameDataRoleRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    return begin
               ? (runtime.ReadWord(array + 8) - begin) / kRecordBytes
               : 0;
}

std::uint32_t GameDataRoleRecordArray::CopyConstructRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IGameDataRoleRecordArrayRuntime& runtime) {
    for (auto source = begin; source != end;
         source += kRecordBytes, destination += kRecordBytes)
        runtime.CopyConstruct(destination, source);
    return destination;
}

std::uint32_t GameDataRoleRecordArray::InsertOne(
    std::uint32_t array, std::uint32_t position, std::uint32_t source,
    IGameDataRoleRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto old_end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto index = (position - begin) / kRecordBytes;

    if ((capacity_end - old_end) / kRecordBytes) {
        if ((old_end - position) / kRecordBytes) {
            static_cast<void>(CopyConstructRange(
                old_end - kRecordBytes, old_end, old_end, runtime));
            runtime.AssignBackward(position, old_end - kRecordBytes,
                                   old_end);
            runtime.AssignRange(position, position + kRecordBytes, source);
        } else {
            static_cast<void>(CopyConstructRange(
                position, old_end, position + kRecordBytes, runtime));
            runtime.FillConstruct(old_end, 1, source);
            runtime.AssignRange(position, old_end, source);
        }
        runtime.WriteWord(array + 8, old_end + kRecordBytes);
    } else {
        const auto old_count = Count(array, runtime);
        const auto growth = old_count > 1 ? old_count : 1u;
        const auto new_capacity = old_count + growth;
        const auto alloc_count = static_cast<std::int32_t>(new_capacity) < 0
                                     ? 0u : new_capacity;
        const auto new_begin = runtime.Allocate(kRecordBytes * alloc_count);
        auto destination = CopyConstructRange(begin, position, new_begin,
                                              runtime);
        runtime.CopyConstruct(destination, source);
        destination += kRecordBytes;
        static_cast<void>(CopyConstructRange(
            position, old_end, destination, runtime));
        runtime.DestroyRange(begin, old_end);
        runtime.Free(begin);
        runtime.WriteWord(array + 12,
                          new_begin + new_capacity * kRecordBytes);
        const auto count = Count(array, runtime);
        runtime.WriteWord(array + 4, new_begin);
        runtime.WriteWord(array + 8,
                          new_begin + (count + 1) * kRecordBytes);
    }
    return runtime.ReadWord(array + 4) + index * kRecordBytes;
}

void GameDataRoleRecordArray::InsertCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IGameDataRoleRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto old_end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto available = (capacity_end - old_end) / kRecordBytes;
    const auto tail_count = (old_end - position) / kRecordBytes;
    const auto inserted_bytes = count * kRecordBytes;

    if (available >= count) {
        if (!count) return;
        if (tail_count >= count) {
            static_cast<void>(CopyConstructRange(
                old_end - inserted_bytes, old_end, old_end, runtime));
            if (position != old_end - inserted_bytes)
                runtime.AssignBackward(position, old_end - inserted_bytes,
                                       old_end);
            runtime.AssignRange(position, position + inserted_bytes, source);
        } else {
            if (position != old_end)
                static_cast<void>(CopyConstructRange(
                    position, old_end, position + inserted_bytes, runtime));
            runtime.FillConstruct(old_end, count - tail_count, source);
            if (position != old_end)
                runtime.AssignRange(position, old_end, source);
        }
        runtime.WriteWord(array + 8, old_end + inserted_bytes);
        return;
    }

    const auto old_count = Count(array, runtime);
    const auto growth = old_count && old_count > count ? old_count : count;
    const auto new_capacity = old_count + growth;
    const auto alloc_count = static_cast<std::int32_t>(new_capacity) < 0
                                 ? 0u : new_capacity;
    const auto new_begin = runtime.Allocate(kRecordBytes * alloc_count);
    auto destination = CopyConstructRange(begin, position, new_begin,
                                          runtime);
    if (count) runtime.FillConstruct(destination, count, source);
    destination += inserted_bytes;
    static_cast<void>(CopyConstructRange(position, old_end,
                                         destination, runtime));
    runtime.DestroyRange(begin, old_end);
    runtime.Free(begin);
    runtime.WriteWord(array + 12,
                      new_begin + new_capacity * kRecordBytes);
    const auto retained = Count(array, runtime);
    runtime.WriteWord(array + 4, new_begin);
    runtime.WriteWord(array + 8,
                      new_begin + (retained + count) * kRecordBytes);
}

void GameDataRoleRecordArray::FillConstructRange(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source,
    IGameDataRoleRecordArrayRuntime& runtime) {
    for (std::uint32_t i = 0; i < count; ++i,
         destination += kRecordBytes)
        runtime.CopyConstruct(destination, source);
}

}  // namespace pal4
