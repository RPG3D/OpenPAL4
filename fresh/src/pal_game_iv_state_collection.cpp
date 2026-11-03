#include "pal4/pal_game_iv_state_collection.h"

#include <algorithm>
#include <new>

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 12u;
constexpr std::uint32_t kBeginOffset = 0x1a0u;
constexpr std::uint32_t kEndOffset = 0x1a4u;
constexpr std::uint32_t kCapacityOffset = 0x1a8u;

}  // namespace

std::uint32_t PalGameIVStateCollection::AddOrUpdate(
    std::uint32_t game, std::string_view text,
    IPalGameIVStateCollectionRuntime& runtime) {
    const auto begin = runtime.ReadWord(game + kBeginOffset);
    const auto end = runtime.ReadWord(game + kEndOffset);
    for (auto cursor = begin; cursor != end; cursor += kRecordBytes) {
        if (runtime.EqualsString(cursor, text))
            return runtime.CleanupInputTemporary(text);
    }

    const auto capacity_end = runtime.ReadWord(game + kCapacityOffset);
    if (end != capacity_end) {
        runtime.ConstructString(end, text);
        runtime.WriteWord(game + kEndOffset, end + kRecordBytes);
    } else {
        const auto count = begin == 0u ? 0u : (end - begin) / kRecordBytes;
        const auto capacity = count + std::max(1u, count);
        const auto replacement = runtime.Allocate(capacity * kRecordBytes);
        if (replacement == 0u) throw std::bad_alloc();
        for (std::uint32_t index = 0; index < count; ++index)
            runtime.CopyString(replacement + index * kRecordBytes,
                               begin + index * kRecordBytes);
        runtime.ConstructString(replacement + count * kRecordBytes, text);
        for (auto cursor = begin; cursor != end; cursor += kRecordBytes)
            (void)runtime.CleanupString(cursor);
        runtime.Free(begin);
        runtime.WriteWord(game + kBeginOffset, replacement);
        runtime.WriteWord(game + kEndOffset,
                          replacement + (count + 1u) * kRecordBytes);
        runtime.WriteWord(game + kCapacityOffset,
                          replacement + capacity * kRecordBytes);
    }
    return runtime.CleanupInputTemporary(text);
}

std::uint32_t PalGameIVStateCollection::ForEach(
    std::uint32_t game, std::uint32_t callback,
    std::uint32_t argument, IPalGameIVStateCollectionRuntime& runtime) {
    if (callback == 0u) return Visit(game, {}, runtime);
    return Visit(game, [&runtime, callback, argument](std::uint32_t text) {
        if (callback != 0u) runtime.InvokeCallback(callback, text, argument);
    }, runtime);
}

std::uint32_t PalGameIVStateCollection::Visit(std::uint32_t game,
    const std::function<void(std::uint32_t)>& callback,
    IPalGameIVStateCollectionRuntime& runtime) {
    auto cursor = runtime.ReadWord(game + kBeginOffset);
    auto end = runtime.ReadWord(game + kEndOffset);
    while (cursor != end) {
        if (callback) callback(runtime.ReadWord(cursor));
        cursor += kRecordBytes;
        end = runtime.ReadWord(game + kEndOffset);
    }
    return end;
}

std::uint32_t PalGameIVStateCollection::Clear(
    std::uint32_t game, IPalGameIVStateCollectionRuntime& runtime) {
    const auto begin = runtime.ReadWord(game + kBeginOffset);
    const auto end = runtime.ReadWord(game + kEndOffset);
    auto result = runtime.EmptyCleanupResult();
    for (auto cursor = begin; cursor != end; cursor += kRecordBytes)
        result = runtime.CleanupString(cursor);
    runtime.WriteWord(game + kEndOffset, begin);
    return result;
}

}  // namespace pal4
