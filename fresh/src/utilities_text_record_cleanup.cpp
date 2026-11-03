#include "pal4/utilities_text_record_cleanup.h"

#include <initializer_list>

namespace pal4 {
namespace {

std::uint32_t ReleaseTextTriplet(
    std::uint32_t address, IUtilitiesTextRecordRuntime& runtime) {
    const auto text = runtime.ReadWord(address);
    std::uint32_t result = 0;
    if (text != 0) {
        const auto prefix = text - 1;
        const auto references = runtime.ReadByte(prefix);
        if (references == 0 || references == 0xff) {
            result = runtime.Free(prefix);
        } else {
            runtime.WriteByte(prefix,
                              static_cast<std::uint8_t>(references - 1));
            result = prefix;
        }
    }
    runtime.WriteWord(address, 0);
    runtime.WriteWord(address + 4, 0);
    runtime.WriteWord(address + 8, 0);
    return result;
}

std::uint32_t ReleaseTriplets(
    std::uint32_t record, IUtilitiesTextRecordRuntime& runtime,
    std::initializer_list<std::uint32_t> descending_offsets) {
    std::uint32_t result = 0;
    for (const auto offset : descending_offsets)
        result = ReleaseTextTriplet(record + offset, runtime);
    return result;
}

}  // namespace

std::uint32_t UtilitiesTextRecordCleanup::Release8And24(
    std::uint32_t record, IUtilitiesTextRecordRuntime& runtime) {
    return ReleaseTriplets(record, runtime, {24, 8});
}

std::uint32_t UtilitiesTextRecordCleanup::Release8And24And40(
    std::uint32_t record, IUtilitiesTextRecordRuntime& runtime) {
    return ReleaseTriplets(record, runtime, {40, 24, 8});
}

std::uint32_t UtilitiesTextRecordCleanup::Release12And28(
    std::uint32_t record, IUtilitiesTextRecordRuntime& runtime) {
    return ReleaseTriplets(record, runtime, {28, 12});
}

std::uint32_t UtilitiesTextRecordCleanup::Release8And28(
    std::uint32_t record, IUtilitiesTextRecordRuntime& runtime) {
    return ReleaseTriplets(record, runtime, {28, 8});
}

}  // namespace pal4
