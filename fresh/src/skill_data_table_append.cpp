#include "pal4/skill_data_table.h"

#include <stdexcept>

namespace pal4 {
namespace {

class PreparedAppendSkillRecord final {
public:
    explicit PreparedAppendSkillRecord(ISkillDataAppendRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedAppendSkillRecord() {
        if (active_)
            static_cast<void>(runtime_.CleanupTemporaryRecord(value));
    }
    [[nodiscard]] std::uint8_t Finish() {
        active_ = false;
        return runtime_.CleanupTemporaryRecord(value);
    }

    SkillDataRecord value{};

private:
    ISkillDataAppendRuntime& runtime_;
    bool active_ = true;
};

}  // namespace

std::uint8_t SkillDataTable::Append(
    std::uint32_t table, std::uint32_t row,
    ISkillDataAppendRuntime& runtime) {
    PreparedAppendSkillRecord record(runtime);
    const auto row_size = runtime.RowSize(row);
    const auto bound_count = runtime.CollectionSize(table + 124);
    if (row_size != bound_count)
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(table, row));

    for (std::uint32_t index = 0;
         index < runtime.CollectionSize(table + 124); ++index) {
        const auto fields = runtime.ReadWord(table + 128);
        const auto field = runtime.ReadWord(fields + 4 * index);
        runtime.ConvertField(field, record.value,
                             runtime.PropertyAt(row, index));
    }
    if (record.value[1] >= 4 || record.value[2] > 14)
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(table, row));

    runtime.AppendRecord(table + 92, runtime.ReadWord(table + 100),
                         record.value);
    return record.Finish();
}

std::uint32_t SkillDataTable::MarkAndRefresh(
    std::uint32_t table, ISkillDataRefreshRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

}  // namespace pal4
