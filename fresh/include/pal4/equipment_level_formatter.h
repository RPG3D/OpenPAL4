#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class EquipmentLevelFormatter final {
public:
    [[nodiscard]] static std::string Format(std::int32_t level);
};

}  // namespace pal4
