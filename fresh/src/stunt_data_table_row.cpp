#include "pal4/stunt_data_table.h"

#include <stdexcept>

namespace pal4 {
namespace {

class PreparedStuntRecord final {
public:
    explicit PreparedStuntRecord(IStuntDataRowRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedStuntRecord() {
        if (active_)
            static_cast<void>(runtime_.CleanupTemporaryRecord(value));
    }
    [[nodiscard]] std::uint8_t Finish() {
        active_ = false;
        return runtime_.CleanupTemporaryRecord(value);
    }

    StuntDataRecord value{};

private:
    IStuntDataRowRuntime& runtime_;
    bool active_ = true;
};

bool ValidStuntRecord(const StuntDataRecord& record) {
    return static_cast<std::int32_t>(record[34]) >= 0 &&
           static_cast<std::int32_t>(record[34]) <= 5 &&
           static_cast<std::int32_t>(record[1]) >= 0 &&
           static_cast<std::int32_t>(record[1]) <= 6 &&
           static_cast<std::int32_t>(record[33]) >= 0 &&
           static_cast<std::int32_t>(record[33]) <= 2 &&
           static_cast<std::int32_t>(record[32]) >= 0;
}

[[noreturn]] void ThrowInvalidRow(std::uint32_t table,
                                   std::uint32_t row,
                                   IStuntDataRowRuntime& runtime) {
    throw std::runtime_error(
        "ERROR <dbRecordBindTable< Record >::addRow >" +
        runtime.DumpRow(table, row));
}

}  // namespace

std::uint8_t StuntDataTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IStuntDataRowRuntime& runtime) {
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

    PreparedStuntRecord record(runtime);
    if (!runtime.ParseBoundRow(table + 108, row, record.value) ||
        !ValidStuntRecord(record.value))
        ThrowInvalidRow(table, row, runtime);

    const auto key = record.value[0];
    const auto index_begin = runtime.ReadWord(table + 8);
    const auto index_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(index_begin, index_end, key);
    if (position == index_end ||
        static_cast<std::int32_t>(key) <
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        const auto begin = runtime.ReadWord(table + 96);
        const auto end = runtime.ReadWord(table + 100);
        const auto index = begin ? (end - begin) / kRecordBytes : 0;
        runtime.InsertRecord(table + 92, end, record.value);
        runtime.InsertKey(table + 4, position, key, index);
    } else {
        const auto destination = runtime.ReadWord(table + 96) +
            kRecordBytes * runtime.ReadWord(position + 4);
        runtime.AssignRecord(destination, record.value);
    }
    return 1;
}

std::uint8_t StuntDataTable::Append(
    std::uint32_t table, std::uint32_t row,
    IStuntDataRowRuntime& runtime) {
    PreparedStuntRecord record(runtime);
    if (!runtime.ParseBoundRow(table + 108, row, record.value) ||
        !ValidStuntRecord(record.value))
        ThrowInvalidRow(table, row, runtime);
    runtime.AppendRecord(table + 92, runtime.ReadWord(table + 100),
                         record.value);
    return record.Finish();
}

std::uint32_t StuntDataTable::MarkAndRefresh(
    std::uint32_t table, IStuntDataRefreshRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

}  // namespace pal4
