#include "pal4/player_property_validator.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestOrdinaryAndInvalidIndexes() {
    pal4::PlayerPropertyState state{};
    state.words.fill(0x77777777u);
    for (std::uint32_t index = 0u; index < 7u; ++index)
        Check(pal4::PlayerPropertyValidator::Set(
                  state, index, -20) == 0xffffffecu &&
                  state.words[51u + index] == 0xffffffecu,
              "ordinary property accepts negative raw value");
    const auto before = state.words;
    Check(pal4::PlayerPropertyValidator::Set(state, 10u, 12) == 10u &&
              pal4::PlayerPropertyValidator::Set(
                  state, 0xffffffffu, 12) == 0xffffffffu &&
              state.words == before,
          "out-of-range indexes return unchanged and write nothing");
}

void TestClamps() {
    pal4::PlayerPropertyState state{};
    state.words[1] = 80u;
    state.words[3] = 40u;
    Check(pal4::PlayerPropertyValidator::Set(state, 7u, 81) == 80u &&
              state.words[58] == 80u,
          "property 7 uses word one as upper bound");
    Check(pal4::PlayerPropertyValidator::Set(state, 7u, -1) == 0u &&
              pal4::PlayerPropertyValidator::Set(state, 7u, 80) == 80u,
          "property 7 floors negatives and keeps upper equality");
    Check(pal4::PlayerPropertyValidator::Set(state, 8u, 101) == 100u &&
              pal4::PlayerPropertyValidator::Set(state, 8u, -3) == 0u &&
              pal4::PlayerPropertyValidator::Set(state, 8u, 100) == 100u,
          "property 8 clamps to zero through 100");
    Check(pal4::PlayerPropertyValidator::Set(state, 9u, 41) == 40u &&
              pal4::PlayerPropertyValidator::Set(state, 9u, -1) == 0u &&
              pal4::PlayerPropertyValidator::Set(state, 9u, 40) == 40u,
          "property 9 uses word three as upper bound");
}

void TestNegativeMaximumPrecedesFloor() {
    pal4::PlayerPropertyState state{};
    state.words[1] = 0xfffffffeu;
    state.words[3] = 0xfffffffdu;
    Check(pal4::PlayerPropertyValidator::Set(state, 7u, -1) ==
              0xfffffffeu &&
              pal4::PlayerPropertyValidator::Set(state, 9u, -2) ==
                  0xfffffffdu,
          "signed upper comparison wins before zero floor");
}

}  // namespace

int main() {
    TestOrdinaryAndInvalidIndexes();
    TestClamps();
    TestNegativeMaximumPrecedesFloor();
}
