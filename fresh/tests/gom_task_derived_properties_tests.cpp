#include "pal4/gom_task_derived_properties.h"

#include <array>
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
    std::array<std::uint32_t, 46u> words{};
    words[15u] = 0xaabbccdd;
    words[17u] = 0xdeadbeefu;
    Check(pal4::GomTaskDerivedProperties::GetFlag60(words) == 0xddu &&
              pal4::GomTaskDerivedProperties::GetFlag61(words) == 0xccu,
          "two shared getters read adjacent bytes +60 and +61");
    Check(pal4::GomTaskDerivedProperties::SetWord68(
              words, 0x12345678u) == 0x12345678u &&
              words[17u] == 0x12345678u &&
              words[15u] == 0xaabbccddu,
          "setter writes full word +68 and returns input");
    Check(!pal4::GomTaskDerivedProperties::IsNonZero(0u) &&
              pal4::GomTaskDerivedProperties::IsNonZero(1u) &&
              pal4::GomTaskDerivedProperties::IsNonZero(0x80000000u),
          "shared predicate tests full input word");
}
