#pragma once

#include "pal4/utilities_record_text_field.h"

#include <cstdint>

namespace pal4 {

class IUtilitiesTextRecordCopyRuntime : public IUtilitiesRecordTextRuntime {
public:
    ~IUtilitiesTextRecordCopyRuntime() override = default;
};

class UtilitiesTextRecordCopy final {
public:
    static constexpr std::uint32_t kRecordBytes = 20u;

    // IDA 0x50b140: assign one existing TextRecord.
    static void AssignOne(std::uint32_t destination, std::uint32_t source,
                          IUtilitiesTextRecordCopyRuntime& runtime);

    // IDA 0x50bba0: compact non-equivalent adjacent records into destination.
    [[nodiscard]] static std::uint32_t SelectUnique(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        const UtilitiesRecordEquals& equals,
        IUtilitiesTextRecordCopyRuntime& runtime);

    // IDA 0x509660: assign the same record into a half-open destination range.
    [[nodiscard]] static std::uint32_t AssignRepeated(
        std::uint32_t begin, std::uint32_t end, std::uint32_t source,
        IUtilitiesTextRecordCopyRuntime& runtime);

    // IDA 0x5097c0: assign a range from its end toward destination_end.
    [[nodiscard]] static std::uint32_t AssignBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        IUtilitiesTextRecordCopyRuntime& runtime);

    // IDA 0x509950: construct one record, sharing text when the header permits.
    static void CopyConstruct(std::uint32_t destination,
                              std::uint32_t source,
                              IUtilitiesTextRecordCopyRuntime& runtime);
};

}  // namespace pal4
