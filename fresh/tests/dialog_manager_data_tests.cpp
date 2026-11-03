#include "pal4/dialog_manager_data.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct EventRuntime final : pal4::IDialogManagerEventRuntime {
    std::unordered_map<std::uint32_t, std::array<std::uint32_t, 4>> nodes;
    std::vector<std::string> calls;
    std::uint32_t predecessor = 0;
    std::uint8_t predecessor_result = 0x33;
    std::uint8_t insert_result = 0x5a;

    std::uint32_t NilNode() override { return 0x9000; }
    std::uint32_t NodeWord(std::uint32_t node,
                           std::size_t offset) override {
        Check(offset <= 12 && offset % 4 == 0, "tree node offset");
        return nodes.at(node)[offset / 4];
    }
    std::uint8_t FindPredecessor(std::uint32_t& node) override {
        calls.emplace_back("predecessor");
        node = predecessor;
        return predecessor_result;
    }
    std::uint8_t Insert(pal4::DialogManagerState&,
                        std::uint32_t leaf, std::uint32_t parent,
                        std::uint32_t key) override {
        Check(leaf == 0x9000, "insertion reaches nil leaf");
        calls.emplace_back("insert:" + std::to_string(parent) + ":" +
                           std::to_string(key));
        return insert_result;
    }
};

struct StoryRuntime final : pal4::IDialogManagerStatRuntime {
    std::array<std::int32_t, 3> source{10, -3, 20};
    std::array<std::int32_t, 3> stored{5, 0, 20};
    std::vector<std::string> calls;
    bool has_list = true;
    std::vector<pal4::DialogStatCandidate> candidates;

    std::uint32_t GameInstance() override {
        calls.emplace_back("game");
        return 0x4000;
    }
    std::uint32_t GameStoryBegin(std::uint32_t game) override {
        Check(game == 0x4154, "game story field offset");
        calls.emplace_back("begin");
        return has_list ? 0x7000 : 0;
    }
    std::uint32_t GameStoryEnd(std::uint32_t game) override {
        Check(game == 0x4154, "game story end field");
        return 0x700c;
    }
    std::uint32_t GameStoryObject(std::uint32_t address) override {
        Check(address >= 0x7000 && address < 0x700c &&
                  (address - 0x7000) % 4 == 0,
              "source object pointer stride");
        return 0x8000 + address - 0x7000;
    }
    std::int32_t ObjectStat(std::uint32_t object) override {
        return source.at((object - 0x8000) / 4);
    }
    std::int32_t StoryStat(std::uint32_t address) override {
        return stored.at((address - 0x6000) / 4);
    }
    void SetStoryStat(std::uint32_t address,
                      std::int32_t value) override {
        stored.at((address - 0x6000) / 4) = value;
        calls.emplace_back("set:" + std::to_string(address));
    }
    std::vector<pal4::DialogStatCandidate>
    SortedCandidatesFromGameDb() override {
        calls.emplace_back("candidates");
        return candidates;
    }
};

void TestNormalizedStat() {
    pal4::DialogManagerState state;
    state.SetWord(128, 123);
    const auto factor = std::bit_cast<float>(0x3c053408u);
    Check(pal4::DialogManagerData::NormalizedStat(state) ==
              123.0 * static_cast<double>(factor),
          "stat uses original float constant and word at offset 128");
    state.SetWord(128, 0xffffffff);
    Check(pal4::DialogManagerData::NormalizedStat(state) > 0.0,
          "fild input is unsigned-extended into a positive 64-bit integer");
}

void TestEventInsertBranches() {
    pal4::DialogManagerState state;
    state.SetWord(120, 0x1000);
    EventRuntime runtime;
    runtime.nodes[0x1000] = {0x1000, 0x9000, 0, 0};
    state.SetByte(124, 1);
    Check(pal4::DialogManagerData::InsertEvent(state, 12, runtime) == 0x5a &&
              runtime.calls ==
                  std::vector<std::string>{"insert:4096:12"},
          "tree flag inserts into empty tree");
    state.SetByte(124, 0);
    runtime.calls.clear();
    Check(pal4::DialogManagerData::InsertEvent(state, 13, runtime) == 0x5a,
          "empty ordinary tree inserts through minimum-head branch");

    runtime.nodes[0x1000] = {0x2000, 0x2000, 0, 0};
    runtime.nodes[0x2000] = {0x9000, 0, 0x9000, 10};
    runtime.calls.clear();
    Check(pal4::DialogManagerData::InsertEvent(state, 10, runtime) == 0 &&
              runtime.calls.empty(),
          "duplicate key is rejected without allocation");
    Check(pal4::DialogManagerData::InsertEvent(state, 11, runtime) == 0x5a &&
              runtime.calls.back() == "insert:8192:11",
          "larger key inserts to the right");
    runtime.calls.clear();
    Check(pal4::DialogManagerData::InsertEvent(state, 5, runtime) == 0x5a &&
              runtime.calls.back() == "insert:8192:5",
          "smaller-than-minimum key inserts to the left");

    runtime.nodes[0x1000] = {0x3000, 0x2000, 0, 0};
    runtime.nodes[0x2000] = {0x3000, 0, 0x4000, 10};
    runtime.nodes[0x3000] = {0x9000, 0, 0x4000, 7};
    runtime.nodes[0x4000] = {0x9000, 0, 0x9000, 9};
    runtime.predecessor = 0x3000;
    runtime.calls.clear();
    Check(pal4::DialogManagerData::InsertEvent(state, 8, runtime) == 0x5a &&
              runtime.calls == std::vector<std::string>{
                  "predecessor", "insert:16384:8"},
          "interior left gap checks predecessor before insertion");
    runtime.nodes[0x3000][3] = 8;
    runtime.calls.clear();
    Check(pal4::DialogManagerData::InsertEvent(state, 8, runtime) == 0x33 &&
              runtime.calls == std::vector<std::string>{"predecessor"},
          "predecessor gate forwards helper result without insertion");
}

void TestStoryMaximumMerge() {
    pal4::DialogManagerState state;
    state.SetWord(184, 0x6000);
    StoryRuntime runtime;
    Check(pal4::DialogManagerData::MergeStoryMax(state, runtime) == 0x4154 &&
              runtime.stored == std::array<std::int32_t, 3>{10, 0, 20} &&
              runtime.calls == std::vector<std::string>{
                  "game", "begin", "set:24576", "begin", "begin",
                  "begin"},
          "story merge uses signed maximum and original index stride");
    StoryRuntime missing;
    missing.has_list = false;
    Check(pal4::DialogManagerData::MergeStoryMax(state, missing) == 0x4154 &&
              missing.stored ==
                  std::array<std::int32_t, 3>{5, 0, 20} &&
              missing.calls == std::vector<std::string>{"game", "begin"},
          "missing game list leaves story values unchanged");
}

void TestStatCandidateSelection() {
    pal4::DialogManagerState state;
    state.SetWord(184, 0x6000);
    state.SetWord(188, 0x600c);
    StoryRuntime runtime;
    runtime.candidates = {
        {0, 3, 1.25f}, {0, 10, 2.5f}, {0, 11, 99.0f},
        {0, 10, 77.0f}, {1, -1, 3.0f}, {1, 0, 4.0f},
        {2, 20, 5.0f}, {3, 0, 99.0f}, {-1, 0, 99.0f}};
    Check(pal4::DialogManagerData::CalculateStatValue(state, runtime) ==
              11.5 &&
              runtime.stored == std::array<std::int32_t, 3>{10, 0, 20} &&
              runtime.calls.back() == "candidates",
          "highest eligible threshold wins per story slot");

    StoryRuntime fractional;
    fractional.has_list = false;
    fractional.candidates = {{0, 5, 0.1f}, {1, 0, 0.2f}};
    const double expected = static_cast<float>(
        static_cast<double>(0.1f) + static_cast<double>(0.2f));
    Check(pal4::DialogManagerData::CalculateStatValue(state, fractional) ==
              expected,
          "x87 sum rounds to float before returning double");
    state.SetWord(184, 0);
    Check(pal4::DialogManagerData::CalculateStatValue(state, fractional) ==
              0.0,
          "missing story array yields zero regardless of candidates");
}

}  // namespace

int main() {
    try {
        TestNormalizedStat();
        TestEventInsertBranches();
        TestStoryMaximumMerge();
        TestStatCandidateSelection();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
