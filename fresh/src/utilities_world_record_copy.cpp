#include "pal4/utilities_world_record_copy.h"

namespace pal4 {

namespace {

void AssignSelected(const std::uint32_t destination,
                    const std::uint32_t source,
                    IUtilitiesRecordTextRuntime& runtime) {
    runtime.WriteWord(destination, runtime.ReadWord(source));
    UtilitiesRecordTextField::Assign(destination + 4u, source + 4u, runtime);
    runtime.WriteWord(destination + 20u, runtime.ReadWord(source + 20u));
    UtilitiesRecordTextField::AssignInline(destination + 24u,
                                           source + 24u, runtime);
}

}  // namespace
void UtilitiesWorldRecordCopy::AssignOne(
    const std::uint32_t destination, const std::uint32_t source,
    IUtilitiesRecordTextRuntime& runtime) {
    runtime.WriteWord(destination, runtime.ReadWord(source));
    UtilitiesRecordTextField::AssignInline(destination + 4u,
                                           source + 4u, runtime, true);
    runtime.WriteWord(destination + 20u,
                      runtime.ReadWord(source + 20u));
    UtilitiesRecordTextField::AssignInline(destination + 24u,
                                           source + 24u, runtime, true);
}

std::uint32_t UtilitiesWorldRecordCopy::SelectUnique(
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

std::uint32_t UtilitiesWorldRecordCopy::AssignRepeated(
    std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t source, IUtilitiesRecordTextRuntime& runtime) {
    while (begin != end) {
        AssignOne(begin, source, runtime);
        begin += kRecordBytes;
    }
    return begin;
}

std::uint32_t UtilitiesWorldRecordCopy::AssignBackward(
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

void UtilitiesWorldRecordCopy::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    IUtilitiesRecordTextRuntime& runtime) {
    if (destination == 0u) return;
    runtime.WriteWord(destination, runtime.ReadWord(source));

    runtime.WriteByte(destination + 4u,
                      runtime.ReadByte(source + 4u));
    UtilitiesRecordTextField::Write(destination + 4u, {}, runtime);
    UtilitiesRecordTextField::Assign(destination + 4u,
                                     source + 4u, runtime);

    runtime.WriteWord(destination + 20u,
                      runtime.ReadWord(source + 20u));
    runtime.WriteByte(destination + 24u,
                      runtime.ReadByte(source + 24u));
    UtilitiesRecordTextField::Write(destination + 24u, {}, runtime);
    UtilitiesRecordTextField::AssignInline(destination + 24u,
                                           source + 24u, runtime);
}

}  // namespace pal4
