#include "pal4/utilities_scene_table.h"
#include "utilities_effect_table_factory.h"
#include "utilities_table_index_common.h"
#include "utilities_table_append_common.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pal4 {
namespace {

class TemporarySceneRecord final {
public:
    explicit TemporarySceneRecord(IUtilitiesSceneRowRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~TemporarySceneRecord() {
        runtime_.DestroyTemporaryRecord(value);
    }

    UtilitiesSceneRecord value{};

private:
    IUtilitiesSceneRowRuntime& runtime_;
};

}  // namespace

std::uint32_t UtilitiesSceneTable::Create(
    std::uint32_t source, IUtilitiesSceneFactoryRuntime& runtime) {
    return detail::CreateUtilitiesEffectTable(source, 0x84480c, runtime);
}

std::uint32_t UtilitiesSceneTable::Destroy(
    std::uint32_t object, IUtilitiesSceneLifetimeRuntime& runtime) {
    runtime.WriteWord(object, 0x84480c);
    runtime.ClearEffect(object);
    const auto records_begin = runtime.ReadWord(object + 96);
    const auto records_end = runtime.ReadWord(object + 100);
    for (auto row = records_begin; row != records_end; row += 60) {
        runtime.ReleaseText(row + 36);
        runtime.ReleaseText(row + 20);
        runtime.ReleaseText(row + 4);
    }
    runtime.WriteWord(object + 100, records_begin);
    runtime.ProcessElements(object + 108);

    runtime.Free(runtime.ReadWord(object + 128));
    for (const auto offset : {128u, 132u, 136u})
        runtime.WriteWord(object + offset, 0);

    const auto bindings_begin = runtime.ReadWord(object + 112);
    const auto bindings_end = runtime.ReadWord(object + 116);
    for (auto binding = bindings_begin; binding != bindings_end;
         binding += 60)
        runtime.DestroyBinding(binding, 0);
    runtime.Free(bindings_begin);
    for (const auto offset : {112u, 116u, 120u})
        runtime.WriteWord(object + offset, 0);

    runtime.Free(records_begin);
    for (const auto offset : {96u, 100u, 104u})
        runtime.WriteWord(object + offset, 0);
    return runtime.DestructEffect(object);
}

std::uint32_t UtilitiesSceneArray::Destroy(
    std::uint32_t array, IUtilitiesSceneArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    for (auto row = begin; row != end; row += 60)
        runtime.DestroyRecord(row);
    runtime.Free(begin);
    runtime.WriteWord(array + 4, 0);
    runtime.WriteWord(array + 8, 0);
    runtime.WriteWord(array + 12, 0);
    return 0;
}

std::uint32_t UtilitiesSceneTable::ClearRows(
    std::uint32_t object, IUtilitiesSceneClearRuntime& runtime) {
    auto result = runtime.ClearEffect(object);
    const auto begin = runtime.ReadWord(object + 96);
    const auto end = runtime.ReadWord(object + 100);
    for (auto row = begin; row != end; row += 60)
        result = runtime.DestroyRecord(row);
    runtime.WriteWord(object + 100, begin);
    return result;
}

std::uint8_t UtilitiesSceneTable::AddOrReplace(
    std::uint32_t object, std::uint32_t row,
    IUtilitiesSceneRowRuntime& runtime) {
    const auto fields = runtime.ReadWord(object + 24);
    const auto field_count = fields
                                 ? static_cast<std::int32_t>(
                                       runtime.ReadWord(object + 28) - fields) /
                                       60
                                 : 0;
    if (runtime.CollectionSize(object + 124) != field_count) {
        runtime.LogBind("beginBindField, table:", object + 52);
        runtime.AssignEffects(object + 108, object + 20);
        runtime.RegisterSceneFields(object + 108);
        runtime.BindEffects(object + 108);
        runtime.LogBind("endBindField, table:", object + 52);
    }

    TemporarySceneRecord record(runtime);
    if (!runtime.ParseBoundRow(object + 108, row, record.value)) {
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(object, row));
    }
    runtime.UppercaseText(record.value, 4);
    runtime.UppercaseText(record.value, 20);

    const auto key = static_cast<std::int32_t>(record.value[0]);
    const auto index_begin = runtime.ReadWord(object + 8);
    const auto index_end = runtime.ReadWord(object + 12);
    const auto position = detail::FindSignedUtilitiesKey(
        index_begin, index_end, key, runtime);
    if (position == index_end ||
        key < static_cast<std::int32_t>(runtime.ReadWord(position))) {
        const auto begin = runtime.ReadWord(object + 96);
        const auto end = runtime.ReadWord(object + 100);
        const auto index = begin ? (end - begin) / 60 : 0;
        runtime.InsertRecord(object + 92, end, record.value);
        runtime.InsertKey(object + 4, position, record.value[0], index);
    } else {
        const auto index = runtime.ReadWord(position + 4);
        runtime.AssignRecord(runtime.ReadWord(object + 96) + 60 * index,
                             record.value);
    }
    return 1;
}

std::uint32_t UtilitiesSceneTable::Append(
    std::uint32_t object, std::uint32_t row,
    IUtilitiesSceneAppendRuntime& runtime) {
    detail::TemporaryUtilitiesAppendRecord<UtilitiesSceneRecord,
                                           IUtilitiesSceneAppendRuntime>
        record(runtime);
    const auto collection = object + 124;
    const auto row_size = runtime.RowSize(row);
    if (row_size != runtime.CollectionSize(collection)) {
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(object, row));
    }
    for (std::uint32_t i = 0; i < runtime.CollectionSize(collection); ++i) {
        const auto bindings = runtime.ReadWord(object + 128);
        const auto binding = runtime.ReadWord(bindings + 4 * i);
        runtime.ConvertField(binding, record.value, runtime.PropertyAt(row, i));
    }
    runtime.UppercaseText(record.value, 4);
    runtime.UppercaseText(record.value, 20);
    runtime.AppendRecord(object + 92, runtime.ReadWord(object + 100),
                         record.value);
    return record.Finish();
}

std::uint32_t UtilitiesSceneTable::MarkAndRefresh(
    std::uint32_t object,
    IUtilitiesSceneRefreshRuntime& runtime) {
    runtime.WriteByte(object + 88, 1);
    return runtime.Refresh(object);
}

void UtilitiesSceneTable::DeduplicateAndRebuildIndex(
    std::uint32_t object,
    IUtilitiesSceneIndexRuntime& runtime) {
    constexpr std::uint32_t kRecordBytes = 60;
    constexpr std::uint32_t kIndexBytes = 8;
    const auto begin = runtime.ReadWord(object + 96);
    const auto old_end = runtime.ReadWord(object + 100);
    auto duplicate = old_end;
    if (begin != old_end) {
        auto previous = begin;
        for (auto source = begin + kRecordBytes; source != old_end;
             source += kRecordBytes) {
            if (runtime.ReadWord(previous) == runtime.ReadWord(source)) {
                duplicate = previous;
                break;
            }
            previous = source;
        }
    }

    auto new_end = old_end;
    if (duplicate != old_end) {
        runtime.CopyRecord(duplicate, duplicate);
        new_end = duplicate + kRecordBytes;
        auto previous = duplicate;
        for (auto source = duplicate + kRecordBytes; source != old_end;
             source += kRecordBytes) {
            if (runtime.ReadWord(previous) != runtime.ReadWord(source)) {
                runtime.CopyRecord(new_end, source);
                new_end += kRecordBytes;
            }
            previous = source;
        }
    }
    static_cast<void>(UtilitiesSceneArray::DestroyRange(new_end, old_end,
                                                        runtime));
    runtime.WriteWord(object + 100, new_end);

    const auto index_begin = runtime.ReadWord(object + 8);
    runtime.WriteWord(object + 12, index_begin);
    if (begin != 0) {
        for (auto row = begin; row != new_end; row += kRecordBytes) {
            const auto physical_index = (row - begin) / kRecordBytes;
            runtime.InsertKey(object + 4, runtime.ReadWord(object + 12),
                              runtime.ReadWord(row), physical_index);
        }
    }

    const auto sorted_begin = runtime.ReadWord(object + 8);
    const auto sorted_end = runtime.ReadWord(object + 12);
    std::vector<std::pair<std::uint32_t, std::uint32_t>> entries;
    for (auto at = sorted_begin; at != sorted_end; at += kIndexBytes)
        entries.emplace_back(runtime.ReadWord(at), runtime.ReadWord(at + 4));
    std::sort(entries.begin(), entries.end(), [](const auto& left,
                                                  const auto& right) {
        return static_cast<std::int32_t>(left.first) <
               static_cast<std::int32_t>(right.first);
    });
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto at = sorted_begin +
                        static_cast<std::uint32_t>(i) * kIndexBytes;
        runtime.WriteWord(at, entries[i].first);
        runtime.WriteWord(at + 4, entries[i].second);
    }
}

std::uint32_t UtilitiesSceneArray::DestroyRange(
    std::uint32_t begin, std::uint32_t end,
    IUtilitiesSceneIndexRuntime& runtime) {
    std::uint32_t result = 0;
    for (auto row = begin; row != end; row += 60)
        result = runtime.DestroyRecord(row);
    return result;
}

std::uint32_t UtilitiesSceneTable::Reserve(
    std::uint32_t object, std::int32_t requested,
    IUtilitiesSceneCapacityRuntime& runtime) {
    constexpr std::uint32_t kRecordBytes = 60;
    constexpr std::uint32_t kIndexBytes = 8;
    const auto desired = static_cast<std::uint32_t>(requested);
    const auto old_records = runtime.ReadWord(object + 96);
    const auto record_capacity = old_records
                                     ? (runtime.ReadWord(object + 104) -
                                        old_records) / kRecordBytes
                                     : 0;
    if (record_capacity < desired) {
        const auto bytes = static_cast<std::uint32_t>(
            requested < 0 ? 0 : requested) * kRecordBytes;
        const auto replacement = runtime.Allocate(bytes);
        const auto old_end = runtime.ReadWord(object + 100);
        const auto old_begin = runtime.ReadWord(object + 96);
        for (auto source = old_begin, destination = replacement;
             source != old_end;
             source += kRecordBytes, destination += kRecordBytes)
            runtime.CopyRecord(destination, source);
        for (auto source = old_begin; source != old_end;
             source += kRecordBytes)
            runtime.DestroyRecord(source);
        runtime.Free(old_begin);

        const auto count = old_begin
                               ? (old_end - old_begin) / kRecordBytes
                               : 0;
        runtime.WriteWord(object + 104,
                          replacement + desired * kRecordBytes);
        runtime.WriteWord(object + 96, replacement);
        runtime.WriteWord(object + 100,
                          replacement + count * kRecordBytes);
    }

    const auto old_keys = runtime.ReadWord(object + 8);
    const auto key_capacity = old_keys
                                  ? static_cast<std::uint32_t>(
                                        static_cast<std::int32_t>(
                                            runtime.ReadWord(object + 16) -
                                            old_keys) >> 3)
                                  : 0;
    if (key_capacity >= desired) return key_capacity;

    const auto bytes = static_cast<std::uint32_t>(
        requested < 0 ? 0 : requested) * kIndexBytes;
    const auto replacement = runtime.Allocate(bytes);
    const auto old_end = runtime.ReadWord(object + 12);
    const auto old_begin = runtime.ReadWord(object + 8);
    for (auto source = old_begin, destination = replacement;
         source != old_end;
         source += kIndexBytes, destination += kIndexBytes) {
        if (destination != 0) {
            runtime.WriteWord(destination, runtime.ReadWord(source));
            runtime.WriteWord(destination + 4,
                              runtime.ReadWord(source + 4));
        }
    }
    runtime.Free(old_begin);
    runtime.WriteWord(object + 16,
                      replacement + desired * kIndexBytes);
    const auto count = old_begin
                           ? static_cast<std::uint32_t>(
                                 static_cast<std::int32_t>(old_end -
                                                           old_begin) >> 3)
                           : 0;
    runtime.WriteWord(object + 8, replacement);
    const auto new_end = replacement + count * kIndexBytes;
    runtime.WriteWord(object + 12, new_end);
    return new_end;
}

}  // namespace pal4
