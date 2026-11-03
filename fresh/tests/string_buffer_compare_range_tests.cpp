#include "pal4/string_buffer.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IStringCompareRangeRuntime {
public:
    void ThrowRange() override {
        events.emplace_back("range");
        throw std::out_of_range("range");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return bytes.at(address);
    }
    void Put(std::uint32_t address, const std::string& value) {
        for (std::size_t i = 0u; i < value.size(); ++i) {
            bytes[address + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(value[i]);
        }
    }
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
};

void CheckEqualityAndClipping() {
    Runtime runtime;
    runtime.Put(0x1000u, "abcde");
    runtime.Put(0x2000u, "bcd");
    pal4::StringBuffer buffer({0x1000u, 5u, 8u});
    Check(buffer.CompareRange(1u, 3u, 0x2000u, 3u, runtime) == 0 &&
              runtime.events.size() == 6u &&
              runtime.events[0] == "read:4097" &&
              runtime.events[1] == "read:8192",
          "equal selected span compares byte pairs from the requested offset");
    runtime.events.clear();
    Check(buffer.CompareRange(1u, 99u, 0x2000u, 3u, runtime) == 1,
          "selected source longer than equal other prefix returns one");
    runtime.events.clear();
    Check(buffer.CompareRange(1u, 2u, 0x2000u, 3u, runtime) == -1,
          "selected source shorter than equal other prefix returns minus one");
}

void CheckDifferingBytes() {
    Runtime runtime;
    runtime.Put(0x1000u, "abc");
    runtime.Put(0x2000u, "abd");
    pal4::StringBuffer buffer({0x1000u, 3u, 8u});
    Check(buffer.CompareRange(0u, 3u, 0x2000u, 3u, runtime) == -1 &&
              runtime.events.size() == 6u,
          "first differing smaller source byte returns minus one");
    runtime.events.clear();
    runtime.bytes[0x1002u] = 'z';
    Check(buffer.CompareRange(0u, 3u, 0x2000u, 3u, runtime) == 1 &&
              runtime.events.size() == 6u,
          "first differing larger source byte returns one");
}

void CheckBoundaryAndNullData() {
    Runtime runtime;
    pal4::StringBuffer buffer({0x1000u, 3u, 8u});
    bool threw = false;
    try {
        (void)buffer.CompareRange(4u, 1u, 0x2000u, 1u, runtime);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<std::string>{"range"},
          "start beyond source length raises range without reading memory");

    runtime.events.clear();
    Check(buffer.CompareRange(3u, 5u, 0x2000u, 0u, runtime) == 0 &&
              runtime.events.empty(),
          "start at end compares as an empty span");
    Check(buffer.CompareRange(3u, 5u, 0x2000u, 1u, runtime) == -1 &&
              runtime.events.empty(),
          "empty span is smaller than nonempty other span");

    runtime.bytes[0u] = 'x';
    runtime.bytes[0x2000u] = 'x';
    pal4::StringBuffer null_data({0u, 1u, 8u});
    Check(null_data.CompareRange(0u, 1u, 0x2000u, 1u, runtime) == 0 &&
              runtime.events[0] == "read:0",
          "null data pointer is compared as address zero without fallback");
}

void CheckHostStringCompareBoundary() {
    struct Case {
        std::string source;
        std::uint32_t start;
        std::uint32_t count;
        std::string other;
    };
    const std::vector<Case> cases{
        {"abcde", 1u, 3u, "bcd"},
        {"abcde", 1u, 99u, "bcd"},
        {"abcde", 1u, 2u, "bcd"},
        {"abc", 0u, 3u, "abd"},
        {"abz", 0u, 3u, "abd"},
        {"abc", 3u, 2u, ""},
        {std::string("a\0b", 3), 0u, 3u, std::string("a\0c", 3)},
        {std::string(1, static_cast<char>(0x80)), 0u, 1u,
         std::string(1, static_cast<char>(0x7f))},
    };
    const auto sign = [](const int value) { return (value > 0) - (value < 0); };
    for (const Case& item : cases) {
        Runtime runtime;
        runtime.Put(0x1000u, item.source);
        runtime.Put(0x2000u, item.other);
        const pal4::StringBuffer buffer({
            0x1000u, static_cast<std::uint32_t>(item.source.size()),
            static_cast<std::uint32_t>(item.source.size())});
        const int observed = buffer.CompareRange(
            item.start, item.count, 0x2000u,
            static_cast<std::uint32_t>(item.other.size()), runtime);
        const int host = item.source.compare(
            item.start, item.count, item.other.data(), item.other.size());
        Check(observed == sign(host),
              "VC6 range comparison agrees with host std::string ordering");
    }

    bool host_threw = false;
    try {
        (void)std::string("abc").compare(4u, 1u, "x", 1u);
    } catch (const std::out_of_range&) {
        host_threw = true;
    }
    Check(host_threw, "host std::string rejects an out-of-range start");
}

}  // namespace

int main() {
    CheckEqualityAndClipping();
    CheckDifferingBytes();
    CheckBoundaryAndNullData();
    CheckHostStringCompareBoundary();
    std::cout << "4 string range comparison test groups passed\n";
}
