#include "pal4/game_data_plain_record_array.h"

namespace pal4 {
namespace {

std::uint32_t Count(std::uint32_t array, std::uint32_t stride,
                    IGameDataPlainRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    return begin ? (runtime.ReadWord(array + 8) - begin) / stride : 0;
}

std::uint32_t CopyRange(std::uint32_t begin, std::uint32_t end,
                        std::uint32_t destination, std::uint32_t stride,
                        IGameDataPlainRecordArrayRuntime& runtime) {
    for (auto source = begin; source != end;
         source += stride, destination += stride)
        if (destination) runtime.CopyRecord(destination, source, stride);
    return destination;
}

std::uint32_t UniqueCopy(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination, std::uint32_t stride,
    const std::function<bool(std::uint32_t, std::uint32_t)>& equivalent,
    IGameDataPlainRecordArrayRuntime& runtime) {
    if (begin == end) return destination;
    runtime.CopyRecord(destination, begin, stride);
    auto last_retained = begin;
    destination += stride;
    for (auto source = begin + stride; source != end; source += stride) {
        if (equivalent(last_retained, source)) continue;
        runtime.CopyRecord(destination, source, stride);
        destination += stride;
        last_retained = source;
    }
    return destination;
}

std::uint32_t InsertOne(std::uint32_t array, std::uint32_t position,
                        std::uint32_t source, std::uint32_t stride,
                        IGameDataPlainRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto old_end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto index = (position - begin) / stride;

    if ((capacity_end - old_end) / stride) {
        if ((old_end - position) / stride) {
            static_cast<void>(CopyRange(old_end - stride, old_end,
                                        old_end, stride, runtime));
            for (auto destination = old_end - stride;
                 destination != position; destination -= stride)
                runtime.CopyRecord(destination, destination - stride,
                                   stride);
            runtime.CopyRecord(position, source, stride);
        } else {
            static_cast<void>(CopyRange(position, old_end,
                                        position + stride, stride, runtime));
            if (old_end) runtime.CopyRecord(old_end, source, stride);
        }
        runtime.WriteWord(array + 8, old_end + stride);
    } else {
        const auto old_count = Count(array, stride, runtime);
        const auto growth = old_count > 1 ? old_count : 1u;
        const auto new_capacity = old_count + growth;
        const auto alloc_count = static_cast<std::int32_t>(new_capacity) < 0
                                     ? 0u : new_capacity;
        const auto new_begin = runtime.Allocate(stride * alloc_count);
        auto destination = CopyRange(begin, position, new_begin,
                                     stride, runtime);
        if (destination) runtime.CopyRecord(destination, source, stride);
        destination += stride;
        static_cast<void>(CopyRange(position, old_end, destination,
                                    stride, runtime));
        runtime.Free(begin);
        runtime.WriteWord(array + 12, new_begin + new_capacity * stride);
        const auto count = Count(array, stride, runtime);
        runtime.WriteWord(array + 4, new_begin);
        runtime.WriteWord(array + 8, new_begin + (count + 1) * stride);
    }
    return runtime.ReadWord(array + 4) + index * stride;
}

enum class BulkBranch { empty, long_tail, short_tail, reallocated };

struct BulkOutcome {
    BulkBranch branch;
    std::uint32_t old_begin;
    std::uint32_t old_end;
};

BulkOutcome InsertCopies(std::uint32_t array, std::uint32_t position,
                         std::uint32_t count, std::uint32_t source,
                         std::uint32_t stride,
                         IGameDataPlainRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto old_end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto inserted_bytes = count * stride;
    const auto tail_count = (old_end - position) / stride;

    if ((capacity_end - old_end) / stride >= count) {
        if (!count) return {BulkBranch::empty, begin, old_end};
        if (tail_count >= count) {
            static_cast<void>(CopyRange(old_end - inserted_bytes, old_end,
                                        old_end, stride, runtime));
            for (auto from = old_end - inserted_bytes; from != position;) {
                from -= stride;
                runtime.CopyRecord(from + inserted_bytes, from, stride);
            }
            for (auto at = position; at != position + inserted_bytes;
                 at += stride)
                runtime.CopyRecord(at, source, stride);
            runtime.WriteWord(array + 8, old_end + inserted_bytes);
            return {BulkBranch::long_tail, begin, old_end};
        }
        static_cast<void>(CopyRange(position, old_end,
                                    position + inserted_bytes,
                                    stride, runtime));
        for (auto at = old_end; at != old_end +
             (count - tail_count) * stride; at += stride)
            if (at) runtime.CopyRecord(at, source, stride);
        for (auto at = position; at != old_end; at += stride)
            runtime.CopyRecord(at, source, stride);
        runtime.WriteWord(array + 8, old_end + inserted_bytes);
        return {BulkBranch::short_tail, begin, old_end};
    }

    const auto old_count = Count(array, stride, runtime);
    const auto growth = old_count && old_count > count ? old_count : count;
    const auto new_capacity = old_count + growth;
    const auto alloc_count = static_cast<std::int32_t>(new_capacity) < 0
                                 ? 0u : new_capacity;
    const auto new_begin = runtime.Allocate(alloc_count * stride);
    auto destination = CopyRange(begin, position, new_begin,
                                 stride, runtime);
    for (std::uint32_t i = 0; i < count; ++i, destination += stride)
        if (destination) runtime.CopyRecord(destination, source, stride);
    static_cast<void>(CopyRange(position, old_end, destination,
                                stride, runtime));
    runtime.Free(begin);
    runtime.WriteWord(array + 12,
                      new_begin + new_capacity * stride);
    const auto retained = Count(array, stride, runtime);
    runtime.WriteWord(array + 4, new_begin);
    runtime.WriteWord(array + 8,
                      new_begin + (retained + count) * stride);
    return {BulkBranch::reallocated, begin, old_end};
}

std::uint32_t FillRange(std::uint32_t destination,
                        std::uint32_t count, std::uint32_t source,
                        std::uint32_t stride,
                        IGameDataPlainRecordArrayRuntime& runtime) {
    if (!count) return 0;
    for (std::uint32_t i = 0; i < count; ++i,
         destination += stride)
        if (destination) runtime.CopyRecord(destination, source, stride);
    return destination;
}

}  // namespace

std::uint32_t GameDataUpgradeRecordArray::InsertOne(
    std::uint32_t array, std::uint32_t position, std::uint32_t source,
    IGameDataPlainRecordArrayRuntime& runtime) {
    return pal4::InsertOne(array, position, source, kRecordBytes, runtime);
}

std::uint32_t GameDataUpgradeRecordArray::Count(
    std::uint32_t array, IGameDataPlainRecordArrayRuntime& runtime) {
    return pal4::Count(array, kRecordBytes, runtime);
}

std::uint32_t GameDataUpgradeRecordArray::CopyRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IGameDataPlainRecordArrayRuntime& runtime) {
    return pal4::CopyRange(begin, end, destination, kRecordBytes, runtime);
}

std::uint32_t GameDataUpgradeRecordArray::InsertCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IGameDataPlainRecordArrayRuntime& runtime) {
    const auto outcome = pal4::InsertCopies(
        array, position, count, source, kRecordBytes, runtime);
    switch (outcome.branch) {
    case BulkBranch::empty:
        return 0;
    case BulkBranch::long_tail:
        return position + count * kRecordBytes;
    case BulkBranch::short_tail:
        return outcome.old_end + count * kRecordBytes;
    case BulkBranch::reallocated:
        return outcome.old_begin
                   ? static_cast<std::uint32_t>(
                         0x66666667ull *
                         (outcome.old_end - outcome.old_begin))
                   : 0;
    }
    return 0;
}

std::uint32_t GameDataUpgradeRecordArray::FillRange(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source,
    IGameDataPlainRecordArrayRuntime& runtime) {
    return pal4::FillRange(destination, count, source,
                            kRecordBytes, runtime);
}

void GameDataUpgradeRecordArray::CopyOne(
    std::uint32_t destination, std::uint32_t source,
    IGameDataPlainRecordArrayRuntime& runtime) {
    if (destination) runtime.CopyRecord(destination, source, kRecordBytes);
}

std::uint32_t GameDataUpgradeRecordArray::UniqueCopy(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    const std::function<bool(std::uint32_t, std::uint32_t)>& equivalent,
    IGameDataPlainRecordArrayRuntime& runtime) {
    return pal4::UniqueCopy(begin, end, destination, kRecordBytes,
                            equivalent, runtime);
}

std::uint32_t GameDataCombatPrizeRecordArray::InsertOne(
    std::uint32_t array, std::uint32_t position, std::uint32_t source,
    IGameDataPlainRecordArrayRuntime& runtime) {
    return pal4::InsertOne(array, position, source, kRecordBytes, runtime);
}

std::uint32_t GameDataCombatPrizeRecordArray::Count(
    std::uint32_t array, IGameDataPlainRecordArrayRuntime& runtime) {
    return pal4::Count(array, kRecordBytes, runtime);
}

std::uint32_t GameDataCombatPrizeRecordArray::CopyRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IGameDataPlainRecordArrayRuntime& runtime) {
    return pal4::CopyRange(begin, end, destination, kRecordBytes, runtime);
}

std::uint32_t GameDataCombatPrizeRecordArray::InsertCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IGameDataPlainRecordArrayRuntime& runtime) {
    const auto outcome = pal4::InsertCopies(
        array, position, count, source, kRecordBytes, runtime);
    if (outcome.branch == BulkBranch::short_tail)
        return count * kRecordBytes;
    return runtime.ReadWord(array + 8);
}

std::uint32_t GameDataCombatPrizeRecordArray::FillRange(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source,
    IGameDataPlainRecordArrayRuntime& runtime) {
    return pal4::FillRange(destination, count, source,
                            kRecordBytes, runtime);
}

void GameDataCombatPrizeRecordArray::CopyOne(
    std::uint32_t destination, std::uint32_t source,
    IGameDataPlainRecordArrayRuntime& runtime) {
    if (destination) runtime.CopyRecord(destination, source, kRecordBytes);
}

std::uint32_t GameDataCombatPrizeRecordArray::UniqueCopy(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    const std::function<bool(std::uint32_t, std::uint32_t)>& equivalent,
    IGameDataPlainRecordArrayRuntime& runtime) {
    return pal4::UniqueCopy(begin, end, destination, kRecordBytes,
                            equivalent, runtime);
}

}  // namespace pal4
