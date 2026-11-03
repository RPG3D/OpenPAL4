#include "pal4/utilities_preview_record_copy.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kFields[] = {4u, 20u, 36u};

void AssignSelected(const std::uint32_t destination,
                    const std::uint32_t source,
                    IUtilitiesRecordTextRuntime& runtime) {
    runtime.WriteWord(destination, runtime.ReadWord(source));
    UtilitiesRecordTextField::Assign(destination + 4u, source + 4u, runtime);
    UtilitiesRecordTextField::Assign(destination + 20u, source + 20u, runtime);
    UtilitiesRecordTextField::AssignInline(destination + 36u,
                                           source + 36u, runtime);
}

}  // namespace

void UtilitiesPreviewRecordCopy::AssignOne(
    const std::uint32_t destination, const std::uint32_t source,
    IUtilitiesRecordTextRuntime& runtime) {
    runtime.WriteWord(destination, runtime.ReadWord(source));
    UtilitiesRecordTextField::Assign(destination + 4u,
                                     source + 4u, runtime);
    UtilitiesRecordTextField::AssignInline(destination + 20u,
                                           source + 20u, runtime);
    UtilitiesRecordTextField::AssignInline(destination + 36u,
                                           source + 36u, runtime);
}

std::uint32_t UtilitiesPreviewRecordCopy::SelectUnique(
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

std::uint32_t UtilitiesPreviewRecordCopy::AssignRepeated(
    std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t source, IUtilitiesRecordTextRuntime& runtime) {
    while (begin != end) {
        AssignOne(begin, source, runtime);
        begin += kRecordBytes;
    }
    return begin;
}

std::uint32_t UtilitiesPreviewRecordCopy::AssignBackward(
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

void UtilitiesPreviewRecordCopy::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    IUtilitiesRecordTextRuntime& runtime) {
    if (destination == 0u) return;
    runtime.WriteWord(destination, runtime.ReadWord(source));
    for (const auto offset : kFields) {
        runtime.WriteByte(destination + offset,
                          runtime.ReadByte(source + offset));
        UtilitiesRecordTextField::Write(destination + offset, {}, runtime);
        UtilitiesRecordTextField::Assign(destination + offset,
                                         source + offset, runtime);
    }
}

}  // namespace pal4
