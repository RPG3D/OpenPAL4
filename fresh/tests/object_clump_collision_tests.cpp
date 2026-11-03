#include "pal4/object_clump_collision.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IObjectClumpCollisionRuntime {
    std::uint32_t calls{};
    std::uint32_t vector{};
    std::uint32_t clump{};
    std::int32_t result{-13};

    std::int32_t ProcessClumpCollision(
        std::uint32_t vector_address, std::uint32_t clump_address,
        std::array<std::uint8_t, 24>& scratch) override {
        ++calls;
        vector = vector_address;
        clump = clump_address;
        scratch[0] = 0xa5u;
        scratch[23] = 0x5au;
        return result;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::ObjectClumpCollision::ProcessConditional(
              0x1200u, 0u, runtime) == 0 && runtime.calls == 0u,
          "null clump avoids the external call");
    Check(pal4::ObjectClumpCollision::ProcessConditional(
              0x1200u, 0x3400u, runtime) == -13 &&
              runtime.calls == 1u && runtime.vector == 0x1200u &&
              runtime.clump == 0x3400u,
          "non-null clump forwards both addresses and signed result");
    pal4::ObjectClumpCollisionRuntimeStub stub;
    Check(pal4::ObjectClumpCollision::ProcessConditional(
              0x1200u, 0x3400u, stub) == 0,
          "engine boundary has a linkable stub");
}
