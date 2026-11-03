#include "pal4/utilities_window_record_copy.h"

namespace pal4 {

namespace {

void AssignSelected(const std::uint32_t destination,
                    const std::uint32_t source,
                    IUtilitiesRecordTextRuntime& runtime) {
    runtime.WriteWord(destination, runtime.ReadWord(source));
    runtime.WriteWord(destination + 4u, runtime.ReadWord(source + 4u));
    UtilitiesRecordTextField::Assign(destination + 8u, source + 8u, runtime);
    UtilitiesRecordTextField::AssignInline(destination + 24u,
                                           source + 24u, runtime);
    runtime.WriteWord(destination + 40u, runtime.ReadWord(source + 40u));
}

}  // namespace
void UtilitiesWindowRecordCopy::AssignOne(
    const std::uint32_t destination, const std::uint32_t source,
    IUtilitiesRecordTextRuntime& runtime) {
    runtime.WriteWord(destination, runtime.ReadWord(source));
    runtime.WriteWord(destination + 4u, runtime.ReadWord(source + 4u));
    UtilitiesRecordTextField::AssignInline(destination + 8u,
                                           source + 8u, runtime);
    UtilitiesRecordTextField::AssignInline(destination + 24u,
                                           source + 24u, runtime);
    runtime.WriteWord(destination + 40u,
                      runtime.ReadWord(source + 40u));
}

std::uint32_t UtilitiesWindowRecordCopy::SelectUnique(
    const std::uint32_t begin, const std::uint32_t end,
    std::uint32_t destination, const UtilitiesRecordEquals& equals,
    IUtilitiesRecordTextRuntime& runtime) {
    if (begin == end) return destination;
    AssignSelected(destination, begin, runtime);
    destination += kRecordBytes;
    auto previous = begin;
    for (auto candidate = begin + kRecordBytes; candidate != end;
         candidate += kRecordBytes) {
        if (!equals(previous, candidate)) {
            AssignSelected(destination, candidate, runtime);
            destination += kRecordBytes;
            previous = candidate;
        }
    }
    return destination;
}

std::uint32_t UtilitiesWindowRecordCopy::AssignRepeated(
    std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t source, IUtilitiesRecordTextRuntime& runtime) {
    while (begin != end) {
        AssignOne(begin, source, runtime);
        begin += kRecordBytes;
    }
    return begin;
}

std::uint32_t UtilitiesWindowRecordCopy::AssignBackward(
    const std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination_end,
    IUtilitiesRecordTextRuntime& runtime) {
    while (end != begin) {
        end -= kRecordBytes;
        destination_end -= kRecordBytes;
        AssignOne(destination_end, end, runtime);
    }
    return destination_end;
}

void UtilitiesWindowRecordCopy::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    IUtilitiesRecordTextRuntime& runtime) {
    if (destination == 0u) return;
    runtime.WriteWord(destination, runtime.ReadWord(source));
    runtime.WriteWord(destination + 4u, runtime.ReadWord(source + 4u));

    runtime.WriteByte(destination + 8u,
                      runtime.ReadByte(source + 8u));
    UtilitiesRecordTextField::Write(destination + 8u, {}, runtime);
    UtilitiesRecordTextField::Assign(destination + 8u,
                                     source + 8u, runtime);

    runtime.WriteByte(destination + 24u,
                      runtime.ReadByte(source + 24u));
    UtilitiesRecordTextField::Write(destination + 24u, {}, runtime);
    UtilitiesRecordTextField::AssignInline(destination + 24u,
                                           source + 24u, runtime);

    runtime.WriteWord(destination + 40u,
                      runtime.ReadWord(source + 40u));
}

}  // namespace pal4
