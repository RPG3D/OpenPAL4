#include "pal4/thunder_visual_storage.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IThunderVisualStorageRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    void ResetProperties(pal4::ThunderObjectState&,
                         const std::uint32_t offset) override {
        Check(offset == 60u, "reset receives visual subobject offset");
        calls.emplace_back("reset");
    }
    std::uint32_t Free(const std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
        return address + 1u;
    }
};

}  // namespace

int main() {
    pal4::ThunderObjectState state{};
    state.words[47u] = 0x1110u;
    state.words[48u] = 2u;
    state.words[49u] = 3u;
    state.words[51u] = 0x2220u;
    state.words[52u] = 4u;
    state.words[53u] = 5u;
    Runtime runtime;
    Check(pal4::ThunderVisualStorage::Cleanup(state, runtime) == 0x1111u &&
              runtime.calls ==
                  std::vector<std::string>{"reset", "free", "free"} &&
              runtime.freed == std::vector<std::uint32_t>{0x2220u, 0x1110u},
          "reset and free order, with second free result");
    for (const auto i : {47u, 48u, 49u, 51u, 52u, 53u})
        Check(state.words[i] == 0u, "both owned blocks cleared");
    Check(pal4::ThunderVisualStorage::Cleanup(state, runtime) == 1u &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0x2220u, 0x1110u, 0u, 0u},
          "repeat cleanup forwards null frees");
}
