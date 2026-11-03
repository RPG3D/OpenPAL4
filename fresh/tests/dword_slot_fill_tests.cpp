#include "pal4/dword_slot_fill.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::ISpatialGridRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    std::uint32_t Free(std::uint32_t) override { return 0u; }
    pal4::SpatialGridState& GridAt(std::uint32_t) override {
        return grid;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    pal4::SpatialGridState grid{};
    std::map<std::uint32_t, std::uint32_t> words;
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::DwordSlotFill::Fill(
              0x1000u, 3u, 0xaabbccddu, runtime) == 0x100cu &&
              runtime.words.size() == 3u &&
              runtime.words[0x1000u] == 0xaabbccddu &&
              runtime.words[0x1004u] == 0xaabbccddu &&
              runtime.words[0x1008u] == 0xaabbccddu,
          "fills contiguous dword slots and returns advanced address");
    Check(pal4::DwordSlotFill::Fill(
              0x2000u, 0u, 3u, runtime) == 0u &&
              runtime.words.size() == 3u,
          "zero count returns zero without writes");
    Check(pal4::DwordSlotFill::Fill(
              0u, 2u, 4u, runtime) == 8u &&
              runtime.words.size() == 3u,
          "null destination safely preserves pointer arithmetic result");
    std::cout << "Dword slot fill tests passed\n";
}
