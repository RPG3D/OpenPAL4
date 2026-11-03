#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class PlayerStatSlots final {
public:
    std::int32_t Set(std::int32_t index, std::int32_t value) noexcept;
    std::int32_t At(std::uint32_t index) const;

private:
    std::array<std::int32_t, 5> values_{};
};

}  // namespace pal4
