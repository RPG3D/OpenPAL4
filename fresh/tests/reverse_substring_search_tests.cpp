#include "pal4/reverse_substring_search.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string_view>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IReverseSubstringRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t reads = 0u;

    void Load(std::uint32_t address, std::string_view text) {
        for (std::uint32_t i = 0u; i < text.size(); ++i)
            bytes[address + i] = static_cast<std::uint8_t>(text[i]);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        ++reads;
        return bytes.at(address);
    }
};

void TestReverseSearch() {
    Runtime runtime;
    runtime.Load(0x1000u, "ababa");
    runtime.Load(0x2000u, "aba");
    using pal4::ReverseSubstringSearch;
    Check(ReverseSubstringSearch::Find(
              0x1000u, 5u, 0x2000u, 3u, 0xffffffffu, runtime) == 2u,
          "latest overlapping substring wins");
    Check(ReverseSubstringSearch::Find(
              0x1000u, 5u, 0x2000u, 3u, 1u, runtime) == 0u,
          "start position limits reverse search");
    Check(ReverseSubstringSearch::Find(
              0x1000u, 5u, 0x2000u, 3u, 0u, runtime) == 0u,
          "position zero is tested");
    runtime.Load(0x2000u, "abb");
    Check(ReverseSubstringSearch::Find(
              0x1000u, 5u, 0x2000u, 3u, 5u, runtime) == 0xffffffffu,
          "partial first-byte candidates do not match");
    const auto before = runtime.reads;
    Check(ReverseSubstringSearch::Find(
              0x1000u, 5u, 0x2000u, 0u, 2u, runtime) == 2u &&
              ReverseSubstringSearch::Find(
                  0x1000u, 5u, 0x2000u, 0u, 99u, runtime) == 5u &&
              ReverseSubstringSearch::Find(
                  0x1000u, 5u, 0x2000u, 6u, 99u, runtime) ==
                  0xffffffffu && runtime.reads == before,
          "empty and oversized needle return without memory reads");
}

}  // namespace

int main() {
    TestReverseSearch();
}
