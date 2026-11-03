#include "pal4/utilities_string_id_table.h"
#include "utilities_effect_table_factory.h"
#include "utilities_table_index_common.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pal4 {
namespace {

std::uint32_t ReservePairArray(
    std::uint32_t object, std::uint32_t begin_offset,
    std::int32_t requested,
    IUtilitiesStringIdCapacityRuntime& runtime) {
    constexpr std::uint32_t kPairBytes = 8;
    const auto desired = static_cast<std::uint32_t>(requested);
    const auto old_begin = runtime.ReadWord(object + begin_offset);
    const auto capacity = old_begin
                              ? (runtime.ReadWord(object + begin_offset + 8) -
                                 old_begin) / kPairBytes
                              : 0;
    if (capacity >= desired) return capacity;

    const auto bytes = static_cast<std::uint32_t>(
        requested < 0 ? 0 : requested) * kPairBytes;
    const auto replacement = runtime.Allocate(bytes);
    const auto old_end = runtime.ReadWord(object + begin_offset + 4);
    for (auto source = old_begin, destination = replacement;
         source != old_end;
         source += kPairBytes, destination += kPairBytes) {
        if (destination != 0) {
            runtime.WriteWord(destination, runtime.ReadWord(source));
            runtime.WriteWord(destination + 4,
                              runtime.ReadWord(source + 4));
        }
    }
    runtime.Free(old_begin);
    runtime.WriteWord(object + begin_offset + 8,
                      replacement + desired * kPairBytes);
    const auto count = old_begin ? (old_end - old_begin) / kPairBytes : 0;
    runtime.WriteWord(object + begin_offset, replacement);
    runtime.WriteWord(object + begin_offset + 4,
                      replacement + count * kPairBytes);
    return count;
}

}  // namespace

std::uint32_t UtilitiesStringIdTable::Create(
    std::uint32_t source, IUtilitiesStringIdFactoryRuntime& runtime) {
    return detail::CreateUtilitiesEffectTable(source, 0x84484c, runtime);
}

std::uint32_t UtilitiesStringIdTable::Destroy(
    std::uint32_t object, IUtilitiesStringIdLifetimeRuntime& runtime) {
    runtime.WriteWord(object, 0x84484c);
    runtime.ClearEffect(object);
    const auto records_begin = runtime.ReadWord(object + 96);
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

std::uint32_t UtilitiesStringIdTable::ClearRows(
    std::uint32_t object, IUtilitiesStringIdClearRuntime& runtime) {
    runtime.ClearEffect(object);
    const auto end = runtime.ReadWord(object + 100);
    runtime.WriteWord(object + 100, runtime.ReadWord(object + 96));
    return end;
}

std::uint8_t UtilitiesStringIdTable::AddOrReplace(
    std::uint32_t object, std::uint32_t row,
    IUtilitiesStringIdRowRuntime& runtime) {
    const auto fields = runtime.ReadWord(object + 24);
    const auto field_count = fields
                                 ? static_cast<std::int32_t>(
                                       runtime.ReadWord(object + 28) - fields) /
                                       60
                                 : 0;
    if (runtime.CollectionSize(object + 124) != field_count) {
        runtime.LogBind("beginBindField, table:", object + 52);
        runtime.AssignEffects(object + 108, object + 20);
        runtime.RegisterStringIdFields(object + 108);
        runtime.BindEffects(object + 108);
        runtime.LogBind("endBindField, table:", object + 52);
    }

    const auto row_size = runtime.RowSize(row);
    if (row_size != static_cast<std::uint32_t>(
                        runtime.CollectionSize(object + 124))) {
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(object, row));
    }
    UtilitiesStringIdRecord record{};
    for (std::uint32_t i = 0;
         i < static_cast<std::uint32_t>(
                 runtime.CollectionSize(object + 124));
         ++i) {
        const auto bindings = runtime.ReadWord(object + 128);
        const auto binding = runtime.ReadWord(bindings + 4 * i);
        runtime.ConvertField(binding, record,
                             runtime.PropertyAt(row, i));
    }

    const auto key = static_cast<std::int32_t>(record[0]);
    const auto begin = runtime.ReadWord(object + 96);
    const auto end = runtime.ReadWord(object + 100);
    const auto index = begin ? (end - begin) / 8 : 0;
    const auto index_begin = runtime.ReadWord(object + 8);
    const auto index_end = runtime.ReadWord(object + 12);
    const auto position = detail::FindSignedUtilitiesKey(
        index_begin, index_end, key, runtime);
    if (position == index_end ||
        key < static_cast<std::int32_t>(runtime.ReadWord(position))) {
        runtime.InsertRecord(object + 92, end, record);
        runtime.InsertKey(object + 4, position, record[0], index);
    } else {
        const auto physical_index = runtime.ReadWord(position + 4);
        const auto destination = begin + 8 * physical_index;
        runtime.WriteWord(destination, record[0]);
        runtime.WriteWord(destination + 4, record[1]);
    }
    return 1;
}

std::uint32_t UtilitiesStringIdTable::Append(
    std::uint32_t object, std::uint32_t row,
    IUtilitiesStringIdAppendRuntime& runtime) {
    const auto row_size = runtime.RowSize(row);
    if (static_cast<std::int32_t>(row_size) !=
        runtime.CollectionSize(object + 124)) {
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(object, row));
    }
    UtilitiesStringIdRecord record{};
    for (std::uint32_t i = 0;
         i < static_cast<std::uint32_t>(
                 runtime.CollectionSize(object + 124)); ++i) {
        const auto bindings = runtime.ReadWord(object + 128);
        const auto binding = runtime.ReadWord(bindings + 4 * i);
        runtime.ConvertField(binding, record,
                             runtime.PropertyAt(row, i));
    }
    return runtime.AppendRecord(object + 92,
                                runtime.ReadWord(object + 100), record);
}

std::uint32_t UtilitiesStringIdTable::MarkAndRefresh(
    std::uint32_t object,
    IUtilitiesStringIdRefreshRuntime& runtime) {
    runtime.WriteByte(object + 88, 1);
    return runtime.Refresh(object);
}

void UtilitiesStringIdTable::DeduplicateAndRebuildIndex(
    std::uint32_t object,
    IUtilitiesStringIdIndexRuntime& runtime) {
    constexpr std::uint32_t kPairBytes = 8;
    const auto begin = runtime.ReadWord(object + 96);
    const auto old_end = runtime.ReadWord(object + 100);
    auto duplicate = old_end;
    if (begin != old_end) {
        auto previous = begin;
        for (auto source = begin + kPairBytes; source != old_end;
             source += kPairBytes) {
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
        new_end = duplicate + kPairBytes;
        auto previous = duplicate;
        for (auto source = duplicate + kPairBytes; source != old_end;
             source += kPairBytes) {
            if (runtime.ReadWord(previous) != runtime.ReadWord(source)) {
                runtime.CopyRecord(new_end, source);
                new_end += kPairBytes;
            }
            previous = source;
        }
    }
    runtime.WriteWord(object + 100, new_end);

    const auto index_begin = runtime.ReadWord(object + 8);
    runtime.WriteWord(object + 12, index_begin);
    if (begin != 0) {
        for (auto record = begin; record != new_end;
             record += kPairBytes) {
            const auto physical_index = (record - begin) / kPairBytes;
            runtime.InsertKey(object + 4, runtime.ReadWord(object + 12),
                              runtime.ReadWord(record), physical_index);
        }
    }

    const auto sorted_begin = runtime.ReadWord(object + 8);
    const auto sorted_end = runtime.ReadWord(object + 12);
    std::vector<std::pair<std::uint32_t, std::uint32_t>> entries;
    for (auto at = sorted_begin; at != sorted_end; at += kPairBytes)
        entries.emplace_back(runtime.ReadWord(at), runtime.ReadWord(at + 4));
    std::sort(entries.begin(), entries.end(), [](const auto& left,
                                                  const auto& right) {
        return static_cast<std::int32_t>(left.first) <
               static_cast<std::int32_t>(right.first);
    });
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto at = sorted_begin +
                        static_cast<std::uint32_t>(i) * kPairBytes;
        runtime.WriteWord(at, entries[i].first);
        runtime.WriteWord(at + 4, entries[i].second);
    }
}

std::uint32_t UtilitiesStringIdTable::Reserve(
    std::uint32_t object, std::int32_t requested,
    IUtilitiesStringIdCapacityRuntime& runtime) {
    static_cast<void>(ReservePairArray(object, 96, requested, runtime));
    return ReservePairArray(object, 8, requested, runtime);
}

}  // namespace pal4
