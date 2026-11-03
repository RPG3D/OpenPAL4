#pragma once

#include "pal4/utilities_record_text_field.h"

#include <cstdint>

namespace pal4 {

class UtilitiesPreviewRecordCopy final {
public:
    static constexpr std::uint32_t kRecordBytes = 52u;

    // IDA 0x50b290: assign one existing PreviewInfo record.
    static void AssignOne(std::uint32_t destination, std::uint32_t source,
                          IUtilitiesRecordTextRuntime& runtime);

    // IDA 0x50bdd0: compact non-equivalent adjacent PreviewInfo records.
    [[nodiscard]] static std::uint32_t SelectUnique(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        const UtilitiesRecordEquals& equals,
        IUtilitiesRecordTextRuntime& runtime);

    // IDA 0x509ac0: assign one PreviewInfo record repeatedly.
    [[nodiscard]] static std::uint32_t AssignRepeated(
        std::uint32_t begin, std::uint32_t end, std::uint32_t source,
        IUtilitiesRecordTextRuntime& runtime);

    // IDA 0x509cd0: assign a PreviewInfo range from its end.
    [[nodiscard]] static std::uint32_t AssignBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        IUtilitiesRecordTextRuntime& runtime);

    // IDA 0x509f00: construct one PreviewInfo record.
    static void CopyConstruct(std::uint32_t destination,
                              std::uint32_t source,
                              IUtilitiesRecordTextRuntime& runtime);
};

}  // namespace pal4
