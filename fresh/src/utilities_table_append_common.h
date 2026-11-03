#pragma once

#include <cstdint>
#include <stdexcept>

namespace pal4::detail {

template <class Record, class Runtime>
class TemporaryUtilitiesAppendRecord final {
public:
    explicit TemporaryUtilitiesAppendRecord(Runtime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~TemporaryUtilitiesAppendRecord() {
        if (active_)
            static_cast<void>(runtime_.CleanupTemporaryRecord(value));
    }
    [[nodiscard]] std::uint32_t Finish() {
        active_ = false;
        return runtime_.CleanupTemporaryRecord(value);
    }

    Record value{};

private:
    Runtime& runtime_;
    bool active_ = true;
};

template <class Record, class Runtime>
std::uint32_t AppendUtilitiesRecord(
    std::uint32_t object, std::uint32_t row,
    Runtime& runtime) {
    TemporaryUtilitiesAppendRecord<Record, Runtime> record(runtime);
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
    runtime.AppendRecord(object + 92, runtime.ReadWord(object + 100),
                         record.value);
    return record.Finish();
}

}  // namespace pal4::detail
