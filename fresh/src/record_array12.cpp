#include "pal4/record_array12.h"

#include <array>
#include <algorithm>
#include <limits>

namespace pal4 {
namespace {

using Record = std::array<std::uint32_t, 3>;

Record ReadRecord(const std::uint32_t address,
                  IRecordArray12Runtime& runtime) {
    return {runtime.ReadWord(address), runtime.ReadWord(address + 4u),
            runtime.ReadWord(address + 8u)};
}

void WriteRecord(const std::uint32_t address, const Record& record,
                 IRecordArray12Runtime& runtime) {
    for (auto i = 0u; i < record.size(); ++i)
        runtime.WriteWord(address + i * 4u, record[i]);
}

}  // namespace

bool RecordArray12::Insert(const std::uint32_t array,
                           const std::uint32_t position,
                           const std::uint32_t count,
                           const std::uint32_t value,
                           IRecordArray12Runtime& runtime) {
    if (count == 0u) return true;
    return InsertValue(array, position, count, ReadRecord(value, runtime),
                       runtime);
}

bool RecordArray12::InsertValue(const std::uint32_t array,
                                const std::uint32_t position,
                                const std::uint32_t count,
                                const std::array<std::uint32_t, 3>& record,
                                IRecordArray12Runtime& runtime) {
    if (count == 0u) return true;
    const auto begin = runtime.ReadWord(array + 4u);
    const auto end = runtime.ReadWord(array + 8u);
    const auto capacity_end = runtime.ReadWord(array + 12u);
    if (count > std::numeric_limits<std::uint32_t>::max() / kRecordSize ||
        (begin == 0u && (end != 0u || capacity_end != 0u || position != 0u)) ||
        (begin != 0u &&
         (position < begin || position > end || end > capacity_end ||
          (end - begin) % kRecordSize != 0u ||
          (position - begin) % kRecordSize != 0u ||
          (capacity_end - begin) % kRecordSize != 0u)))
        return false;

    const auto size = begin == 0u ? 0u : (end - begin) / kRecordSize;
    const auto index = begin == 0u ? 0u : (position - begin) / kRecordSize;
    const auto required_bytes = count * kRecordSize;
    if (begin != 0u && capacity_end - end >= required_bytes) {
        for (auto i = size; i > index; --i)
            WriteRecord(begin + (i - 1u + count) * kRecordSize,
                        ReadRecord(begin + (i - 1u) * kRecordSize, runtime),
                        runtime);
        for (auto i = 0u; i < count; ++i)
            WriteRecord(position + i * kRecordSize, record, runtime);
        runtime.WriteWord(array + 8u, end + required_bytes);
        return true;
    }

    const auto growth = std::max(size, count);
    if (size > std::numeric_limits<std::uint32_t>::max() / kRecordSize - growth)
        return false;
    const auto new_capacity = size + growth;
    const auto replacement = runtime.Allocate(new_capacity * kRecordSize);
    if (replacement == 0u) return false;
    for (auto i = 0u; i < index; ++i)
        WriteRecord(replacement + i * kRecordSize,
                    ReadRecord(begin + i * kRecordSize, runtime), runtime);
    for (auto i = 0u; i < count; ++i)
        WriteRecord(replacement + (index + i) * kRecordSize, record, runtime);
    for (auto i = index; i < size; ++i)
        WriteRecord(replacement + (i + count) * kRecordSize,
                    ReadRecord(begin + i * kRecordSize, runtime), runtime);
    runtime.Free(begin);
    runtime.WriteWord(array + 4u, replacement);
    runtime.WriteWord(array + 8u,
                      replacement + (size + count) * kRecordSize);
    runtime.WriteWord(array + 12u,
                      replacement + new_capacity * kRecordSize);
    return true;
}

}  // namespace pal4
