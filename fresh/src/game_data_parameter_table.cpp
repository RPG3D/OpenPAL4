#include "pal4/game_data_parameter_table.h"

#include <stdexcept>
#include <string>

namespace pal4 {

namespace {

constexpr std::string_view kTableName = "Prescription";
constexpr std::string_view kPropertyName = "PrescriptionType";

class PreparedRow final {
public:
    explicit PreparedRow(IGameDataParameterRowRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedRow() { runtime_.DestroyTemporaryRecordTexts(value); }
    GameDataParameterRecord value{};

private:
    IGameDataParameterRowRuntime& runtime_;
};

class PreparedAppend final {
public:
    explicit PreparedAppend(IGameDataParameterAppendRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedAppend() { runtime_.DestroyTemporaryRecordTexts(value); }
    GameDataParameterRecord value{};

private:
    IGameDataParameterAppendRuntime& runtime_;
};

std::string InvalidRow(const std::string& detail) {
    return "ERROR <dbRecordBindTable< Record >::addRow >" + detail;
}

void ThrowMissing(std::string_view name,
                  IGameDataParameterTableLoadRuntime& runtime) {
    const auto message =
        "ERROR [ palDBPrescriptionManager::verifyDataTable < the table " +
        std::string(name) + "  doesn't exist>]";
    runtime.LogError(message);
    throw std::runtime_error(message);
}

}  // namespace

std::uint8_t GameDataParameterTable::Preload(
    std::uint32_t manager, IGameDataParameterTableLoadRuntime& runtime) {
    return runtime.PreloadTable(runtime.ReadWord(manager), kTableName, 0x4e2ad0);
}

std::uint8_t GameDataParameterTable::Verify(
    std::uint32_t manager, IGameDataParameterTableLoadRuntime& runtime) {
    const auto database = runtime.ReadWord(manager);
    const auto table = runtime.FindTable(database, kTableName);
    runtime.WriteWord(manager + 4, table);
    if (!table) ThrowMissing(kTableName, runtime);
    const auto property = runtime.FindProperty(database, kPropertyName);
    runtime.WriteWord(manager + 8, property);
    if (!property) ThrowMissing(kPropertyName, runtime);
    return 1;
}

bool GameDataParameterTable::IsValidCategory(
    std::uint32_t record, IGameDataParameterTableRuntime& runtime) {
    return runtime.ReadWord(record + 100) <= 2u;
}

std::uint32_t GameDataParameterTable::Find(
    std::uint32_t table, std::int32_t key,
    IGameDataParameterTableRuntime& runtime) {
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
    const auto index = runtime.ReadWord(first + 4);
    return runtime.ReadWord(effect + 96) + 328 * index;
}

std::uint32_t GameDataParameterTable::FindOrLog(
    std::uint32_t table, std::int32_t key,
    IGameDataParameterTableRuntime& runtime) {
    const auto address = Find(table, key, runtime);
    if (!address) runtime.LogMissingRecord();
    return address;
}

bool GameDataParameterTable::Contains(
    std::uint32_t table, std::int32_t key,
    IGameDataParameterTableRuntime& runtime) {
    return Find(table, key, runtime) != 0;
}

std::uint8_t GameDataParameterTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IGameDataParameterRowRuntime& runtime) {
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

    PreparedRow record(runtime);
    if (!runtime.ParseBoundRow(table + 108, row, record.value) ||
        record.value[25] > 2u)
        throw std::runtime_error(
            InvalidRow(runtime.DumpRow(table, row)));

    const auto key = record.value[0];
    const auto key_begin = runtime.ReadWord(table + 8);
    const auto key_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(key_begin, key_end, key);
    if (position != key_end &&
        static_cast<std::int32_t>(key) >=
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        runtime.CopyRecord(runtime.ReadWord(table + 96) +
                               runtime.ReadWord(position + 4) * 328,
                           record.value);
    } else {
        const auto records = runtime.ReadWord(table + 96);
        const auto end = runtime.ReadWord(table + 100);
        const auto index = records ? (end - records) / 328 : 0;
        runtime.InsertRecord(table + 92, end, record.value);
        runtime.InsertKey(table + 4, position, key, index);
    }
    return 1;
}

void GameDataParameterTable::Append(
    std::uint32_t table, std::uint32_t row,
    IGameDataParameterAppendRuntime& runtime) {
    PreparedAppend record(runtime);
    if (runtime.RowSize(row) != runtime.CollectionSize(table + 124))
        throw std::runtime_error(
            InvalidRow(runtime.DumpRow(table, row)));
    for (std::uint32_t i = 0; i < runtime.CollectionSize(table + 124); ++i) {
        const auto fields = runtime.ReadWord(table + 128);
        const auto field = runtime.ReadWord(fields + i * 4);
        runtime.ConvertField(field, record.value, runtime.PropertyAt(row, i));
    }
    if (record.value[25] > 2u)
        throw std::runtime_error(
            InvalidRow(runtime.DumpRow(table, row)));
    runtime.AppendRecord(table + 92, runtime.ReadWord(table + 100),
                         record.value);
}

std::uint32_t GameDataParameterTable::MarkAndRefresh(
    std::uint32_t table, IGameDataParameterRefreshRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

void GameDataParameterTable::RefreshBinding(
    std::uint32_t table, IGameDataParameterBindingRuntime& runtime) {
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

}  // namespace pal4
