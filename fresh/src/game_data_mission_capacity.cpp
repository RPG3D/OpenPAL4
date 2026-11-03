#include "pal4/game_data_mission_capacity.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 68;
constexpr std::uint32_t kKeyBytes = 8;

}  // namespace

std::uint32_t GameDataMissionCapacity::Reserve(
    std::uint32_t table, std::int32_t requested,
    IGameDataMissionCapacityRuntime& runtime) {
    const auto desired = static_cast<std::uint32_t>(requested);
    const auto old_records = runtime.ReadWord(table + 96);
    const auto record_capacity = old_records
                                     ? (runtime.ReadWord(table + 104) -
                                        old_records) /
                                           kRecordBytes
                                     : 0;
    if (record_capacity < desired) {
        const auto bytes = static_cast<std::uint32_t>(
            requested < 0 ? 0 : requested) * kRecordBytes;
        const auto replacement = runtime.Allocate(bytes);
        const auto old_end = runtime.ReadWord(table + 100);
        const auto old_begin = runtime.ReadWord(table + 96);
        auto destination = replacement;
        for (auto source = old_begin; source != old_end;
             source += kRecordBytes, destination += kRecordBytes)
            runtime.CopyRecord(destination, source);
        for (auto source = old_begin; source != old_end;
             source += kRecordBytes)
            runtime.DestroyRecord(source);
        runtime.Free(old_begin);

        const auto count = old_begin
                               ? (old_end - old_begin) / kRecordBytes
                               : 0;
        runtime.WriteWord(table + 104,
                          replacement + desired * kRecordBytes);
        runtime.WriteWord(table + 96, replacement);
        runtime.WriteWord(table + 100,
                          replacement + count * kRecordBytes);
    }

    const auto old_keys = runtime.ReadWord(table + 8);
    const auto key_capacity = old_keys
                                  ? static_cast<std::uint32_t>(
                                        static_cast<std::int32_t>(
                                            runtime.ReadWord(table + 16) -
                                            old_keys) >> 3)
                                  : 0;
    if (key_capacity >= desired) return key_capacity;

    const auto bytes = static_cast<std::uint32_t>(
        requested < 0 ? 0 : requested) * kKeyBytes;
    const auto replacement = runtime.Allocate(bytes);
    const auto old_end = runtime.ReadWord(table + 12);
    const auto old_begin = runtime.ReadWord(table + 8);
    auto destination = replacement;
    for (auto source = old_begin; source != old_end;
         source += kKeyBytes, destination += kKeyBytes) {
        if (destination != 0) {
            runtime.WriteWord(destination, runtime.ReadWord(source));
            runtime.WriteWord(destination + 4, runtime.ReadWord(source + 4));
        }
    }
    runtime.Free(old_begin);
    runtime.WriteWord(table + 16, replacement + desired * kKeyBytes);
    const auto count = old_begin
                           ? static_cast<std::uint32_t>(
                                 static_cast<std::int32_t>(old_end -
                                                           old_begin) >> 3)
                           : 0;
    runtime.WriteWord(table + 8, replacement);
    const auto new_end = replacement + count * kKeyBytes;
    runtime.WriteWord(table + 12, new_end);
    return new_end;
}

}  // namespace pal4
