#include "pal4/game_question_data_table.h"

#include <stdexcept>

namespace pal4 {
namespace {

class PreparedQuestionRecord final {
public:
    explicit PreparedQuestionRecord(IGameQuestionDataRowRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedQuestionRecord() {
        if (active_)
            static_cast<void>(runtime_.CleanupTemporaryRecord(value));
    }
    [[nodiscard]] std::uint32_t Finish() {
        active_ = false;
        return runtime_.CleanupTemporaryRecord(value);
    }

    GameQuestionDataRecord value{};

private:
    IGameQuestionDataRowRuntime& runtime_;
    bool active_ = true;
};

[[noreturn]] void ThrowInvalidRow(
    std::uint32_t table, std::uint32_t row,
    IGameQuestionDataRowRuntime& runtime) {
    throw std::runtime_error(
        "ERROR <dbRecordBindTable< Record >::addRow >" +
        runtime.DumpRow(table, row));
}

}  // namespace

std::uint8_t GameQuestionDataTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IGameQuestionDataRowRuntime& runtime) {
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

    PreparedQuestionRecord record(runtime);
    if (!runtime.ParseBoundRow(table + 108, row, record.value))
        ThrowInvalidRow(table, row, runtime);

    const auto key = record.value[0];
    const auto index_begin = runtime.ReadWord(table + 8);
    const auto index_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(index_begin, index_end, key);
    if (position == index_end ||
        static_cast<std::int32_t>(key) <
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        runtime.InsertRecord(table + 92,
                             runtime.ReadWord(table + 100), record.value);
        runtime.InsertKey(table + 4, position, key);
    } else {
        const auto destination = runtime.ReadWord(table + 96) +
            76 * runtime.ReadWord(position + 4);
        runtime.AssignRecord(destination, record.value);
    }
    return 1;
}

std::uint32_t GameQuestionDataTable::Append(
    std::uint32_t table, std::uint32_t row,
    IGameQuestionDataRowRuntime& runtime) {
    PreparedQuestionRecord record(runtime);
    const auto row_size = runtime.RowSize(row);
    if (row_size != runtime.CollectionSize(table + 124))
        ThrowInvalidRow(table, row, runtime);

    for (std::uint32_t index = 0;
         index < static_cast<std::uint32_t>(runtime.CollectionSize(table + 124));
         ++index) {
        const auto field = runtime.FieldAt(table + 128, index);
        const auto property = runtime.PropertyAt(row, index);
        runtime.ConvertField(field, record.value, property);
    }
    runtime.AppendRecord(table + 92,
                         runtime.ReadWord(table + 100), record.value);
    return record.Finish();
}

std::uint32_t GameQuestionDataTable::MarkAndRefresh(
    std::uint32_t table, IStoryDataRefreshRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

}  // namespace pal4
