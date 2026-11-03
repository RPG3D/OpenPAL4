#include "pal4/game_data_record_access.h"

#include <cstdint>
#include <vector>

namespace pal4 {
namespace {

std::uint32_t LowerBoundAddress(
    std::uint32_t object, std::int32_t key,
    IGameDataRecordAccessRuntime& runtime) {
    const auto table = runtime.ReadWord(object + 8);
    const auto begin = runtime.ReadWord(table + 8);
    const auto end = runtime.ReadWord(table + 12);
    const auto count = static_cast<std::int32_t>(end - begin) >> 3;
    auto first = begin;
    auto remaining = count;
    while (remaining > 0) {
        const auto half = remaining / 2;
        const auto middle = first + static_cast<std::uint32_t>(half) * 8;
        const auto candidate =
            static_cast<std::int32_t>(runtime.ReadWord(middle));
        if (candidate >= key) {
            remaining = half;
        } else {
            first = middle + 8;
            remaining -= half + 1;
        }
    }
    if (first == end || key <
            static_cast<std::int32_t>(runtime.ReadWord(first)))
        return 0;
    const auto index = runtime.ReadWord(first + 4);
    return runtime.ReadWord(table + 96) + index * 400;
}

}  // namespace

std::uint32_t GameDataRecordAccess::MemberAt16(
    std::uint32_t object, IGameDataRecordAccessRuntime& runtime) {
    return runtime.ReadWord(object + 16);
}

std::uint32_t GameDataRecordAccess::FindEffect(
    std::uint32_t object, std::uint32_t key,
    IGameDataRecordAccessRuntime& runtime) {
    return runtime.FindEffectEntry(runtime.ReadWord(object + 12), key);
}

std::uint32_t GameDataRecordAccess::FirstMatchingRecordValue(
    std::uint32_t object, std::uint32_t key,
    IGameDataRecordAccessRuntime& runtime) {
    const auto collection = runtime.ReadWord(object + 4);
    const auto begin = runtime.ReadWord(collection + 96);
    if (!begin) return 5;
    const auto end = runtime.ReadWord(collection + 100);
    const auto count = (end - begin) / 12;
    std::vector<std::uint32_t> matches;
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto record = begin + index * 12;
        if (runtime.ReadWord(record + 8) == key)
            matches.push_back(record);
    }
    return matches.empty() ? 5 : runtime.ReadWord(matches.front() + 4);
}

std::uint32_t GameDataRecordAccess::FindRecord(
    std::uint32_t object, std::int32_t key,
    IGameDataRecordAccessRuntime& runtime) {
    return LowerBoundAddress(object, key, runtime);
}

bool GameDataRecordAccess::HasRecord(
    std::uint32_t object, std::int32_t key,
    IGameDataRecordAccessRuntime& runtime) {
    return LowerBoundAddress(object, key, runtime) != 0;
}

}  // namespace pal4
