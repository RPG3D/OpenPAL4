#pragma once

#include "pal4/utilities_record_text_field.h"

#include <cstdint>

namespace pal4 {

class UtilitiesWorldRecordCopy final {
public:
    static constexpr std::uint32_t kRecordBytes = 40u;

    // IDA 0x50b960: assign one existing World record.
    static void AssignOne(std::uint32_t destination, std::uint32_t source,
                          IUtilitiesRecordTextRuntime& runtime);

    // IDA 0x50c540: compact adjacent equal World records.
    [[nodiscard]] static std::uint32_t SelectUnique(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        const UtilitiesRecordEquals& equals,
        IUtilitiesRecordTextRuntime& runtime);

    // IDA 0x50aba0: assign one World record throughout a range.
    [[nodiscard]] static std::uint32_t AssignRepeated(
        std::uint32_t begin, std::uint32_t end, std::uint32_t source,
        IUtilitiesRecordTextRuntime& runtime);

    // IDA 0x50ada0: assign World records from the end of a range.
    [[nodiscard]] static std::uint32_t AssignBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        IUtilitiesRecordTextRuntime& runtime);

    // IDA 0x50afc0: construct one World record.
    static void CopyConstruct(std::uint32_t destination,
                              std::uint32_t source,
                              IUtilitiesRecordTextRuntime& runtime);
};

}  // namespace pal4
