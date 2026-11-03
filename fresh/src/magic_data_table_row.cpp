#include "pal4/magic_data_table.h"

#include <stdexcept>

namespace pal4 {
namespace {

class PreparedMagicRecord final {
public:
    explicit PreparedMagicRecord(IMagicDataRowRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedMagicRecord() {
        if (active_)
            static_cast<void>(runtime_.CleanupTemporaryRecord(value));
    }
    [[nodiscard]] std::uint8_t Finish() {
        active_ = false;
        return runtime_.CleanupTemporaryRecord(value);
    }

    MagicDataRecord value{};

private:
    IMagicDataRowRuntime& runtime_;
    bool active_ = true;
};

bool ValidMagicRecord(const MagicDataRecord& record) {
    return static_cast<std::int32_t>(record[2]) >= 0 &&
           static_cast<std::int32_t>(record[2]) <= 6 &&
           static_cast<std::int32_t>(record[1]) >= 0 &&
           static_cast<std::int32_t>(record[1]) <= 4 &&
           static_cast<std::int32_t>(record[20]) >= 0 &&
           static_cast<std::int32_t>(record[20]) <= 5;
}

[[noreturn]] void ThrowInvalidRow(std::uint32_t table,
                                   std::uint32_t row,
                                   IMagicDataRowRuntime& runtime) {
    throw std::runtime_error(
        "ERROR <dbRecordBindTable< Record >::addRow >" +
        runtime.DumpRow(table, row));
}

}  // namespace

std::uint8_t MagicDataTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IMagicDataRowRuntime& runtime) {
    const auto source_begin = runtime.ReadWord(table + 24);
    const auto source_count = source_begin
        ? static_cast<std::int32_t>(
              runtime.ReadWord(table + 28) - source_begin) / 60
        : 0;
    if (runtime.CollectionSize(table + 124) != source_count) {
        runtime.LogBind("beginBindField, table:", table + 52);
        runtime.AssignEffects(table + 108, table + 20);
        runtime.ProcessPropertyRegistry(table + 108);
        runtime.BindEffects(table + 108);
        runtime.LogBind("endBindField, table:", table + 52);
    }

    PreparedMagicRecord record(runtime);
    if (!runtime.ParseBoundRow(table + 108, row, record.value) ||
        !ValidMagicRecord(record.value))
        ThrowInvalidRow(table, row, runtime);

    const auto key = record.value[0];
    const auto index = static_cast<std::uint32_t>(
        RecordCount(table + 92, runtime));
    const auto index_begin = runtime.ReadWord(table + 8);
    const auto index_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(index_begin, index_end, key);
    if (position == index_end ||
        static_cast<std::int32_t>(key) <
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        runtime.InsertRecord(table + 92, runtime.ReadWord(table + 100),
                             record.value);
        runtime.InsertKey(table + 4, position, key, index);
    } else {
        const auto destination = runtime.ReadWord(table + 96) +
            kRecordBytes * runtime.ReadWord(position + 4);
        runtime.AssignRecord(destination, record.value);
    }
    return 1;
}

std::int32_t MagicDataTable::RecordCount(
    std::uint32_t array, IMagicDataCountRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    if (!begin) return 0;
    return static_cast<std::int32_t>(runtime.ReadWord(array + 8) - begin) /
           static_cast<std::int32_t>(kRecordBytes);
}

std::uint8_t MagicDataTable::Append(
    std::uint32_t table, std::uint32_t row,
    IMagicDataRowRuntime& runtime) {
    PreparedMagicRecord record(runtime);
    if (!runtime.ParseBoundRow(table + 108, row, record.value) ||
        !ValidMagicRecord(record.value))
        ThrowInvalidRow(table, row, runtime);
    runtime.AppendRecord(table + 92, runtime.ReadWord(table + 100),
                         record.value);
    return record.Finish();
}

std::uint32_t MagicDataTable::MarkAndRefresh(
    std::uint32_t table, IMagicDataRefreshRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

}  // namespace pal4
