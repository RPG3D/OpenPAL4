#include "pal4/utilities_record_text_field.h"

namespace pal4 {

StringBufferState UtilitiesRecordTextField::Read(
    const std::uint32_t field, IUtilitiesRecordTextRuntime& runtime) {
    return {runtime.ReadWord(field + 4u),
            runtime.ReadWord(field + 8u),
            runtime.ReadWord(field + 12u)};
}

void UtilitiesRecordTextField::Write(
    const std::uint32_t field, const StringBufferState& text,
    IUtilitiesRecordTextRuntime& runtime) {
    runtime.WriteWord(field + 4u, text.data);
    runtime.WriteWord(field + 8u, text.length);
    runtime.WriteWord(field + 12u, text.capacity);
}

void UtilitiesRecordTextField::Assign(
    const std::uint32_t destination, const std::uint32_t source,
    IUtilitiesRecordTextRuntime& runtime) {
    StringBuffer text(Read(destination, runtime));
    if (destination == source) {
        static_cast<void>(text.Assign(text, 0u, 0xffffffffu, runtime));
    } else {
        const StringBuffer source_text(Read(source, runtime));
        static_cast<void>(text.Assign(source_text, 0u, 0xffffffffu,
                                      runtime));
    }
    Write(destination, text.State(), runtime);
}

void UtilitiesRecordTextField::AssignInline(
    const std::uint32_t destination, const std::uint32_t source,
    IUtilitiesRecordTextRuntime& runtime,
    const bool finish_with_set_size) {
    StringBuffer text(Read(destination, runtime));
    const auto source_text = Read(source, runtime);
    if (destination == source) {
        static_cast<void>(text.Erase(source_text.length, 0xffffffffu,
                                     runtime));
        static_cast<void>(text.Erase(0u, 0u, runtime));
        Write(destination, text.State(), runtime);
        return;
    }

    auto state = text.State();
    const auto source_data = source_text.data != 0u
        ? source_text.data : StringBuffer::EmptyLogMessageAddress();
    if (source_text.length != 0u &&
        runtime.ReadByte(source_data - 1u) < 0xfeu) {
        runtime.SetErrorFlag(state, 1u);
        state = {source_data, source_text.length, source_text.capacity};
        runtime.WriteByte(source_data - 1u,
            static_cast<std::uint8_t>(
                runtime.ReadByte(source_data - 1u) + 1u));
    } else if (runtime.AllocateText(state, source_text.length, 1u)) {
        runtime.Copy(state.data, source_data, source_text.length);
        if (finish_with_set_size) {
            runtime.SetTextSize(state, source_text.length);
        } else {
            state.length = source_text.length;
            runtime.WriteByte(state.data + state.length, 0u);
        }
    }
    Write(destination, state, runtime);
}

}  // namespace pal4
