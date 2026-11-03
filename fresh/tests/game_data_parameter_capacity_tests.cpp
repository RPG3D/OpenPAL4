#include "pal4/game_data_parameter_capacity.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataParameterCapacityRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> destroyed_texts;
    std::vector<std::uint32_t> allocations{0x5000, 0x6000};
    std::size_t allocation_index = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        Check(allocation_index < allocations.size(),
              "unexpected capacity allocation");
        return allocations[allocation_index++];
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.push_back("copy " + std::to_string(destination) + " " +
                        std::to_string(source));
        words[destination] = words[source];
    }
    void DestroyTextRecord(std::uint32_t address) override {
        destroyed_texts.push_back(address);
        calls.push_back("text " + std::to_string(address));
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing operation: " + call);
}

Runtime WithTwoEntries() {
    Runtime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2000 + 2 * 328;
    runtime.words[0x1068] = 0x2000 + 2 * 328;
    runtime.words[0x2000] = 7;
    runtime.words[0x2000 + 328] = 9;
    runtime.words[0x1008] = 0x3000;
    runtime.words[0x100c] = 0x3010;
    runtime.words[0x1010] = 0x3010;
    runtime.words[0x3000] = 7;
    runtime.words[0x3004] = 0;
    runtime.words[0x3008] = 9;
    runtime.words[0x300c] = 1;
    return runtime;
}

void CheckBothGrow() {
    auto runtime = WithTwoEntries();
    Check(pal4::GameDataParameterCapacity::Reserve(0x1000, 4, runtime) ==
              0x6010 &&
              runtime.words[0x1060] == 0x5000 &&
              runtime.words[0x1064] == 0x5000 + 2 * 328 &&
              runtime.words[0x1068] == 0x5000 + 4 * 328 &&
              runtime.words[0x5000] == 7 &&
              runtime.words[0x5000 + 328] == 9,
          "record growth preserves two entries and reserves four");
    Check(runtime.words[0x1008] == 0x6000 &&
              runtime.words[0x100c] == 0x6010 &&
              runtime.words[0x1010] == 0x6020 &&
              runtime.words[0x6000] == 7 &&
              runtime.words[0x6008] == 9,
          "key growth copies pairs and reserves four entries");
    Check(At(runtime, "allocate 1312") <
              At(runtime, "copy 20480 8192") &&
              At(runtime, "copy 20808 8520") <
                  At(runtime, "text " + std::to_string(0x2000 + 84)) &&
              At(runtime, "text " +
                              std::to_string(0x2000 + 328 + 4)) <
                  At(runtime, "free 8192") &&
              At(runtime, "free 8192") < At(runtime, "allocate 32"),
          "deep record copies and text cleanup precede key growth");
    Check(runtime.destroyed_texts.size() == 12,
          "two old records release twelve text objects");
}

void CheckIndependentGrowth() {
    auto records_only = WithTwoEntries();
    records_only.words[0x1010] = 0x3028;
    Check(pal4::GameDataParameterCapacity::Reserve(
              0x1000, 4, records_only) == 5 &&
              records_only.allocation_index == 1 &&
              records_only.words[0x1008] == 0x3000,
          "record-only growth returns existing key capacity");

    auto keys_only = WithTwoEntries();
    keys_only.words[0x1068] = 0x2000 + 5 * 328;
    Check(pal4::GameDataParameterCapacity::Reserve(
              0x1000, 4, keys_only) == 0x5010 &&
              keys_only.allocation_index == 1 &&
              keys_only.calls.front() == "allocate 32" &&
              keys_only.words[0x1060] == 0x2000,
          "key-only growth leaves record storage intact");

    auto unchanged = WithTwoEntries();
    unchanged.words[0x1068] = 0x2000 + 5 * 328;
    unchanged.words[0x1010] = 0x3028;
    Check(pal4::GameDataParameterCapacity::Reserve(
              0x1000, 4, unchanged) == 5 &&
              unchanged.allocation_index == 0,
          "sufficient capacities allocate nothing");
}

void CheckEmptyAndNegative() {
    Runtime empty;
    Check(pal4::GameDataParameterCapacity::Reserve(
              0x1000, 1, empty) == 0x6000 &&
              empty.calls.front() == "allocate 328" &&
              At(empty, "allocate 8") > At(empty, "free 0") &&
              empty.words[0x1060] == 0x5000 &&
              empty.words[0x1064] == 0x5000 &&
              empty.words[0x1008] == 0x6000 &&
              empty.words[0x100c] == 0x6000,
          "empty arrays reserve without copying records");

    Runtime negative;
    (void)pal4::GameDataParameterCapacity::Reserve(0x1000, -1, negative);
    Check(negative.calls.front() == "allocate 0" &&
              negative.allocation_index == 2,
          "negative request makes two zero-byte allocations");
}

}  // namespace

int main() {
    try {
        CheckBothGrow();
        CheckIndependentGrowth();
        CheckEmptyAndNegative();
        std::cout << "game_data_parameter_capacity_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
