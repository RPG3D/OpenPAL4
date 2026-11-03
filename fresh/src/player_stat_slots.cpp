#include "pal4/player_stat_slots.h"

#include <cstddef>

namespace pal4 {

std::int32_t PlayerStatSlots::Set(std::int32_t index,
                                  std::int32_t value) noexcept {
    if (index < 0 || index >= static_cast<std::int32_t>(values_.size()))
        return index;
    values_[static_cast<std::size_t>(index)] = value;
    return value;
}

std::int32_t PlayerStatSlots::At(std::uint32_t index) const {
    return values_.at(index);
}

}  // namespace pal4
