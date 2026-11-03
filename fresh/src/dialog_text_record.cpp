#include "pal4/dialog_text_record.h"

namespace pal4 {

std::uint32_t DialogTextRecord::InitializeAndCopy(
    std::uint32_t destination, std::uint32_t text_source,
    std::uint32_t fields_source, IUiTextBufferRecordRuntime& runtime) {
    UiTextBufferRecord::InitializeTextOnly(destination, text_source, runtime);
    for (std::uint32_t offset = 0u; offset < 12u; offset += 4u)
        runtime.WriteWord(destination + 16u + offset,
                          runtime.ReadWord(fields_source + offset));
    return destination;
}

void DialogTextRecord::CopyTextAndValue(
    std::uint32_t destination, std::uint32_t source,
    IUiTextBufferRecordRuntime& runtime) {
    UiTextBufferRecord::InitializeAndCopy(destination, source, runtime);
}

void DialogTextRecord::CopyWholeRecord(
    std::uint32_t destination, std::uint32_t source,
    IUiTextBufferRecordRuntime& runtime) {
    if (destination == 0u) return;
    (void)InitializeAndCopy(destination, source, source + 16u, runtime);
}

void DialogTextRecord::CopyTextAndValueAlternate(
    std::uint32_t destination, std::uint32_t source,
    IUiTextBufferRecordRuntime& runtime) {
    UiTextBufferRecord::InitializeAndCopy(destination, source, runtime);
}

}  // namespace pal4
