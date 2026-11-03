#include "pal4/game_data_ai_record_capacity.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kKeyBytes = 8;

template <typename Record, typename Runtime>
std::uint32_t ReserveRecords(
    std::uint32_t table, std::uint32_t requested,
    std::uint32_t record_bytes, Runtime& runtime) {
    const auto old_records = runtime.ReadWord(table + 96);
    const auto record_capacity = old_records
                                     ? (runtime.ReadWord(table + 104) -
                                        old_records) /
                                           record_bytes
                                     : 0;
    if (record_capacity < requested) {
        const auto allocation_count =
            static_cast<std::int32_t>(requested) < 0 ? 0u : requested;
        const auto replacement = runtime.Allocate(allocation_count * record_bytes);
        const auto old_end = runtime.ReadWord(table + 100);
        const auto record_count = old_records
                                      ? (old_end - old_records) / record_bytes
                                      : 0;
        for (std::uint32_t i = 0; i < record_count; ++i) {
            const Record record =
                runtime.ReadRecord(old_records + i * record_bytes);
            if (replacement)
                runtime.WriteRecord(replacement + i * record_bytes, record);
        }
        runtime.Free(old_records);
        runtime.WriteWord(table + 104,
                          replacement + requested * record_bytes);
        runtime.WriteWord(table + 96, replacement);
        runtime.WriteWord(table + 100,
                          replacement + record_count * record_bytes);
    }

    const auto old_keys = runtime.ReadWord(table + 8);
    const auto key_capacity = old_keys
                                  ? (runtime.ReadWord(table + 16) - old_keys) /
                                        kKeyBytes
                                  : 0;
    if (key_capacity >= requested) return key_capacity;

    const auto allocation_count =
        static_cast<std::int32_t>(requested) < 0 ? 0u : requested;
    const auto replacement = runtime.Allocate(allocation_count * kKeyBytes);
    const auto old_end = runtime.ReadWord(table + 12);
    const auto key_count = old_keys ? (old_end - old_keys) / kKeyBytes : 0;
    for (std::uint32_t i = 0; i < key_count; ++i) {
        const auto entry = runtime.ReadKey(old_keys + i * kKeyBytes);
        if (replacement)
            runtime.WriteKey(replacement + i * kKeyBytes, entry);
    }
    runtime.Free(old_keys);
    runtime.WriteWord(table + 16, replacement + requested * kKeyBytes);
    runtime.WriteWord(table + 8, replacement);
    const auto new_end = replacement + key_count * kKeyBytes;
    runtime.WriteWord(table + 12, new_end);
    return new_end;
}

}  // namespace

std::uint32_t GameDataAiRecordCapacity::Reserve(
    std::uint32_t table, std::uint32_t requested,
    IGameDataAiRecordCapacityRuntime& runtime) {
    return ReserveRecords<GameDataAiRecord>(table, requested, 56, runtime);
}

std::uint32_t GameDataAiPropertyCapacity::Reserve(
    std::uint32_t table, std::uint32_t requested,
    IGameDataAiPropertyCapacityRuntime& runtime) {
    return ReserveRecords<GameDataAiPropertyRecord>(table, requested, 68,
                                                     runtime);
}

}  // namespace pal4
