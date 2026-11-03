#include "pal4/game_data_object_table.h"

#include "pal4/game_data_object_schema.h"

#include <stdexcept>

namespace pal4 {
namespace {

class PreparedObjectRecord final {
public:
    explicit PreparedObjectRecord(IGameDataObjectRowRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedObjectRecord() { runtime_.DestroyTemporaryRecord(value); }

    GameDataObjectRecord value{};

private:
    IGameDataObjectRowRuntime& runtime_;
};

class PreparedAppendRecord final {
public:
    explicit PreparedAppendRecord(IGameDataObjectAppendRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedAppendRecord() {
        if (active_)
            static_cast<void>(runtime_.CleanupTemporaryRecord(value));
    }
    [[nodiscard]] std::uint32_t Finish() {
        active_ = false;
        return runtime_.CleanupTemporaryRecord(value);
    }

    GameDataObjectRecord value{};

private:
    IGameDataObjectAppendRuntime& runtime_;
    bool active_ = true;
};

}  // namespace

std::uint8_t GameDataObjectTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IGameDataObjectRowRuntime& runtime, LegacyCrc32& crc) {
    const auto raw_begin = runtime.ReadWord(table + 24);
    const auto raw_count = raw_begin
        ? (runtime.ReadWord(table + 28) - raw_begin) / 60
        : 0;
    if (runtime.CollectionSize(table + 124) != raw_count) {
        runtime.LogBind("beginBindField, table:", table + 52);
        runtime.AssignEffects(table + 108, table + 20);
        static_cast<void>(GameDataObjectSchema::RegisterFields(
            table + 108, runtime));
        runtime.BindEffects(table + 108);
        runtime.LogBind("endBindField, table:", table + 52);
    }

    PreparedObjectRecord record(runtime);
    if (!runtime.ParseBoundRow(table + 108, row, record.value) ||
        !GameDataRowHashGate::Update(
            record.value[0], runtime.RecordTextData(record.value),
            record.value[3],
            static_cast<std::int32_t>(record.value[13]), crc))
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(table, row));

    const auto key = record.value[0];
    const auto key_begin = runtime.ReadWord(table + 8);
    const auto key_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(key_begin, key_end, key);
    if (position == key_end ||
        static_cast<std::int32_t>(key) <
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        const auto begin = runtime.ReadWord(table + 96);
        const auto end = runtime.ReadWord(table + 100);
        const auto index = begin ? (end - begin) / 56 : 0;
        runtime.InsertRecord(table + 92, end, record.value);
        runtime.InsertKey(table + 4, position, key, index);
    } else {
        const auto destination = runtime.ReadWord(table + 96) +
            runtime.ReadWord(position + 4) * 56;
        runtime.AssignRecord(destination, record.value);
    }
    return 1;
}

std::uint32_t GameDataObjectTable::Append(
    std::uint32_t table, std::uint32_t row,
    IGameDataObjectAppendRuntime& runtime, LegacyCrc32& crc) {
    PreparedAppendRecord record(runtime);
    const auto count = runtime.CollectionSize(table + 124);
    if (runtime.RowSize(row) != count)
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(table, row));
    for (std::uint32_t index = 0;
         index < runtime.CollectionSize(table + 124); ++index) {
        const auto fields = runtime.ReadWord(table + 128);
        const auto field = runtime.ReadWord(fields + index * 4);
        runtime.ConvertField(field, record.value,
                             runtime.PropertyAt(row, index));
    }
    if (!GameDataRowHashGate::Update(
            record.value[0], runtime.RecordTextData(record.value),
            record.value[3],
            static_cast<std::int32_t>(record.value[13]), crc))
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(table, row));
    runtime.AppendRecord(table + 92, runtime.ReadWord(table + 100),
                         record.value);
    return record.Finish();
}

std::uint32_t GameDataObjectTable::MarkAndRefresh(
    std::uint32_t table, IGameDataObjectRefreshRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

std::uint32_t GameDataObjectTable::RefreshBindings(
    std::uint32_t table, IGameDataObjectBindingRefreshRuntime& runtime) {
    const auto field_begin = runtime.ReadWord(table + 128);
    const auto field_count = field_begin
        ? static_cast<std::int32_t>(
              runtime.ReadWord(table + 132) - field_begin) / 4 : 0;
    const auto source_begin = runtime.ReadWord(table + 24);
    const auto source_count = source_begin
        ? static_cast<std::int32_t>(
              runtime.ReadWord(table + 28) - source_begin) / 60 : 0;
    if (field_count == source_count)
        return source_begin ? (source_count < 0 ? 1u : 0u) : table + 20;

    try {
        runtime.LogBind("beginBindField, table:", table + 52);
        runtime.AssignEffects(table + 108, table + 20);
        static_cast<void>(GameDataObjectSchema::RegisterFields(
            table + 108, runtime));
        runtime.BindEffects(table + 108);
        runtime.LogBind("endBindField, table:", table + 52);
    } catch (...) {
        static_cast<void>(runtime.ReleaseLogBuffer());
        throw;
    }
    return runtime.ReleaseLogBuffer();
}

}  // namespace pal4
