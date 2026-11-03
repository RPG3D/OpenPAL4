#include "pal4/player_stat_slots.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestFiveSlotsAndReturnValue() {
    pal4::PlayerStatSlots slots;
    for (std::int32_t index = 0; index < 5; ++index) {
        const auto value = 100 + index;
        Check(slots.Set(index, value) == value &&
                  slots.At(static_cast<std::uint32_t>(index)) == value,
              "valid index writes and returns supplied value");
    }
    Check(slots.Set(4, -9) == -9 && slots.At(4) == -9,
          "last slot accepts signed value");
}

void TestInvalidIndicesPreserveSlots() {
    pal4::PlayerStatSlots slots;
    for (std::int32_t index = 0; index < 5; ++index)
        slots.Set(index, index + 10);
    for (const auto index : {-1, -5, 5, 100,
                             std::numeric_limits<std::int32_t>::min(),
                             std::numeric_limits<std::int32_t>::max()}) {
        Check(slots.Set(index, 99) == index,
              "out-of-range index is returned unchanged");
    }
    for (std::uint32_t index = 0; index < 5u; ++index)
        Check(slots.At(index) == static_cast<std::int32_t>(index) + 10,
              "invalid index does not write any slot");
}

}  // namespace

int main() {
    TestFiveSlotsAndReturnValue();
    TestInvalidIndicesPreserveSlots();
}
