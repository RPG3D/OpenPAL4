#include "pal4/xml_string_comparator.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestPrefixRules() {
    using Comparator = pal4::XmlStringComparator;
    const std::uint8_t full[]{'N', 'a', 'm', 'e', '=', 0u};
    const std::uint8_t exact[]{'N', 'a', 'm', 'e', '=', 0u};
    const std::uint8_t prefix[]{'N', 'a', 'm', 'e', 0u};
    const std::uint8_t too_long[]{'N', 'a', 'm', 'e', '=', 'x', 0u};
    const std::uint8_t empty[]{0u};
    Check(Comparator::HasPrefix(full, exact, false, 0) &&
              Comparator::HasPrefix(full, prefix, false, 0) &&
              Comparator::HasPrefix(full, empty, false, 0),
          "exact, shorter and empty prefixes match nonempty source");
    Check(!Comparator::HasPrefix(full, too_long, false, 0) &&
              !Comparator::HasPrefix(empty, empty, false, 0) &&
              !Comparator::HasPrefix(nullptr, prefix, false, 0),
          "longer prefix and empty source fail");
}

void TestCaseModes() {
    using Comparator = pal4::XmlStringComparator;
    const std::uint8_t source[]{'A', 'b', 'C', 0u};
    const std::uint8_t lower[]{'a', 'B', 0u};
    const std::uint8_t mismatch[]{'a', 'x', 0u};
    Check(!Comparator::HasPrefix(source, lower, false, 0) &&
              Comparator::HasPrefix(source, lower, true, 0) &&
              Comparator::HasPrefix(source, lower, true, 1) &&
              !Comparator::HasPrefix(source, mismatch, true, 1),
          "case folding applies only in ignore-case branch");
    const std::uint8_t extended[]{0x80u, 'A', 0u};
    const std::uint8_t same[]{0x80u, 'a', 0u};
    Check(Comparator::HasPrefix(extended, same, true, 1),
          "identical extended byte and folded ASCII match");
}

}  // namespace

int main() {
    TestPrefixRules();
    TestCaseModes();
}
