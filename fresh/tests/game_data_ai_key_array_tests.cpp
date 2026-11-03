#include "pal4/game_data_ai_key_array.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataAiKeyArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, pal4::GameDataAiKeyEntry> keys;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x6000;

    Runtime() {
        words[0x1004] = 0x2000;
        words[0x1008] = 0x2018;
        words[0x100c] = 0x2020;
        keys[0x2000] = {1, 10};
        keys[0x2008] = {2, 20};
        keys[0x2010] = {3, 30};
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    pal4::GameDataAiKeyEntry ReadKey(std::uint32_t address) override {
        return keys.at(address);
    }
    void WriteKey(std::uint32_t address,
                  const pal4::GameDataAiKeyEntry& entry) override {
        keys[address] = entry;
        calls.push_back("key " + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckInPlace() {
    Runtime middle;
    Check(pal4::GameDataAiKeyArray::Insert(
              0x1000, 0x2008, {9, 99}, middle) == 0x2008 &&
              middle.words[0x1008] == 0x2020 &&
              middle.keys[0x2000].key == 1 &&
              middle.keys[0x2008].key == 9 &&
              middle.keys[0x2008].record_index == 99 &&
              middle.keys[0x2010].key == 2 &&
              middle.keys[0x2018].key == 3 &&
              middle.calls.front() == "key 8216" &&
              middle.calls.back() == "word 4104",
          "free capacity shifts the suffix backward and inserts in place");

    Runtime end;
    Check(pal4::GameDataAiKeyArray::Insert(0x1000, 0x2018, {4, 40}, end) ==
              0x2018 &&
              end.keys[0x2018].key == 4 && end.calls.size() == 2,
          "end insertion writes one pair without shifting");
}

void CheckGrowth() {
    Runtime middle;
    middle.words[0x100c] = 0x2018;
    Check(pal4::GameDataAiKeyArray::Insert(
              0x1000, 0x2008, {9, 99}, middle) == 0x6008 &&
              middle.words[0x1004] == 0x6000 &&
              middle.words[0x1008] == 0x6020 &&
              middle.words[0x100c] == 0x6030 &&
              middle.keys[0x6000].key == 1 &&
              middle.keys[0x6008].key == 9 &&
              middle.keys[0x6010].key == 2 &&
              middle.keys[0x6018].key == 3 &&
              middle.calls.front() == "allocate 48" &&
              At(middle, "key 24584") < At(middle, "free 8192") &&
              middle.calls.back() == "word 4104",
          "full array doubles capacity and copies prefix, new pair and suffix");

    Runtime empty;
    empty.words[0x1004] = empty.words[0x1008] = empty.words[0x100c] = 0;
    empty.keys.clear();
    Check(pal4::GameDataAiKeyArray::Insert(0x1000, 0, {7, 70}, empty) ==
              0x6000 &&
              empty.words[0x1004] == 0x6000 &&
              empty.words[0x1008] == empty.words[0x100c] &&
              empty.words[0x1008] == 0x6008 &&
              empty.keys[0x6000].record_index == 70 &&
              empty.calls.front() == "allocate 8",
          "null array starts with one element of capacity");
}

void CheckAllocationFailure() {
    Runtime runtime;
    runtime.words[0x100c] = 0x2018;
    runtime.allocation = 0;
    try {
        (void)pal4::GameDataAiKeyArray::Insert(0x1000, 0x2008,
                                               {9, 99}, runtime);
        Check(false, "failed allocation must stop insertion");
    } catch (const std::bad_alloc&) {
        Check(runtime.words[0x1004] == 0x2000 &&
                  runtime.words[0x1008] == 0x2018 &&
                  runtime.keys[0x2008].key == 2 &&
                  runtime.calls ==
                      std::vector<std::string>{"allocate 48"},
              "allocation failure preserves the original key array");
    }
}

void CheckCopyRange() {
    Runtime runtime;
    Check(pal4::GameDataAiKeyArray::CopyRange(
              0x2000, 0x2018, 0x8000, runtime) == 0x8018 &&
              runtime.keys[0x8000].key == 1 &&
              runtime.keys[0x8008].record_index == 20 &&
              runtime.keys[0x8010].key == 3,
          "pair range copy preserves both words in each entry");
    Check(pal4::GameDataAiKeyArray::CopyRange(
              0x2000, 0x2000, 0x9000, runtime) == 0x9000,
          "empty pair range returns original destination");
    Check(pal4::GameDataAiKeyArray::CopyRange(
              0x2000, 0x2008, 0, runtime) == 8 &&
              runtime.keys[0].key == 1,
          "key copy has no null destination guard");
}

}  // namespace

int main() {
    try {
        CheckInPlace();
        CheckGrowth();
        CheckAllocationFailure();
        CheckCopyRange();
        std::cout << "game_data_ai_key_array_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
