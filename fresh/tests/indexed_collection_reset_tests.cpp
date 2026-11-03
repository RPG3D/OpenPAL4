#include "pal4/indexed_collection_reset.h"

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

void TestClearFirstFive() {
    pal4::IndexedCollectionResetState state;
    for (std::size_t i = 0; i < 5u; ++i) {
        const auto offset = static_cast<std::uint32_t>(i) * 0x100u;
        state.values[i] = {0x11u + offset, 0x1000u + offset,
                           0x1008u + offset, 0x1010u + offset};
        state.key_values[i] = {0x22u + offset, 0x2000u + offset,
                               0x2010u + offset, 0x2020u + offset};
    }
    state.key_values[5] = {0x33u, 0x3000u, 0x3018u, 0x3020u};
    Check(pal4::IndexedCollectionReset::ClearFirstFive(state) == 0x2410u,
          "clear returns the prior end of the fifth key/value slot");
    for (std::size_t i = 0; i < 5u; ++i) {
        const auto offset = static_cast<std::uint32_t>(i) * 0x100u;
        Check(state.values[i].reserved == 0x11u + offset &&
                  state.values[i].begin == 0x1000u + offset &&
                  state.values[i].end == state.values[i].begin &&
                  state.values[i].capacity_end == 0x1010u + offset &&
                  state.key_values[i].reserved == 0x22u + offset &&
                  state.key_values[i].begin == 0x2000u + offset &&
                  state.key_values[i].end == state.key_values[i].begin &&
                  state.key_values[i].capacity_end == 0x2020u + offset,
              "clear resets only the ten end pointers");
    }
    Check(state.key_values[5].reserved == 0x33u &&
              state.key_values[5].begin == 0x3000u &&
              state.key_values[5].end == 0x3018u &&
              state.key_values[5].capacity_end == 0x3020u,
          "sixth key/value slot is outside the five-iteration loop");
    Check(pal4::IndexedCollectionReset::ClearFirstFive(state) == 0x2400u,
          "second clear returns the fifth key/value begin pointer");
}

struct Runtime final : pal4::ISortedValueProcessRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    pal4::IndexedCollectionResetState* state{};
    bool shorten_first{};

    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void Process(std::uint32_t context, std::uint32_t value) override {
        calls.push_back("process:" + std::to_string(context) + ":" +
                        std::to_string(value));
        if (shorten_first)
            state->values[0].end = state->values[0].begin + 4u;
    }
};

void TestPresenceAndProcessing() {
    pal4::IndexedCollectionResetState state;
    Runtime runtime;
    Check(!pal4::IndexedCollectionReset::HasAnyValues(state) &&
              pal4::IndexedCollectionReset::ProcessValues(
                  state, 0x3000u, runtime) == 0u && runtime.calls.empty(),
          "five empty slots are absent and perform no callbacks");
    state.values[0] = {0u, 0x1000u, 0x1000u, 0x100cu};
    state.key_values[5] = {0u, 0x5000u, 0x5008u, 0x5010u};
    Check(!pal4::IndexedCollectionReset::HasAnyValues(state),
          "empty first slot and nonempty sixth pair slot do not count");
    state.values[0].end = 0x1008u;
    state.values[4] = {0u, 0x4000u, 0x4004u, 0x4008u};
    runtime.words = {{0x1000u, 11u}, {0x1004u, 12u}, {0x4000u, 99u}};
    Check(pal4::IndexedCollectionReset::HasAnyValues(state) &&
              pal4::IndexedCollectionReset::ProcessValues(
                  state, 0x3000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:4096", "process:12288:11", "read:4100",
                  "process:12288:12", "read:16384", "process:12288:99"},
          "processing visits values in slot and element order");
    runtime.calls.clear();
    runtime.state = &state;
    runtime.shorten_first = true;
    Check(pal4::IndexedCollectionReset::ProcessValues(
              state, 0x3000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:4096", "process:12288:11",
                  "read:16384", "process:12288:99"},
          "processor reloads the current end after a callback");
    pal4::SortedValueProcessRuntimeStub stub;
    pal4::IndexedCollectionResetState empty;
    Check(pal4::IndexedCollectionReset::ProcessValues(
              empty, 0u, stub) == 0u,
          "default collection processor links");
}

}  // namespace

int main() {
    TestClearFirstFive();
    TestPresenceAndProcessing();
    std::cout << "2 indexed collection reset cases passed\n";
}
