#include "pal4/game_world_constants.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    std::uint32_t angle_bits = 0u;
    Check(pal4::GameWorldConstants::kFirstAngleBits == 0x3fc90fdbu &&
              pal4::GameWorldConstants::kSecondAngleBits == 0x40490fdbu,
          "IDA float constants retain their exact bits");
    pal4::GameWorldConstants::InitializeValue(angle_bits);
    Check(angle_bits == 0x4096cbe4u,
          "constant initializer stores the rounded float sum");
    angle_bits = 0xffffffffu;
    pal4::GameWorldConstants::Initialize(angle_bits);
    Check(angle_bits == 0x4096cbe4u,
          "thunk entry delegates to the same constant initializer");
}
