#include "pal4/property_string_pair.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyStringPairRuntime {
    std::vector<std::string> calls;
    std::vector<int> comparisons;
    std::size_t comparison_index = 0u;
    std::uint8_t first_count = 0u;
    std::uint8_t second_count = 0u;

    int CompareBytes(std::uint32_t right, std::uint32_t left,
                     std::uint32_t count) override {
        calls.push_back("compare:" + std::to_string(right) + ":" +
                        std::to_string(left) + ":" + std::to_string(count));
        return comparison_index < comparisons.size()
            ? comparisons[comparison_index++] : 0;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return address == 0x1fffu ? first_count : second_count;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return address + 9u;
    }
};

void TestCompare() {
    pal4::PropertyStringPair::Storage left{};
    pal4::PropertyStringPair::Storage right{};
    left[1] = 0x1000u; left[2] = 3u;
    left[5] = 0x2000u; left[6] = 4u;
    right[2] = 0x3000u; right[3] = 3u;
    right[6] = 0x4000u; right[7] = 4u;
    Runtime runtime;
    Check(pal4::PropertyStringPair::Compare(left, right, runtime),
          "both equal fields compare true");
    Check(runtime.calls == std::vector<std::string>{
              "compare:12288:4096:3", "compare:16384:8192:4"},
          "comparison uses asymmetric record offsets and order");

    runtime.calls.clear();
    left[1] = 0u; left[2] = 0u; right[3] = 1u;
    Check(!pal4::PropertyStringPair::Compare(left, right, runtime),
          "different first lengths compare false");
    Check(runtime.calls == std::vector<std::string>{
              "compare:12288:8657252:0"},
          "zero-length comparison still calls byte comparator with empty literal");

    runtime.calls.clear();
    left[1] = 0x1000u; left[2] = 3u; right[3] = 3u;
    runtime.comparisons = {1};
    Check(!pal4::PropertyStringPair::Compare(left, right, runtime),
          "first field mismatch stops comparison");
    Check(runtime.calls.size() == 1u, "second field is skipped on first mismatch");

    runtime.calls.clear();
    runtime.comparisons = {0, 1};
    runtime.comparison_index = 0u;
    Check(!pal4::PropertyStringPair::Compare(left, right, runtime),
          "second field mismatch compares false");
    Check(runtime.calls.size() == 2u, "both fields reached on second mismatch");

    runtime.calls.clear();
    runtime.comparisons.clear();
    runtime.comparison_index = 0u;
    right[7] = 6u;
    Check(!pal4::PropertyStringPair::Compare(left, right, runtime),
          "second field length mismatch compares false");
    Check(runtime.calls.back() == "compare:16384:8192:4",
          "second field compares minimum length before length test");
}

void TestGetterAndCleanup() {
    pal4::PropertyStringPair::Storage object{};
    Runtime runtime;
    object[0] = 0x1111u; object[4] = 0x2222u;
    object[1] = 0x2000u; object[2] = 5u; object[3] = 6u;
    object[5] = 0x3000u; object[6] = 7u; object[7] = 8u;
    Check(pal4::PropertyStringPair::ItemProperty(object) == 0x3000u,
          "getter reads object offset 0x14");
    runtime.first_count = 2u;
    runtime.second_count = 0xffu;
    Check(pal4::PropertyStringPair::Cleanup(object, runtime) == 0x1fffu,
          "decrement branch returns first header address");
    Check(runtime.calls == std::vector<std::string>{
              "read:12287", "free:12287", "read:8191", "write:8191:1"},
          "second string is released before first string");
    Check(object == pal4::PropertyStringPair::Storage{
              0x1111u, 0u, 0u, 0u, 0x2222u, 0u, 0u, 0u},
          "cleanup clears two string triples and preserves other words");

    runtime.calls.clear();
    object[1] = 0x2000u; object[5] = 0x3000u;
    runtime.first_count = 0u;
    runtime.second_count = 3u;
    Check(pal4::PropertyStringPair::Cleanup(object, runtime) == 0x2008u,
          "free branch returns allocator result");
    Check(runtime.calls == std::vector<std::string>{
              "read:12287", "write:12287:2", "read:8191", "free:8191"},
          "count zero frees and positive count decrements");

    runtime.calls.clear();
    Check(pal4::PropertyStringPair::Cleanup(object, runtime) == 0u,
          "null fields return zero");
    Check(runtime.calls.empty(), "null fields need no runtime calls");
}

}  // namespace

int main() {
    TestCompare();
    TestGetterAndCleanup();
}
