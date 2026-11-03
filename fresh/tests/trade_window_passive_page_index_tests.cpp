#include "pal4/trade_window_passive_page_index.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckValidIndices() {
    for (std::int32_t index = 0; index < 3; ++index) {
        Check(pal4::TradeWindowPassivePageIndex::Next(index) ==
                  (index + 1) % 3 &&
                  pal4::TradeWindowPassivePageIndex::Previous(index) ==
                  (index + 2) % 3,
              "three-page forward and reverse cycle");
    }
}

void CheckSignedBoundaries() {
    Check(pal4::TradeWindowPassivePageIndex::Next(-2) == -1 &&
              pal4::TradeWindowPassivePageIndex::Next(3) == 0 &&
              pal4::TradeWindowPassivePageIndex::Previous(-2) == 2 &&
              pal4::TradeWindowPassivePageIndex::Previous(4) == 3,
          "out-of-range values use signed comparisons");
    Check(pal4::TradeWindowPassivePageIndex::Next(
              std::numeric_limits<std::int32_t>::max()) ==
              std::numeric_limits<std::int32_t>::min() &&
              pal4::TradeWindowPassivePageIndex::Previous(
                  std::numeric_limits<std::int32_t>::min()) ==
              std::numeric_limits<std::int32_t>::max(),
          "x86 increment and decrement wrap at signed boundaries");
}

}  // namespace

int main() {
    try {
        CheckValidIndices();
        CheckSignedBoundaries();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
