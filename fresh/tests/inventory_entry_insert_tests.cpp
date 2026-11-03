#include "pal4/inventory_entry_processor.h"

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

struct Runtime final : pal4::IInventoryEntryInsertRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5180u, 77u}, {0x6000u, 0xaabbccddu},
        {0x600cu, 0xffffffffu}};
    std::vector<std::string> calls;
    bool contains = true;
    std::uint32_t allocation = 0x9000u;

    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t GetGameDatabase() override {
        calls.push_back("database");
        return 0x2000u;
    }
    bool ContainsDatabaseKey(std::uint32_t array_address,
                              std::uint32_t key) override {
        calls.push_back("contains:" + std::to_string(array_address) +
                        ":" + std::to_string(key));
        return contains;
    }
    std::uint32_t LookupDatabaseKey(std::uint32_t array_address,
                                    std::uint32_t key) override {
        calls.push_back("lookup:" + std::to_string(array_address) + ":" +
                        std::to_string(key));
        return 0x6000u;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("alloc:" + std::to_string(bytes));
        return allocation;
    }
    void CopyRange(std::uint32_t begin, std::uint32_t end,
                   std::uint32_t destination) override {
        calls.push_back("copy:" + std::to_string(begin) + ":" +
                        std::to_string(end) + ":" +
                        std::to_string(destination));
        for (auto source = begin; source != end; source += 4u) {
            if (destination != 0u)
                words[destination] = words.at(source);
            destination += 4u;
        }
    }
    void FillOne(std::uint32_t destination,
                 std::uint32_t value) override {
        calls.push_back("fill:" + std::to_string(destination) + ":" +
                        std::to_string(value));
        if (destination != 0u) words[destination] = value;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
};

void TestGates() {
    pal4::InventoryEntryArrayState state{0u, 0x7000u,
                                         0x7000u, 0x7004u};
    Runtime runtime;
    pal4::InventoryEntryProcessor::ProcessAndInsert(state, 0u, runtime);
    Check(runtime.calls.empty() && state.end == 0x7000u,
          "null entry exits before database access");

    runtime.contains = false;
    pal4::InventoryEntryProcessor::ProcessAndInsert(
        state, 0x5000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "read:20864", "database", "contains:8204:77"} &&
              state.end == 0x7000u,
          "first database test rejects missing keys");

    runtime.calls.clear();
    runtime.contains = true;
    runtime.words[0x600cu] = 0u;
    pal4::InventoryEntryProcessor::ProcessAndInsert(
        state, 0x5000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "read:20864", "database", "contains:8204:77",
              "read:20864", "database", "lookup:8204:77",
              "read:24588"} && state.end == 0x7000u,
          "non-sentinel database record does not enter insertion logic");
}

void TestSpareCapacity() {
    pal4::InventoryEntryArrayState state{0x33u, 0x7000u,
                                         0x7008u, 0x7010u};
    Runtime runtime;
    runtime.words[0x7000u] = 11u;
    runtime.words[0x7004u] = 22u;
    pal4::InventoryEntryProcessor::ProcessAndInsert(
        state, 0x5000u, runtime);
    Check(state.begin == 0x7000u && state.end == 0x700cu &&
              state.capacity == 0x7010u &&
              runtime.words[0x7000u] == 11u &&
              runtime.words[0x7004u] == 22u &&
              runtime.words[0x7008u] == 0xaabbccddu &&
              runtime.calls == std::vector<std::string>{
                  "read:20864", "database", "contains:8204:77",
                  "read:20864", "database", "lookup:8204:77",
                  "read:24588", "read:24576",
                  "copy:28680:28680:28684", "fill:28680:2864434397"},
          "spare-capacity path appends value without allocation");
}

void TestReallocation() {
    pal4::InventoryEntryArrayState empty{};
    Runtime runtime;
    pal4::InventoryEntryProcessor::ProcessAndInsert(
        empty, 0x5000u, runtime);
    Check(empty.begin == 0x9000u && empty.end == 0x9004u &&
              empty.capacity == 0x9004u &&
              runtime.words[0x9000u] == 0xaabbccddu &&
              runtime.calls[8] == "alloc:4" &&
              runtime.calls[9] == "copy:0:0:36864" &&
              runtime.calls[10] == "fill:36864:2864434397" &&
              runtime.calls[11] == "copy:0:0:36868" &&
              runtime.calls[12] == "free:0",
          "empty collection allocates one slot and still frees old null base");

    pal4::InventoryEntryArrayState populated{
        0x44u, 0x7000u, 0x7008u, 0x7008u};
    Runtime second;
    second.words[0x7000u] = 11u;
    second.words[0x7004u] = 22u;
    pal4::InventoryEntryProcessor::ProcessAndInsert(
        populated, 0x5000u, second);
    Check(populated.begin == 0x9000u &&
              populated.end == 0x900cu &&
              populated.capacity == 0x9010u &&
              second.words[0x9000u] == 11u &&
              second.words[0x9004u] == 22u &&
              second.words[0x9008u] == 0xaabbccddu &&
              second.calls[8] == "alloc:16" &&
              second.calls[9] == "copy:28672:28680:36864" &&
              second.calls[10] == "fill:36872:2864434397" &&
              second.calls[11] == "copy:28680:28680:36876" &&
              second.calls[12] == "free:28672",
          "full collection doubles capacity and preserves old entries");
}

}  // namespace

int main() {
    TestGates();
    TestSpareCapacity();
    TestReallocation();
    pal4::InventoryEntryArrayState state;
    pal4::InventoryEntryInsertRuntimeStub stub;
    pal4::InventoryEntryProcessor::ProcessAndInsert(state, 0u, stub);
    std::cout << "4 inventory entry insertion cases passed\n";
}
