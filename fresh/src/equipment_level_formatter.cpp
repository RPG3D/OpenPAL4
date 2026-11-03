#include "pal4/equipment_level_formatter.h"

#include <cstdint>
#include <string>

namespace pal4 {

std::string EquipmentLevelFormatter::Format(std::int32_t level) {
    if (level <= 0 || level >= 100) return "0";
    const auto tens = level / 10;
    const auto ones = level % 10;
    if (tens <= 0) return std::to_string(ones) + "\n \n";
    if (ones != 0)
        return std::to_string(tens) + "\n0\n" +
               std::to_string(ones) + "\n \n ";
    return std::to_string(tens) + "\n\n0\n \n ";
}

}  // namespace pal4
