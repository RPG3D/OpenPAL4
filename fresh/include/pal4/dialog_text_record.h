#pragma once

#include "pal4/ui_text_buffer_record.h"

#include <cstdint>

namespace pal4 {

class DialogTextRecord final {
public:
    // IDA 0x62c6a0: initialize 16 text bytes and copy three trailing words.
    [[nodiscard]] static std::uint32_t InitializeAndCopy(
        std::uint32_t destination, std::uint32_t text_source,
        std::uint32_t fields_source, IUiTextBufferRecordRuntime& runtime);
    // IDA 0x62c7d0: copy a nullable 20-byte text/value record.
    static void CopyTextAndValue(
        std::uint32_t destination, std::uint32_t source,
        IUiTextBufferRecordRuntime& runtime);
    // IDA 0x62c930: copy a nullable 28-byte text/three-value record.
    static void CopyWholeRecord(
        std::uint32_t destination, std::uint32_t source,
        IUiTextBufferRecordRuntime& runtime);
    // IDA 0x62caa0: second instance of the 20-byte copy entry.
    static void CopyTextAndValueAlternate(
        std::uint32_t destination, std::uint32_t source,
        IUiTextBufferRecordRuntime& runtime);
};

}  // namespace pal4
