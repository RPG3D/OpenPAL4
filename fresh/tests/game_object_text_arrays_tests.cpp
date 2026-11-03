#include "pal4/game_object_text_arrays.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::GameObjectTextCleanupRuntimeStub {
    std::vector<std::uint32_t> addresses;
    void DestroyTextRecord(std::uint32_t address) override {
        addresses.push_back(address);
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::GameObjectTextArrays::Cleanup(0x5000u, runtime);
    Check(runtime.addresses == std::vector<std::uint32_t>{
              0x50a0u, 0x5090u, 0x5080u, 0x5070u, 0x5060u,
              0x5030u, 0x5020u, 0x5010u, 0x5000u},
          "two arrays are destroyed in their observed reverse order");
}
