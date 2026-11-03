#include "pal4/skill_data_table.h"

#include <stdexcept>

namespace pal4 {
namespace {

class PreparedSkillRecord final {
public:
    explicit PreparedSkillRecord(ISkillDataRowRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedSkillRecord() { runtime_.DestroyTemporaryRecord(value); }

    SkillDataRecord value{};

private:
    ISkillDataRowRuntime& runtime_;
};

}  // namespace

std::uint8_t SkillDataTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    ISkillDataRowRuntime& runtime) {
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

    PreparedSkillRecord record(runtime);
    if (!runtime.ParseBoundRow(table + 108, row, record.value) ||
        record.value[1] >= 4 || record.value[2] > 14)
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(table, row));

    const auto key = record.value[0];
    const auto index_begin = runtime.ReadWord(table + 8);
    const auto index_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(index_begin, index_end, key);
    if (position == index_end ||
        static_cast<std::int32_t>(key) <
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        const auto begin = runtime.ReadWord(table + 96);
        const auto end = runtime.ReadWord(table + 100);
        const auto index = begin ? (end - begin) / kSkillRecordBytes : 0;
        runtime.InsertRecord(table + 92, end, record.value);
        runtime.InsertKey(table + 4, position, key, index);
    } else {
        const auto destination = runtime.ReadWord(table + 96) +
                                 kSkillRecordBytes * runtime.ReadWord(position + 4);
        runtime.AssignRecord(destination, record.value);
    }
    return 1;
}

}  // namespace pal4
