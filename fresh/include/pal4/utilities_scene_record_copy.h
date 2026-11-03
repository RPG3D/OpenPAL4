#pragma once

#include "pal4/utilities_record_text_field.h"

#include <cstdint>

namespace pal4 {

class UtilitiesSceneRecordCopy final {
public:
    static constexpr std::uint32_t kRecordBytes = 60u;

    // IDA 0x50b4e0: assign one existing Scene record.
    static void AssignOne(std::uint32_t destination, std::uint32_t source,
                          IUtilitiesRecordTextRuntime& runtime);

    // IDA 0x50c010: compact non-equivalent adjacent Scene records.
    [[nodiscard]] static std::uint32_t SelectUnique(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        const UtilitiesRecordEquals& equals,
        IUtilitiesRecordTextRuntime& runtime);

    // IDA 0x50a100: assign one Scene record throughout a range.
    [[nodiscard]] static std::uint32_t AssignRepeated(
        std::uint32_t begin, std::uint32_t end, std::uint32_t source,
        IUtilitiesRecordTextRuntime& runtime);

    // IDA 0x50a320: assign a Scene range from its end.
    [[nodiscard]] static std::uint32_t AssignBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        IUtilitiesRecordTextRuntime& runtime);

    // IDA 0x50a560: construct one Scene record.
    static void CopyConstruct(std::uint32_t destination,
                              std::uint32_t source,
                              IUtilitiesRecordTextRuntime& runtime);
};

}  // namespace pal4
