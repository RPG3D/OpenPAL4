#include "pal4/uppercase_string_pair.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckNormalCopy() {
    std::string first = "scene_01";
    std::string second = "intro.a";
    const pal4::UppercaseStringPair pair(first, second);
    first[0] = 'x';
    second[0] = 'x';
    Check(pair.First() == "SCENE_01" && pair.Second() == "INTRO.A",
          "constructor copies and uppercases both independent strings");
}

void CheckEmptyAndSharedSources() {
    const pal4::UppercaseStringPair empty("", "");
    Check(empty.First().empty() && empty.Second().empty(),
          "empty buffers stay empty");
    const std::string source = "same key";
    const pal4::UppercaseStringPair shared(source, source);
    Check(shared.First() == "SAME KEY" && shared.Second() == "SAME KEY",
          "both fields can copy the same source buffer");
}

void CheckCStringBoundary() {
    constexpr std::string_view first("ab\0cd", 5);
    constexpr std::string_view second("x\0y", 3);
    const pal4::UppercaseStringPair pair(first, second);
    Check(pair.First() == "AB" && pair.Second() == "X",
          "native uppercase conversion stops at the first null byte");
}

}  // namespace

int main() {
    try {
        CheckNormalCopy();
        CheckEmptyAndSharedSources();
        CheckCStringBoundary();
        std::cout << "uppercase_string_pair_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
