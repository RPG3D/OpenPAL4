#include "pal4/record_array_copier.h"

#include <cstdlib>
#include <iostream>
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

struct Runtime final : pal4::RecordArrayCopyRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t allocation{0x9000u};
    std::uint32_t allocated_size{};

    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocated_size = bytes;
        events.push_back("allocate");
        return allocation;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
};

void TestTriples() {
    Runtime runtime;
    pal4::RecordArrayState source{0x123456a5u, 0x7000u,
                                   0x7018u, 0x7040u};
    pal4::RecordArrayState destination{0xabcdef01u, 1u, 2u, 3u};
    for (std::uint32_t i = 0u; i < 6u; ++i)
        runtime.words[0x7000u + i * 4u] = 100u + i;
    Check(&pal4::RecordArrayCopier::CopyTriples(destination, source,
                                               runtime) == &destination,
          "triple copy returns destination");
    Check(destination.marker == 0xabcdefa5u &&
              destination.begin == 0x9000u &&
              destination.end == 0x9018u &&
              destination.capacity_end == 0x9018u &&
              runtime.allocated_size == 24u,
          "triple record metadata and exact allocation size");
    for (std::uint32_t i = 0u; i < 6u; ++i)
        Check(runtime.words[0x9000u + i * 4u] == 100u + i,
              "all triple words copied");
}

void TestPairs() {
    Runtime runtime;
    pal4::RecordArrayState source{0x33u, 0x7100u, 0x7110u, 0u};
    pal4::RecordArrayState destination{};
    for (std::uint32_t i = 0u; i < 4u; ++i)
        runtime.words[0x7100u + i * 4u] = 200u + i;
    Check(&pal4::RecordArrayCopier::CopyPairs(destination, source,
                                             runtime) == &destination &&
              runtime.allocated_size == 16u &&
              destination.end == 0x9010u,
          "pair copy allocates two eight-byte records");
    for (std::uint32_t i = 0u; i < 4u; ++i)
        Check(runtime.words[0x9000u + i * 4u] == 200u + i,
              "all pair words copied");
}

void TestEmptyAndNullAllocation() {
    Runtime empty;
    pal4::RecordArrayState none{0x5au, 0u, 0u, 0u};
    pal4::RecordArrayState destination{};
    static_cast<void>(pal4::RecordArrayCopier::CopyPairs(destination, none,
                                                          empty));
    Check(empty.allocated_size == 0u &&
              destination.begin == 0x9000u &&
              destination.end == 0x9000u,
          "empty source still allocates zero bytes and uses returned pointer");

    Runtime failed;
    failed.allocation = 0u;
    pal4::RecordArrayState one{0x7bu, 0x7200u, 0x720cu, 0u};
    destination = {};
    static_cast<void>(pal4::RecordArrayCopier::CopyTriples(destination,
                                                            one, failed));
    Check(failed.allocated_size == 12u &&
              destination.begin == 0u && destination.end == 12u &&
              destination.capacity_end == 12u &&
              failed.events.size() == 1u,
          "null allocation skips first element reads and writes but advances end");
}

}  // namespace

int main() {
    TestTriples();
    TestPairs();
    TestEmptyAndNullAllocation();
}
