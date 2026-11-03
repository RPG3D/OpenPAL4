#include "pal4/utilities_introduction_table.h"
#include "utilities_effect_table_factory.h"
#include "utilities_table_index_common.h"
#include "utilities_table_append_common.h"

#include <stdexcept>

namespace pal4 {
namespace {

class TemporaryIntroductionRecord final {
public:
    explicit TemporaryIntroductionRecord(
        IUtilitiesIntroductionRowRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~TemporaryIntroductionRecord() {
        runtime_.DestroyTemporaryRecord(value);
    }

    UtilitiesIntroductionRecord value{};

private:
    IUtilitiesIntroductionRowRuntime& runtime_;
};

}  // namespace

std::uint32_t UtilitiesIntroductionTable::Create(
    std::uint32_t source,
    IUtilitiesIntroductionFactoryRuntime& runtime) {
    return detail::CreateUtilitiesEffectTable(source, 0x8447ec,
                                               runtime);
}

std::uint32_t UtilitiesIntroductionTable::Destroy(
    std::uint32_t object,
    IUtilitiesIntroductionLifetimeRuntime& runtime) {
    runtime.WriteWord(object, 0x8447ec);
    runtime.ClearEffect(object);
    const auto records_begin = runtime.ReadWord(object + 96);
    const auto records_end = runtime.ReadWord(object + 100);
    for (auto row = records_begin; row != records_end; row += 36) {
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

std::uint32_t UtilitiesIntroductionArray::Destroy(
    std::uint32_t array,
    IUtilitiesIntroductionArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    for (auto row = begin; row != end; row += 36)
        runtime.DestroyRecord(row);
    runtime.Free(begin);
    runtime.WriteWord(array + 4, 0);
    runtime.WriteWord(array + 8, 0);
    runtime.WriteWord(array + 12, 0);
    return 0;
}

std::uint32_t UtilitiesIntroductionTable::ClearRows(
    std::uint32_t object,
    IUtilitiesIntroductionClearRuntime& runtime) {
    auto result = runtime.ClearEffect(object);
    const auto begin = runtime.ReadWord(object + 96);
    const auto end = runtime.ReadWord(object + 100);
    for (auto row = begin; row != end; row += 36)
        result = runtime.DestroyRecord(row);
    runtime.WriteWord(object + 100, begin);
    return result;
}

std::uint8_t UtilitiesIntroductionTable::AddOrReplace(
    std::uint32_t object, std::uint32_t row,
    IUtilitiesIntroductionRowRuntime& runtime) {
    const auto fields = runtime.ReadWord(object + 24);
    const auto field_count = fields
                                 ? static_cast<std::int32_t>(
                                       runtime.ReadWord(object + 28) - fields) /
                                       60
                                 : 0;
    if (runtime.CollectionSize(object + 124) != field_count) {
        runtime.LogBind("beginBindField, table:", object + 52);
        runtime.AssignEffects(object + 108, object + 20);
        runtime.RegisterIntroductionFields(object + 108);
        runtime.BindEffects(object + 108);
        runtime.LogBind("endBindField, table:", object + 52);
    }

    TemporaryIntroductionRecord record(runtime);
    if (!runtime.ParseBoundRow(object + 108, row, record.value)) {
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(object, row));
    }

    const auto key = static_cast<std::int32_t>(record.value[0]);
    const auto index_begin = runtime.ReadWord(object + 8);
    const auto index_end = runtime.ReadWord(object + 12);
    const auto position = detail::FindSignedUtilitiesKey(
        index_begin, index_end, key, runtime);
    if (position == index_end ||
        key < static_cast<std::int32_t>(runtime.ReadWord(position))) {
        const auto begin = runtime.ReadWord(object + 96);
        const auto end = runtime.ReadWord(object + 100);
        const auto index = begin ? (end - begin) / 36 : 0;
        runtime.InsertRecord(object + 92, end, record.value);
        runtime.InsertKey(object + 4, position, record.value[0], index);
    } else {
        const auto index = runtime.ReadWord(position + 4);
        runtime.AssignRecord(runtime.ReadWord(object + 96) + 36 * index,
                             record.value);
    }
    return 1;
}

std::uint32_t UtilitiesIntroductionTable::Append(
    std::uint32_t object, std::uint32_t row,
    IUtilitiesIntroductionAppendRuntime& runtime) {
    return detail::AppendUtilitiesRecord<UtilitiesIntroductionRecord>(
        object, row, runtime);
}

std::uint32_t UtilitiesIntroductionTable::MarkAndRefresh(
    std::uint32_t object,
    IUtilitiesIntroductionRefreshRuntime& runtime) {
    runtime.WriteByte(object + 88, 1);
    return runtime.Refresh(object);
}

std::uint32_t UtilitiesIntroductionTable::Reserve(
    std::uint32_t object, std::int32_t requested,
    IUtilitiesIntroductionCapacityRuntime& runtime) {
    constexpr std::uint32_t kRecordBytes = 36;
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
