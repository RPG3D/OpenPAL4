#include "pal4/indexed_sorted_value_arrays.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IIndexedFilteredValueRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> records;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::uint32_t next_allocation = 0x9000u;
    pal4::IndexedSortedValueArrayState* source{};
    bool shorten_first_source{};
    pal4::FixedSortedValueArrayState* fixed_source{};
    bool shorten_fixed_source{};

    std::uint32_t GetGameDatabase() override {
        calls.push_back("database");
        return 0x4000u;
    }
    std::uint32_t LookupRecord(std::uint32_t collection,
                               std::uint32_t key) override {
        calls.push_back("lookup:" + std::to_string(collection) + ":" +
                        std::to_string(key));
        if (shorten_first_source)
            source->slots[0].end = source->slots[0].begin + 4u;
        if (shorten_fixed_source)
            fixed_source->values.end = fixed_source->values.begin + 4u;
        return records[key];
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("alloc:" + std::to_string(bytes));
        allocations.push_back(bytes);
        const auto result = next_allocation;
        next_allocation += 0x1000u;
        return result;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        frees.push_back(address);
    }
};

using Collector = pal4::IndexedFilteredValueCollector;
using Source = pal4::IndexedSortedValueArrayState;
using Result = pal4::IndexedFilteredResultArrayState;

void TestEmptyAndRejectedSource() {
    Source source;
    Result result{0x55u, 0x8000u, 0x8004u, 0x800cu};
    Runtime runtime;
    Check(Collector::Collect(source, result, 7u, runtime) == 0u &&
              result.end == 0x8004u && runtime.calls.empty(),
          "six empty source slots do not access database or result");

    source.slots[0] = {0u, 0x1000u, 0x1008u, 0x1008u};
    runtime.words[0x1000u] = 1u;
    runtime.words[0x1004u] = 2u;
    runtime.records[1u] = 0u;
    runtime.records[2u] = 0x5000u;
    runtime.words[0x5008u] = 8u;
    Check(Collector::Collect(source, result, 7u, runtime) == 0u &&
              result.end == 0x8004u &&
              runtime.calls == std::vector<std::string>{
                  "database", "read:4096", "lookup:16396:1",
                  "database", "read:4100", "lookup:16396:2",
                  "read:20488"} && runtime.allocations.empty(),
          "null and wrong-attribute records are both rejected");
}

void TestSpareCapacityAndDynamicEnd() {
    Source source;
    source.slots[0] = {0u, 0x1000u, 0x1008u, 0x1008u};
    Result result{0x77u, 0x8000u, 0x8004u, 0x800cu};
    Runtime runtime;
    runtime.source = &source;
    runtime.shorten_first_source = true;
    runtime.words = {{0x1000u, 1u}, {0x1004u, 2u},
                     {0x5008u, 7u}, {0x8000u, 0xaaaau}};
    runtime.records[1u] = 0x5000u;
    Check(Collector::Collect(source, result, 7u, runtime) == 0u &&
              source.slots[0].end == 0x1004u &&
              result.reserved == 0x77u && result.end == 0x8008u &&
              result.capacity_end == 0x800cu &&
              runtime.words[0x8000u] == 0xaaaau &&
              runtime.words[0x8004u] == 0x5000u &&
              runtime.allocations.empty() && runtime.frees.empty(),
          "source end reload and spare-capacity append preserve existing result");
}

void TestCrossSlotGrowth() {
    Source source;
    source.slots[0] = {0u, 0x1000u, 0x100cu, 0x100cu};
    source.slots[5] = {0u, 0x2000u, 0x2004u, 0x2004u};
    Result result{};
    Runtime runtime;
    runtime.words = {{0x1000u, 1u}, {0x1004u, 2u},
                     {0x1008u, 3u}, {0x2000u, 4u},
                     {0x5008u, 7u}, {0x6008u, 8u}, {0x7008u, 7u}};
    runtime.records = {{1u, 0x5000u}, {2u, 0u},
                       {3u, 0x6000u}, {4u, 0x7000u}};
    Check(Collector::Collect(source, result, 7u, runtime) == 0u &&
              result.begin == 0xa000u && result.end == 0xa008u &&
              result.capacity_end == 0xa008u &&
              runtime.words[0xa000u] == 0x5000u &&
              runtime.words[0xa004u] == 0x7000u &&
              runtime.allocations == std::vector<std::uint32_t>{4u, 8u} &&
              runtime.frees == std::vector<std::uint32_t>{0u, 0x9000u},
          "matching records across source slots grow result from zero to two");
}

void TestFixedSourceGatesAndCapacity() {
    pal4::FixedSortedValueArrayState source;
    source.values = {0x55u, 0x1000u, 0x1000u, 0x100cu};
    Result result{0x66u, 0x8000u, 0x8004u, 0x800cu};
    Runtime runtime;
    Check(pal4::FixedFilteredValueCollector::Collect(
              source, result, 7u, runtime) == 0x1000u &&
              runtime.calls.empty() && result.end == 0x8004u,
          "empty fixed source returns its end without GameDB calls");
    source.values.end = 0x100cu;
    runtime.words = {{0x1000u, 1u}, {0x1004u, 2u},
                     {0x1008u, 3u}, {0x5004u, 8u},
                     {0x6004u, 7u}, {0x8000u, 0xaaaau}};
    runtime.records = {{1u, 0u}, {2u, 0x5000u}, {3u, 0x6000u}};
    Check(pal4::FixedFilteredValueCollector::Collect(
              source, result, 7u, runtime) == 0x100cu &&
              result.reserved == 0x66u && result.end == 0x8008u &&
              runtime.words[0x8000u] == 0xaaaau &&
              runtime.words[0x8004u] == 0x6000u &&
              runtime.allocations.empty() && runtime.frees.empty(),
          "fixed source matches record +4 and appends in spare capacity");
}

void TestFixedSourceGrowthAndReload() {
    pal4::FixedSortedValueArrayState source;
    source.values = {0u, 0x1000u, 0x100cu, 0x100cu};
    Result result;
    Runtime runtime;
    runtime.words = {{0x1000u, 1u}, {0x1004u, 2u},
                     {0x1008u, 3u}, {0x5004u, 7u},
                     {0x6004u, 7u}, {0x7004u, 8u}};
    runtime.records = {{1u, 0x5000u}, {2u, 0x6000u},
                       {3u, 0x7000u}};
    Check(pal4::FixedFilteredValueCollector::Collect(
              source, result, 7u, runtime) == 0x100cu &&
              result.begin == 0xa000u && result.end == 0xa008u &&
              result.capacity_end == 0xa008u &&
              runtime.words[0xa000u] == 0x5000u &&
              runtime.words[0xa004u] == 0x6000u &&
              runtime.allocations == std::vector<std::uint32_t>{4u, 8u} &&
              runtime.frees == std::vector<std::uint32_t>{0u, 0x9000u},
          "fixed source grows result through two matching records");

    result = {};
    runtime = {};
    runtime.fixed_source = &source;
    runtime.shorten_fixed_source = true;
    runtime.words = {{0x1000u, 1u}, {0x1004u, 2u},
                     {0x5004u, 7u}};
    runtime.records[1u] = 0x5000u;
    Check(pal4::FixedFilteredValueCollector::Collect(
              source, result, 7u, runtime) == 0x1004u &&
              result.end == 0x9004u &&
              runtime.calls[0] == "database" &&
              runtime.calls[1] == "read:4096" &&
              runtime.calls[2] == "lookup:16396:1",
          "fixed source reloads end after a callback mutates the range");
}

}  // namespace

int main() {
    TestEmptyAndRejectedSource();
    TestSpareCapacityAndDynamicEnd();
    TestCrossSlotGrowth();
    TestFixedSourceGatesAndCapacity();
    TestFixedSourceGrowthAndReload();
    Source source;
    Result result;
    pal4::IndexedFilteredValueRuntimeStub stub;
    Check(Collector::Collect(source, result, 0u, stub) == 0u,
          "default filtered-value runtime links");
    std::cout << "6 indexed filtered value collector cases passed\n";
}
