#include "pal4/thunder_flag.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    pal4::ThunderObjectState state{};
    state.words[15u] = 0x44332211u;
    Check(pal4::ThunderFlag::Get(state) == 0x33u,
          "getter reads owner byte +62 without activity gate");
    Check(pal4::ThunderFlag::Set(state, 0xabu) == 0xabu &&
              state.words[15u] == 0x44ab2211u &&
              pal4::ThunderFlag::Get(state) == 0xabu,
          "setter returns input and preserves adjacent bytes");
    state.words[1u] = 0u;
    Check(pal4::ThunderFlag::Set(state, 0u) == 0u &&
              state.words[15u] == 0x44002211u,
          "inactive owner still accepts flag write");
}
