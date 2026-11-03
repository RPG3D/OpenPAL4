#include "pal4/game_data_equipment_attribute_description.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <string>

namespace pal4 {
namespace {

struct Field {
    std::uint32_t index;
    std::uint32_t label_key;
};

constexpr std::array<Field, 19> kIntegerFields{{
    {0, 0x29}, {1, 0x2a}, {2, 0x2b},
    {3, 0x0c}, {4, 0x0d}, {5, 0x0e}, {6, 0x0f}, {7, 0x10},
    {31, 0x11}, {32, 0x12}, {33, 0x13}, {34, 0x14}, {35, 0x15},
    {13, 0x42}, {14, 0x43}, {15, 0x44},
    {16, 0x45}, {17, 0x46}, {18, 0x47},
}};

constexpr std::array<Field, 16> kPercentFields{{
    {25, 0x48}, {26, 0x49}, {27, 0x4a}, {28, 0x4b},
    {29, 0x4c}, {30, 0x4d}, {19, 0x4e}, {20, 0x4f},
    {21, 0x50}, {22, 0x51}, {23, 0x52}, {24, 0x53},
    {9, 0x54}, {10, 0x55}, {11, 0x56}, {12, 0x57},
}};

void Append(std::string& result, std::string label,
            const std::string& value, char separator) {
    result += label;
    result += separator;
    result += value;
    result += ' ';
}

}  // namespace

std::uint32_t GameDataEquipmentAttributeDescription::Build(
    std::uint32_t object, std::uint32_t destination,
    IGameDataEquipmentAttributeDescriptionRuntime& runtime) {
    std::string result;
    for (std::uint32_t i = 0; i < kIntegerFields.size(); ++i) {
        const auto field = kIntegerFields[i];
        const auto value = i < 8
            ? runtime.IndexedValue(object, field.index)
            : runtime.ReadWord(object + field.index * 4);
        if (!value) continue;
        Append(result, runtime.Label(field.label_key),
               std::to_string(static_cast<std::int32_t>(value)), '+');
    }

    for (const auto field : kPercentFields) {
        const auto value = runtime.ReadFloat(object + field.index * 4);
        if (value == 0.0f) continue;
        const auto percent = static_cast<std::int32_t>(
            std::trunc(static_cast<double>(value) * 100.0));
        Append(result, runtime.Label(field.label_key),
               std::to_string(percent) + '%', '+');
    }

    const auto first_raw = runtime.ReadWord(object + 380);
    const auto first = static_cast<std::int32_t>(first_raw) <= 0
        ? 0u : runtime.ReadByte(object + 380);
    Append(result, runtime.Label(0x59), std::to_string(first), ':');

    const auto second_raw = runtime.ReadWord(object + 388);
    const auto second = static_cast<std::int32_t>(second_raw) <= 0
        ? 0u : second_raw;
    Append(result, runtime.Label(0x58), std::to_string(second), ':');

    runtime.CopyDescription(destination, result);
    return destination;
}

}  // namespace pal4
