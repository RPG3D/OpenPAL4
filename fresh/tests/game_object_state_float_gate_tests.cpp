#include "pal4/game_object_state_float_gate.h"

#include <bit>
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

bool Compare(float left, float right) {
    pal4::BehaviorPathEndpointState helper{};
    helper.words[0xc4u / 4u] = std::bit_cast<std::uint32_t>(left);
    helper.words[0xc0u / 4u] = std::bit_cast<std::uint32_t>(right);
    return pal4::GameObjectStateFloatGate::Check(helper);
}

}  // namespace

int main() {
    Check(!Compare(1.0f, 2.0f), "less-than sets x87 C0");
    Check(Compare(2.0f, 2.0f), "equality clears x87 C0");
    Check(Compare(3.0f, 2.0f), "greater-than clears x87 C0");
    Check(!Compare(std::numeric_limits<float>::quiet_NaN(), 2.0f) &&
          !Compare(2.0f, std::numeric_limits<float>::quiet_NaN()),
          "unordered comparison sets x87 C0");
    Check(Compare(std::numeric_limits<float>::infinity(), 2.0f) &&
          !Compare(-std::numeric_limits<float>::infinity(), 2.0f),
          "infinity comparisons follow ordered float semantics");
}
