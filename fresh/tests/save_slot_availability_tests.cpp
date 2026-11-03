#include "pal4/save_slot_availability.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISaveSlotAvailabilityRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::uint32_t database_calls = 0u;

    Runtime() {
        words[0x1000u + 168u] = 0x2000u;
        words[0x2000u + 96u] = 0x3000u;
        words[0x2000u + 100u] = 0x3000u;
    }

    std::uint32_t GameDatabase() override {
        ++database_calls;
        return 0x1000u;
    }

    std::uint32_t ReadWord(const std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
};

void TestPredicate() {
    Runtime runtime;
    for (const auto [raw, expected] : {
             std::pair{0u, false}, {1u, true},
             {0x7fffffffu, true}, {0x80000000u, false},
             {0xffffffffu, false}}) {
        runtime.words[0x4000u + 404u] = raw;
        Check(pal4::SaveSlotAvailability::IsAvailable(0x4000u, runtime) ==
                  expected,
              "predicate uses signed field at record +404");
    }
    Check(runtime.database_calls == 0u && runtime.reads.size() == 5u,
          "predicate reads only its record field");
}

void TestCount() {
    Runtime runtime;
    constexpr std::uint32_t begin = 0x3000u;
    runtime.words[0x2000u + 100u] = begin + 4u * 596u + 595u;
    runtime.words[begin + 404u] = 1u;
    runtime.words[begin + 596u + 404u] = 0u;
    runtime.words[begin + 2u * 596u + 404u] = 0xffffffffu;
    runtime.words[begin + 3u * 596u + 404u] = 7u;
    Check(pal4::SaveSlotAvailability::CountAvailable(runtime) == 2u,
          "count includes positive slots and ignores incomplete tail");
    Check(runtime.database_calls == 1u, "database accessed once");
    std::vector<std::uint32_t> record_reads;
    for (const auto address : runtime.reads) {
        if (address >= begin + 404u) record_reads.push_back(address);
    }
    Check(record_reads == std::vector<std::uint32_t>{
              begin + 404u, begin + 596u + 404u,
              begin + 2u * 596u + 404u,
              begin + 3u * 596u + 404u},
          "record stride is 596 bytes");
}

void TestEmptyAndInvalidRanges() {
    Runtime runtime;
    Check(pal4::SaveSlotAvailability::CountAvailable(runtime) == 0u,
          "empty range");
    runtime.words[0x2000u + 96u] = 0u;
    runtime.words.erase(0x2000u + 100u);
    Check(pal4::SaveSlotAvailability::CountAvailable(runtime) == 0u,
          "null begin does not read end");
    runtime.words[0x2000u + 96u] = 0x3000u;
    runtime.words[0x2000u + 100u] = 0x2fffu;
    Check(pal4::SaveSlotAvailability::CountAvailable(runtime) == 0u,
          "reversed range cannot become a huge count");
    runtime.words[0x2000u + 100u] = 0x3000u + 595u;
    Check(pal4::SaveSlotAvailability::CountAvailable(runtime) == 0u,
          "partial record is ignored");
}

}  // namespace

int main() {
    TestPredicate();
    TestCount();
    TestEmptyAndInvalidRanges();
    return EXIT_SUCCESS;
}
