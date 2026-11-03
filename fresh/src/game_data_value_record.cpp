#include "pal4/game_data_value_record.h"

#include <array>
#include <bit>

namespace pal4 {
namespace {

double ReadFloat(std::uint32_t address, IGameDataValueRecordRuntime& runtime) {
    return static_cast<double>(std::bit_cast<float>(runtime.ReadWord(address)));
}

}  // namespace

std::uint32_t GameDataValueRecord::Clear(
    std::uint32_t object, IGameDataValueRecordRuntime& runtime) {
    for (std::uint32_t offset = 16; offset < 68; offset += 4)
        runtime.WriteWord(object + offset, 0);
    return object;
}

bool GameDataValueRecord::CheckValues(
    std::uint32_t object, IGameDataValueRecordRuntime& runtime) {
    const auto first_index = static_cast<std::int32_t>(runtime.ReadWord(object + 8));
    if (first_index < 0 || first_index >= 8) return false;
    const auto second_index = static_cast<std::int32_t>(runtime.ReadWord(object + 12));
    if (second_index < 0 || second_index >= 4) return false;

    constexpr std::array<std::uint32_t, 8> first_offsets{
        24, 28, 32, 36, 40, 48, 52, 44};
    double first_sum = ReadFloat(object + first_offsets[0], runtime);
    for (std::size_t i = 1; i < first_offsets.size(); ++i)
        first_sum += ReadFloat(object + first_offsets[i], runtime);
    const double epsilon = static_cast<double>(0.001f);
    if (first_sum - 1.0 > epsilon && first_sum > epsilon) return false;

    constexpr std::array<std::uint32_t, 3> second_offsets{60, 64, 56};
    double second_sum = ReadFloat(object + second_offsets[0], runtime);
    for (std::size_t i = 1; i < second_offsets.size(); ++i)
        second_sum += ReadFloat(object + second_offsets[i], runtime);
    return !(second_sum - 1.0 > epsilon);
}

}  // namespace pal4
