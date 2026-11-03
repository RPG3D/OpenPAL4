#include "pal4/utilities_text_record_copy.h"

namespace pal4 {
namespace {

StringBufferState ReadText(const std::uint32_t record,
                           IUtilitiesTextRecordCopyRuntime& runtime) {
    return {runtime.ReadWord(record + 8u),
            runtime.ReadWord(record + 12u),
            runtime.ReadWord(record + 16u)};
}

void WriteText(const std::uint32_t record, const StringBufferState& text,
               IUtilitiesTextRecordCopyRuntime& runtime) {
    runtime.WriteWord(record + 8u, text.data);
    runtime.WriteWord(record + 12u, text.length);
    runtime.WriteWord(record + 16u, text.capacity);
}

void AssignSelected(const std::uint32_t destination,
                    const std::uint32_t source,
                    IUtilitiesTextRecordCopyRuntime& runtime,
                    const bool first) {
    runtime.WriteWord(destination, runtime.ReadWord(source));
    if (first) {
        UtilitiesRecordTextField::AssignInline(destination + 4u,
                                               source + 4u, runtime);
        return;
    }
    const auto destination_field = destination + 4u;
    const auto source_field = source + 4u;
    if (destination_field == source_field) {
        UtilitiesRecordTextField::AssignInline(destination_field,
                                               source_field, runtime, true);
        return;
    }

    auto state = UtilitiesRecordTextField::Read(destination_field, runtime);
    const auto source_text = UtilitiesRecordTextField::Read(source_field,
                                                            runtime);
    const auto data = source_text.data != 0u ? source_text.data
        : StringBuffer::EmptyLogMessageAddress();
    if (source_text.length != 0u && runtime.ReadByte(data - 1u) < 0xfeu) {
        if (state.data != 0u) {
            const auto reference = runtime.ReadByte(state.data - 1u);
            if (reference == 0u || reference == 0xffu)
                runtime.Free(state.data - 1u);
            else
                runtime.WriteByte(state.data - 1u,
                    static_cast<std::uint8_t>(reference - 1u));
        }
        state = {data, source_text.length, source_text.capacity};
        runtime.WriteByte(data - 1u,
            static_cast<std::uint8_t>(runtime.ReadByte(data - 1u) + 1u));
    } else if (runtime.AllocateText(state, source_text.length, 1u)) {
        runtime.Copy(state.data, data, source_text.length);
        runtime.SetTextSize(state, source_text.length);
    }
    UtilitiesRecordTextField::Write(destination_field, state, runtime);
}

}  // namespace

void UtilitiesTextRecordCopy::AssignOne(
    const std::uint32_t destination, const std::uint32_t source,
    IUtilitiesTextRecordCopyRuntime& runtime) {
    runtime.WriteWord(destination, runtime.ReadWord(source));
    StringBuffer text(ReadText(destination, runtime));
    if (destination == source) {
        static_cast<void>(text.Assign(text, 0u, 0xffffffffu, runtime));
    } else {
        const auto source_text = ReadText(source, runtime);
        const auto source_data = source_text.data != 0u
            ? source_text.data : StringBuffer::EmptyLogMessageAddress();
        auto state = text.State();
        if (source_text.length != 0u &&
            runtime.ReadByte(source_data - 1u) < 0xfeu) {
            if (state.data != 0u) {
                const auto reference = runtime.ReadByte(state.data - 1u);
                if (reference == 0u || reference == 0xffu)
                    runtime.Free(state.data - 1u);
                else
                    runtime.WriteByte(state.data - 1u,
                        static_cast<std::uint8_t>(reference - 1u));
            }
            state = {source_data, source_text.length,
                     source_text.capacity};
            runtime.WriteByte(source_data - 1u,
                static_cast<std::uint8_t>(
                    runtime.ReadByte(source_data - 1u) + 1u));
        } else if (runtime.AllocateText(state, source_text.length, 1u)) {
            runtime.Copy(state.data, source_data, source_text.length);
            state.length = source_text.length;
            runtime.WriteByte(state.data + state.length, 0u);
        }
        WriteText(destination, state, runtime);
        return;
    }
    WriteText(destination, text.State(), runtime);
}

std::uint32_t UtilitiesTextRecordCopy::SelectUnique(
    const std::uint32_t begin, const std::uint32_t end,
    std::uint32_t destination, const UtilitiesRecordEquals& equals,
    IUtilitiesTextRecordCopyRuntime& runtime) {
    if (begin == end) return destination;
    AssignSelected(destination, begin, runtime, true);
    destination += kRecordBytes;
    auto previous = begin;
    for (auto candidate = begin + kRecordBytes; candidate != end;
         candidate += kRecordBytes) {
        if (!equals(previous, candidate)) {
            AssignSelected(destination, candidate, runtime, false);
            destination += kRecordBytes;
            previous = candidate;
        }
    }
    return destination;
}

std::uint32_t UtilitiesTextRecordCopy::AssignRepeated(
    std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t source, IUtilitiesTextRecordCopyRuntime& runtime) {
    while (begin != end) {
        AssignOne(begin, source, runtime);
        begin += kRecordBytes;
    }
    return begin;
}

std::uint32_t UtilitiesTextRecordCopy::AssignBackward(
    const std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination_end,
    IUtilitiesTextRecordCopyRuntime& runtime) {
    while (end != begin) {
        end -= kRecordBytes;
        destination_end -= kRecordBytes;
        AssignOne(destination_end, end, runtime);
    }
    return destination_end;
}

void UtilitiesTextRecordCopy::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    IUtilitiesTextRecordCopyRuntime& runtime) {
    if (destination == 0u) return;

    runtime.WriteWord(destination, runtime.ReadWord(source));
    runtime.WriteByte(destination + 4u, runtime.ReadByte(source + 4u));
    StringBufferState text{};
    WriteText(destination, text, runtime);
    const auto source_text = ReadText(source, runtime);

    if (destination == source) {
        runtime.DetachAndReset(text);
        runtime.DetachAndReset(text);
    } else {
        const auto source_data = source_text.data != 0u
            ? source_text.data : StringBuffer::EmptyLogMessageAddress();
        if (source_text.length != 0u &&
            runtime.ReadByte(source_data - 1u) < 0xfeu) {
            runtime.SetErrorFlag(text, 1u);
            text = {source_data, source_text.length,
                    source_text.capacity};
            runtime.WriteByte(source_data - 1u,
                static_cast<std::uint8_t>(
                    runtime.ReadByte(source_data - 1u) + 1u));
        } else if (runtime.AllocateText(text, source_text.length, 1u)) {
            runtime.Copy(text.data, source_data, source_text.length);
            text.length = source_text.length;
            runtime.WriteByte(text.data + text.length, 0u);
        }
    }
    WriteText(destination, text, runtime);
}

}  // namespace pal4
