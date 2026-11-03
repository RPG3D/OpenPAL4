#include "pal4/game_data_mission_table.h"

#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

constexpr std::string_view kTableName = "Mission";
constexpr std::string_view kMissingTable =
    "ERROR [ palDBMissionManager::verifyDataTable < the table Mission  "
    "doesn't exist>]";
constexpr std::uint32_t kMissionRecordBytes = 68;
constexpr std::uint32_t kMissionFieldBytes = 60;

class PreparedMissionRecord final {
public:
    explicit PreparedMissionRecord(IGameDataMissionRowRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedMissionRecord() { runtime_.DestroyRecord(value); }
    PreparedMissionRecord(const PreparedMissionRecord&) = delete;
    PreparedMissionRecord& operator=(const PreparedMissionRecord&) = delete;
    GameDataMissionRecord value{};

private:
    IGameDataMissionRowRuntime& runtime_;
};

class PreparedAppendRecord final {
public:
    explicit PreparedAppendRecord(IGameDataMissionAppendRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedAppendRecord() {
        if (active_) (void)runtime_.DestroyRecord(value);
    }
    PreparedAppendRecord(const PreparedAppendRecord&) = delete;
    PreparedAppendRecord& operator=(const PreparedAppendRecord&) = delete;
    std::uint32_t Finish() {
        active_ = false;
        return runtime_.DestroyRecord(value);
    }
    GameDataMissionRecord value{};

private:
    IGameDataMissionAppendRuntime& runtime_;
    bool active_ = true;
};

}  // namespace

std::uint32_t GameDataMissionTable::FindRecord(
    std::uint32_t manager, std::int32_t key,
    IGameDataMissionTableRuntime& runtime) {
    const auto table = runtime.ReadWord(manager + 4);
    auto begin = runtime.ReadWord(table + 8);
    const auto end = runtime.ReadWord(table + 12);
    auto count = static_cast<std::int32_t>(end - begin) / 8;
    while (count > 0) {
        const auto half = count / 2;
        const auto mid = begin + static_cast<std::uint32_t>(half) * 8;
        if (static_cast<std::int32_t>(runtime.ReadWord(mid)) >= key) {
            count = half;
        } else {
            begin = mid + 8;
            count -= half + 1;
        }
    }
    if (begin == end ||
        static_cast<std::int32_t>(runtime.ReadWord(begin)) != key)
        return 0;
    const auto index = runtime.ReadWord(begin + 4);
    return runtime.ReadWord(table + 96) + index * 68;
}

std::uint8_t GameDataMissionTable::Preload(
    std::uint32_t manager, IGameDataMissionTableRuntime& runtime) {
    return runtime.PreloadTable(runtime.ReadWord(manager), kTableName,
                                0x4db740);
}

std::uint8_t GameDataMissionTable::Verify(
    std::uint32_t manager, IGameDataMissionTableRuntime& runtime) {
    const auto table = runtime.FindTable(runtime.ReadWord(manager), kTableName);
    runtime.WriteWord(manager + 4, table);
    if (!table) {
        runtime.LogError(kMissingTable);
        throw std::runtime_error(std::string(kMissingTable));
    }
    return 1;
}

bool GameDataMissionTable::CompareMemberAt8(
    std::uint32_t left, std::uint32_t right,
    IGameDataMissionTableRuntime& runtime) {
    return static_cast<std::int32_t>(runtime.ReadWord(left + 8)) >
           static_cast<std::int32_t>(runtime.ReadWord(right + 8));
}

std::uint8_t GameDataMissionTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IGameDataMissionRowRuntime& runtime) {
    const auto fields = runtime.ReadWord(table + 24);
    const auto field_count = fields
                                 ? (runtime.ReadWord(table + 28) - fields) /
                                       kMissionFieldBytes
                                 : 0;
    if (runtime.CollectionSize(table + 124) != field_count) {
        runtime.LogBind("beginBindField, table:", table + 52);
        runtime.AssignEffects(table + 108, table + 20);
        runtime.InitializeStructures(table + 108);
        runtime.BindEffects(table + 108);
        runtime.LogBind("endBindField, table:", table + 52);
    }

    PreparedMissionRecord record(runtime);
    if (!runtime.ParseBoundRow(table + 108, row, record.value)) {
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(table, row));
    }

    const auto key = record.value[0];
    const auto key_begin = runtime.ReadWord(table + 8);
    const auto key_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(key_begin, key_end, key);
    if (position != key_end &&
        static_cast<std::int32_t>(key) >=
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        runtime.CopyRecord(runtime.ReadWord(table + 96) +
                               runtime.ReadWord(position + 4) *
                                   kMissionRecordBytes,
                           record.value);
    } else {
        const auto records = runtime.ReadWord(table + 96);
        const auto end = runtime.ReadWord(table + 100);
        const auto index = records ?
                               (end - records) / kMissionRecordBytes : 0;
        runtime.InsertRecord(table + 92, end, record.value);
        runtime.InsertKey(table + 4, position, key, index);
    }
    return 1;
}

std::uint32_t GameDataMissionTable::Append(
    std::uint32_t table, std::uint32_t row,
    IGameDataMissionAppendRuntime& runtime) {
    PreparedAppendRecord record(runtime);
    const auto row_size = runtime.RowSize(row);
    if (row_size != runtime.CollectionSize(table + 124)) {
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(table, row));
    }
    for (std::uint32_t i = 0; i < runtime.CollectionSize(table + 124); ++i) {
        const auto field_pointers = runtime.ReadWord(table + 128);
        const auto field = runtime.ReadWord(field_pointers + 4 * i);
        const auto property = runtime.PropertyAt(row, i);
        runtime.ConvertField(field, record.value, property);
    }
    runtime.AppendRecord(table + 92, runtime.ReadWord(table + 100),
                         record.value);
    return record.Finish();
}

std::uint32_t GameDataMissionTable::MarkAndRefresh(
    std::uint32_t table, IGameDataMissionRefreshRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

void GameDataMissionTable::RefreshBinding(
    std::uint32_t table, IGameDataMissionBindingRuntime& runtime) {
    const auto bound_begin = runtime.ReadWord(table + 128);
    const auto bound_count = bound_begin
                                 ? (runtime.ReadWord(table + 132) -
                                    bound_begin) / 4
                                 : 0;
    const auto field_begin = runtime.ReadWord(table + 24);
    const auto field_count = field_begin
                                 ? (runtime.ReadWord(table + 28) -
                                    field_begin) / kMissionFieldBytes
                                 : 0;
    if (bound_count == field_count) return;
    runtime.LogBind("beginBindField, table:", table + 52);
    runtime.AssignEffects(table + 108, table + 20);
    runtime.InitializeStructures(table + 108);
    runtime.BindEffects(table + 108);
    runtime.LogBind("endBindField, table:", table + 52);
}

}  // namespace pal4
