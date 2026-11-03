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

struct Runtime final : pal4::IInventoryEntryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    pal4::InventoryEntryArrayState* state = nullptr;

    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t UpdateStructMembers(
        std::uint32_t context, std::uint32_t entry) override {
        calls.push_back("update:" + std::to_string(context) + ":" +
                        std::to_string(entry));
        if (state != nullptr) state->end = state->begin + 4u;
        return entry + 7u;
    }
};

void TestConditional() {
    Runtime runtime;
    Check(pal4::InventoryEntryProcessor::ProcessConditional(
              0x1000u, 0x2000u, 0u, runtime) == 0u &&
              runtime.calls.empty(),
          "null entry returns zero without a member update");
    Check(pal4::InventoryEntryProcessor::ProcessConditional(
              0x1000u, 0x2000u, 0x3000u, runtime) == 0x3007u &&
              runtime.calls == std::vector<std::string>{
                  "update:4096:12288"},
          "non-null entry forwards context and returns the raw update value");
}

void TestEmptyAndPopulatedArray() {
    pal4::InventoryEntryArrayState state{0x55u, 0x4000u, 0x4000u};
    Runtime runtime;
    Check(pal4::InventoryEntryProcessor::ProcessArray(
              state, 0x1000u, 0x2000u, runtime) == 0x4000u &&
              runtime.calls.empty(),
          "empty pointer array returns its end without reading entries");

    state.end = 0x400cu;
    runtime.words = {{0x4000u, 0x5000u},
                     {0x4004u, 0u}, {0x4008u, 0x6000u}};
    Check(pal4::InventoryEntryProcessor::ProcessArray(
              state, 0x1000u, 0x2000u, runtime) == 0x400cu &&
              runtime.calls == std::vector<std::string>{
                  "read:16384", "update:4096:20480", "read:16388",
                  "read:16392", "update:4096:24576"},
          "array walks four-byte entries and skips null callbacks");
}

void TestMutableEnd() {
    pal4::InventoryEntryArrayState state{0u, 0x4000u, 0x400cu};
    Runtime runtime;
    runtime.state = &state;
    runtime.words[0x4000u] = 0x5000u;
    Check(pal4::InventoryEntryProcessor::ProcessArray(
              state, 0x1000u, 0u, runtime) == 0x4004u &&
              runtime.calls == std::vector<std::string>{
                  "read:16384", "update:4096:20480"},
          "walker rereads the end pointer after each callback");
}

}  // namespace

int main() {
    TestConditional();
    TestEmptyAndPopulatedArray();
    TestMutableEnd();
    pal4::InventoryEntryArrayState state;
    pal4::InventoryEntryRuntimeStub stub;
    Check(pal4::InventoryEntryProcessor::ProcessArray(
              state, 0u, 0u, stub) == 0u,
          "default entry runtime links");
    std::cout << "4 inventory entry processor cases passed\n";
}
