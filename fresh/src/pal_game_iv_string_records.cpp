#include "pal4/pal_game_iv_string_records.h"

#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 12u;

void CheckRange(std::uint32_t begin, std::uint32_t end) {
    if (end < begin || (end - begin) % kRecordBytes != 0u)
        throw std::invalid_argument("unaligned PAL4 string record range");
}

}  // namespace

std::uint32_t PalGameIVStringRecords::CleanupAndFree(
    std::uint32_t owner, IPalGameIVStringRecordRuntime& runtime) {
    const auto begin = runtime.ReadWord(owner + 4u);
    const auto end = runtime.ReadWord(owner + 8u);
    CheckRange(begin, end);
    if (begin != end) (void)CleanupRange(begin, end, runtime);
    runtime.Free(begin);
    runtime.WriteWord(owner + 4u, 0u);
    runtime.WriteWord(owner + 8u, 0u);
    runtime.WriteWord(owner + 12u, 0u);
    return 0u;
}

std::uint32_t PalGameIVStringRecords::CleanupRange(
    std::uint32_t begin, std::uint32_t end,
    IPalGameIVStringRecordRuntime& runtime) {
    CheckRange(begin, end);
    if (begin == end) return runtime.EmptyResult();
    auto result = runtime.Cleanup(begin);
    for (auto cursor = begin + kRecordBytes; cursor != end;
         cursor += kRecordBytes)
        result = runtime.Cleanup(cursor);
    return result;
}

std::uint32_t PalGameIVStringRecords::CopyRange(
    std::uint32_t source_begin, std::uint32_t source_end,
    std::uint32_t destination, IPalGameIVStringRecordRuntime& runtime) {
    CheckRange(source_begin, source_end);
    for (auto cursor = source_begin; cursor != source_end;
         cursor += kRecordBytes, destination += kRecordBytes)
        (void)runtime.CopyConstruct(destination, cursor);
    return destination;
}

std::uint32_t PalGameIVStringRecords::CopyCount(
    std::uint32_t destination, std::uint32_t count, std::uint32_t source,
    IPalGameIVStringRecordRuntime& runtime) {
    if (count == 0u) return runtime.EmptyResult();
    auto result = runtime.CopyConstruct(destination, source);
    destination += kRecordBytes;
    for (std::uint32_t index = 1u; index < count; ++index) {
        result = runtime.CopyConstruct(destination, source);
        destination += kRecordBytes;
    }
    return result;
}

std::uint32_t PalGameIVStringRecords::InitializeRange(
    std::uint32_t destination_begin, std::uint32_t destination_end,
    std::uint32_t source, IPalGameIVStringRecordRuntime& runtime) {
    CheckRange(destination_begin, destination_end);
    if (destination_begin == destination_end) return runtime.EmptyResult();
    auto result = runtime.InitializeBuffer(
        destination_begin, runtime.ReadWord(source),
        runtime.ReadWord(source + 4u));
    for (auto cursor = destination_begin + kRecordBytes;
         cursor != destination_end;
         cursor += kRecordBytes)
        result = runtime.InitializeBuffer(
            cursor, runtime.ReadWord(source), runtime.ReadWord(source + 4u));
    return result;
}

std::uint32_t PalGameIVStringRecords::InitializeReverse(
    std::uint32_t source_begin, std::uint32_t source_end,
    std::uint32_t destination_end, IPalGameIVStringRecordRuntime& runtime) {
    CheckRange(source_begin, source_end);
    auto destination = destination_end;
    for (auto source = source_end; source != source_begin;) {
        source -= kRecordBytes;
        destination -= kRecordBytes;
        (void)runtime.InitializeBuffer(
            destination, runtime.ReadWord(source),
            runtime.ReadWord(source + 4u));
    }
    return destination;
}

}  // namespace pal4
