#include "pal4/game_data_property_table.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

constexpr std::string_view kTableName = "Property";
constexpr std::string_view kPropertyName = "PropertyClass";

template <typename Runtime>
class PreparedRecord final {
public:
    explicit PreparedRecord(Runtime& runtime) : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedRecord() { runtime_.DestroyTemporaryRecordTexts(value); }
    GameDataPropertyRecord value{};

private:
    Runtime& runtime_;
};

std::string InvalidRow(const std::string& detail) {
    return "ERROR <dbRecordBindTable< Record >::addRow >" + detail;
}

bool ValidRecord(const GameDataPropertyRecord& record) {
    return record[3] <= 5u && record[1] <= 4u && record[2] <= 6u;
}

void ThrowMissing(std::string_view name,
                  IGameDataPropertyTableLoadRuntime& runtime) {
    const auto message =
        "ERROR [ palDBPropertyManager::verifyDataTable < the table " +
        std::string(name) + "  doesn't exist>]";
    runtime.LogError(message);
    throw std::runtime_error(message);
}

}  // namespace

bool GameDataPropertyTable::IsValidCategory(
    std::uint32_t record, IGameDataPropertyTableRuntime& runtime) {
    return runtime.ReadWord(record + 12) <= 5u &&
           runtime.ReadWord(record + 4) <= 4u &&
           runtime.ReadWord(record + 8) <= 6u;
}

bool GameDataPropertyTable::IsAiCommandBelowSix(
    std::uint32_t record, IGameDataPropertyTableRuntime& runtime) {
    return static_cast<std::int32_t>(runtime.ReadWord(record + 8)) < 6;
}

bool GameDataPropertyTable::CanUseRecord(
    std::uint32_t record, IGameDataPropertyTableRuntime& runtime) {
    if (!runtime.ReadWord(record + 4)) return true;
    const auto id = runtime.ReadWord(record);
    return (id == 3067 || id == 3182) && runtime.CanStartAction() > 0;
}

std::uint32_t GameDataPropertyTable::Find(
    std::uint32_t table, std::int32_t key,
    IGameDataPropertyTableRuntime& runtime) {
    const auto effect = runtime.ReadWord(table + 4);
    auto first = runtime.ReadWord(effect + 8);
    const auto end = runtime.ReadWord(effect + 12);
    auto count = static_cast<std::int32_t>((end - first) / 8);
    while (count > 0) {
        const auto half = count / 2;
        const auto candidate = first + static_cast<std::uint32_t>(half) * 8;
        if (static_cast<std::int32_t>(runtime.ReadWord(candidate)) < key) {
            first = candidate + 8;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    if (first == end ||
        key < static_cast<std::int32_t>(runtime.ReadWord(first)))
        return 0;
    return runtime.ReadWord(effect + 96) +
           runtime.ReadWord(first + 4) * 168;
}

std::uint32_t GameDataPropertyTable::FindOrLog(
    std::uint32_t table, std::int32_t key,
    IGameDataPropertyTableRuntime& runtime) {
    const auto address = Find(table, key, runtime);
    if (!address) runtime.LogMissingRecord();
    return address;
}

bool GameDataPropertyTable::Contains(
    std::uint32_t table, std::int32_t key,
    IGameDataPropertyTableRuntime& runtime) {
    return Find(table, key, runtime) != 0;
}

std::uint8_t GameDataPropertyTable::Preload(
    std::uint32_t manager, IGameDataPropertyTableLoadRuntime& runtime) {
    return runtime.PreloadTable(runtime.ReadWord(manager), kTableName,
                                0x4e57e0);
}

std::uint8_t GameDataPropertyTable::Verify(
    std::uint32_t manager, IGameDataPropertyTableLoadRuntime& runtime) {
    const auto database = runtime.ReadWord(manager);
    const auto table = runtime.FindTable(database, kTableName);
    runtime.WriteWord(manager + 4, table);
    if (!table) ThrowMissing(kTableName, runtime);
    const auto property = runtime.FindProperty(database, kPropertyName);
    runtime.WriteWord(manager + 8, property);
    if (!property) ThrowMissing(kPropertyName, runtime);
    return 1;
}

std::uint8_t GameDataPropertyTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IGameDataPropertyRowRuntime& runtime) {
    const auto fields = runtime.ReadWord(table + 24);
    const auto field_count = fields
                                 ? (runtime.ReadWord(table + 28) - fields) / 60
                                 : 0;
    if (runtime.CollectionSize(table + 124) != field_count) {
        runtime.LogBind("beginBindField, table:", table + 52);
        runtime.AssignEffects(table + 108, table + 20);
        runtime.InitializeStructures(table + 108);
        runtime.BindEffects(table + 108);
        runtime.LogBind("endBindField, table:", table + 52);
    }

    PreparedRecord record(runtime);
    if (!runtime.ParseBoundRow(table + 108, row, record.value) ||
        !ValidRecord(record.value))
        throw std::runtime_error(InvalidRow(runtime.DumpRow(table, row)));

    const auto key = record.value[0];
    const auto key_begin = runtime.ReadWord(table + 8);
    const auto key_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(key_begin, key_end, key);
    if (position != key_end &&
        static_cast<std::int32_t>(key) >=
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        runtime.CopyRecord(runtime.ReadWord(table + 96) +
                               runtime.ReadWord(position + 4) * 168,
                           record.value);
    } else {
        const auto records = runtime.ReadWord(table + 96);
        const auto end = runtime.ReadWord(table + 100);
        const auto index = records ? (end - records) / 168 : 0;
        runtime.InsertRecord(table + 92, end, record.value);
        runtime.InsertKey(table + 4, position, key, index);
    }
    return 1;
}

void GameDataPropertyTable::Append(
    std::uint32_t table, std::uint32_t row,
    IGameDataPropertyAppendRuntime& runtime) {
    PreparedRecord record(runtime);
    if (runtime.RowSize(row) != runtime.CollectionSize(table + 124))
        throw std::runtime_error(InvalidRow(runtime.DumpRow(table, row)));
    for (std::uint32_t i = 0; i < runtime.CollectionSize(table + 124); ++i) {
        const auto fields = runtime.ReadWord(table + 128);
        const auto field = runtime.ReadWord(fields + i * 4);
        runtime.ConvertField(field, record.value, runtime.PropertyAt(row, i));
    }
    if (!ValidRecord(record.value))
        throw std::runtime_error(InvalidRow(runtime.DumpRow(table, row)));
    runtime.AppendRecord(table + 92, runtime.ReadWord(table + 100),
                         record.value);
}

std::uint32_t GameDataPropertyTable::MarkAndRefresh(
    std::uint32_t table, IGameDataPropertyRefreshRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

void GameDataPropertyTable::RefreshBinding(
    std::uint32_t table, IGameDataPropertyBindingRuntime& runtime) {
    const auto pointers = runtime.ReadWord(table + 128);
    const auto pointer_count = pointers
                                   ? (runtime.ReadWord(table + 132) -
                                      pointers) / 4
                                   : 0;
    const auto fields = runtime.ReadWord(table + 24);
    const auto field_count = fields
                                 ? (runtime.ReadWord(table + 28) -
                                    fields) / 60
                                 : 0;
    if (pointer_count == field_count) return;
    runtime.LogBind("beginBindField, table:", table + 52);
    runtime.AssignEffects(table + 108, table + 20);
    runtime.InitializeStructures(table + 108);
    runtime.BindEffects(table + 108);
    runtime.LogBind("endBindField, table:", table + 52);
}

void GameDataPropertyIndex::Rebuild(
    std::vector<GameDataPropertyRecord>& records,
    std::vector<GameDataPropertyKeyIndex>& index,
    IGameDataPropertyIndexRuntime& runtime) {
    std::size_t retained = 0;
    for (std::size_t source = 0; source < records.size(); ++source) {
        if (retained && records[source][0] == records[retained - 1][0])
            continue;
        if (retained != source)
            runtime.AssignRecord(records[retained], records[source]);
        ++retained;
    }
    for (std::size_t i = retained; i < records.size(); ++i)
        runtime.DestroyRecord(records[i]);
    records.resize(retained);
    index.clear();
    index.reserve(records.size());
    for (std::uint32_t i = 0; i < records.size(); ++i)
        index.push_back({static_cast<std::int32_t>(records[i][0]), i});
    std::stable_sort(index.begin(), index.end(),
                     [](const auto& left, const auto& right) {
                         return left.key < right.key;
                     });
}

}  // namespace pal4
