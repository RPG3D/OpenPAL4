#include "pal4/player_indexed_stats.h"

#include <bit>

namespace pal4 {
namespace {

bool InRange(const std::int32_t index, const std::int32_t count) {
    return index >= 0 && index < count;
}

double GetFloat(const std::uint32_t player, const std::int32_t index,
                const std::int32_t count, const std::uint32_t offset,
                IPlayerIndexedStatsRuntime& runtime) {
    if (!InRange(index, count)) return static_cast<double>(index);
    const auto bits = runtime.ReadWord(player + offset +
                                       static_cast<std::uint32_t>(index) * 4u);
    return static_cast<double>(std::bit_cast<float>(bits));
}

double SetFloat(const std::uint32_t player, const std::int32_t index,
                const std::int32_t count, const std::uint32_t offset,
                const float value, IPlayerIndexedStatsRuntime& runtime) {
    if (!InRange(index, count)) return static_cast<double>(index);
    runtime.WriteWord(player + offset +
                          static_cast<std::uint32_t>(index) * 4u,
                      std::bit_cast<std::uint32_t>(value));
    return static_cast<double>(value);
}

}  // namespace

std::uint32_t PlayerIndexedStats::GetCurrent(
    const std::uint32_t player, const std::int32_t index,
    IPlayerIndexedStatsRuntime& runtime) {
    if (!InRange(index, 5)) return static_cast<std::uint32_t>(index);
    return runtime.ReadWord(player + 0x7cu +
                            static_cast<std::uint32_t>(index) * 4u);
}

std::uint32_t PlayerIndexedStats::AddBase(
    const std::uint32_t player, const std::int32_t index,
    const std::uint32_t delta, IPlayerIndexedStatsRuntime& runtime) {
    if (!InRange(index, 8)) return static_cast<std::uint32_t>(index);
    const auto address = player + 4u +
                         static_cast<std::uint32_t>(index) * 4u;
    const auto result = runtime.ReadWord(address) + delta;
    runtime.WriteWord(address, result);
    return result;
}

double PlayerIndexedStats::GetAttribute(
    const std::uint32_t player, const std::int32_t index,
    IPlayerIndexedStatsRuntime& runtime) {
    return GetFloat(player, index, 4, 0x24u, runtime);
}

double PlayerIndexedStats::SetAttribute(
    const std::uint32_t player, const std::int32_t index,
    const float value, IPlayerIndexedStatsRuntime& runtime) {
    return SetFloat(player, index, 4, 0x24u, value, runtime);
}

double PlayerIndexedStats::GetSkill(
    const std::uint32_t player, const std::int32_t index,
    IPlayerIndexedStatsRuntime& runtime) {
    return GetFloat(player, index, 6, 0x64u, runtime);
}

double PlayerIndexedStats::SetSkill(
    const std::uint32_t player, const std::int32_t index,
    const float value, IPlayerIndexedStatsRuntime& runtime) {
    return SetFloat(player, index, 6, 0x64u, value, runtime);
}

double PlayerIndexedStats::SetFloatResistance(
    const std::uint32_t player, const std::int32_t index,
    const float value, IPlayerIndexedStatsRuntime& runtime) {
    return SetFloat(player, index, 6, 0x4cu, value, runtime);
}

std::uint32_t PlayerIndexedStats::GetIntegerResistance(
    const std::uint32_t player, const std::int32_t index,
    IPlayerIndexedStatsRuntime& runtime) {
    if (!InRange(index, 6)) return static_cast<std::uint32_t>(index);
    return runtime.ReadWord(player + 0x34u +
                            static_cast<std::uint32_t>(index) * 4u);
}

std::uint32_t PlayerIndexedStats::SetIntegerResistance(
    const std::uint32_t player, const std::int32_t index,
    const std::uint32_t value, IPlayerIndexedStatsRuntime& runtime) {
    if (!InRange(index, 6)) return static_cast<std::uint32_t>(index);
    runtime.WriteWord(player + 0x34u +
                          static_cast<std::uint32_t>(index) * 4u,
                      value);
    return value;
}

}  // namespace pal4
