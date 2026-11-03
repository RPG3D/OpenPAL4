#include "pal4/smith_window_item_quantity.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowQuantityRuntime {
    static constexpr std::uint32_t item = 0x5000u;
    std::array<std::int32_t, 4> values{};
    std::vector<std::uint32_t> addresses;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address >= item + 0x168u &&
                  address <= item + 0x174u &&
                  (address - item - 0x168u) % 4u == 0u,
              "quantity address");
        addresses.push_back(address);
        return static_cast<std::uint32_t>(
            values[(address - item - 0x168u) / 4u]);
    }
};

void CheckFirstPositive() {
    Runtime first;
    first.values = {7, 8, 9, 10};
    Check(pal4::SmithWindowItemQuantity::FirstPositive(
              Runtime::item, first) == 7 &&
              first.addresses ==
                  (std::vector<std::uint32_t>{Runtime::item + 0x168u}),
          "first positive stops immediately");

    Runtime later;
    later.values = {-3, 0, 5, 8};
    Check(pal4::SmithWindowItemQuantity::FirstPositive(
              Runtime::item, later) == 5 &&
              later.addresses.size() == 3u,
          "signed nonpositive values are skipped");

    Runtime none;
    none.values = {0, -1, -2, 0};
    Check(pal4::SmithWindowItemQuantity::FirstPositive(
              Runtime::item, none) == 0 &&
              none.addresses.size() == 4u,
          "four nonpositive values return zero");
}

}  // namespace

int main() {
    try {
        CheckFirstPositive();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
