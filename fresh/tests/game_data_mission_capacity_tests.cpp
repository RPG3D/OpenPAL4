#include "pal4/game_data_mission_capacity.h"

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

struct Runtime final : pal4::IGameDataMissionCapacityRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocation_addresses{0x5000, 0x6000};
    std::size_t allocation_index = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        Check(allocation_index < allocation_addresses.size(),
              "unexpected allocation");
        return allocation_addresses[allocation_index++];
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
    void DestroyRecord(std::uint32_t record) override {
        calls.push_back("destroy " + std::to_string(record));
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call: " + call);
}

Runtime WithTwoEntries() {
    Runtime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2088;
    runtime.words[0x1068] = 0x2088;
    runtime.words[0x2000] = 7;
    runtime.words[0x2044] = 9;
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
    Check(pal4::GameDataMissionCapacity::Reserve(0x1000, 4, runtime) ==
              0x6010,
          "both-array growth returns new used key end");
    Check(runtime.words[0x1060] == 0x5000 &&
              runtime.words[0x1064] == 0x5088 &&
              runtime.words[0x1068] == 0x5110 &&
              runtime.words[0x5000] == 7 && runtime.words[0x5044] == 9,
          "record growth preserves two 68-byte entries and reserves four");
    Check(runtime.words[0x1008] == 0x6000 &&
              runtime.words[0x100c] == 0x6010 &&
              runtime.words[0x1010] == 0x6020 &&
              runtime.words[0x6000] == 7 && runtime.words[0x6004] == 0 &&
              runtime.words[0x6008] == 9 && runtime.words[0x600c] == 1,
          "key growth preserves pairs and reserves four entries");
    Check(At(runtime, "allocate 272") < At(runtime, "copy 20480 8192") &&
              At(runtime, "copy 20548 8260") <
                  At(runtime, "destroy 8192") &&
              At(runtime, "destroy 8260") < At(runtime, "free 8192") &&
              At(runtime, "free 8192") < At(runtime, "allocate 32") &&
              At(runtime, "allocate 32") < At(runtime, "free 12288"),
          "record copy and cleanup complete before key-array allocation");
}

void CheckIndependentGrowth() {
    auto records_only = WithTwoEntries();
    records_only.words[0x1010] = 0x3028;
    Check(pal4::GameDataMissionCapacity::Reserve(0x1000, 4,
                                                  records_only) == 5 &&
              records_only.allocation_index == 1 &&
              records_only.words[0x1008] == 0x3000 &&
              records_only.words[0x100c] == 0x3010,
          "unchanged key array returns capacity count after record growth");

    auto keys_only = WithTwoEntries();
    keys_only.words[0x1068] = 0x2110;
    Check(pal4::GameDataMissionCapacity::Reserve(0x1000, 4,
                                                  keys_only) == 0x5010 &&
              keys_only.allocation_index == 1 &&
              keys_only.calls.front() == "allocate 32" &&
              keys_only.words[0x1060] == 0x2000 &&
              keys_only.words[0x1064] == 0x2088,
          "key growth does not move records with sufficient capacity");

    auto unchanged = WithTwoEntries();
    unchanged.words[0x1068] = 0x2154;
    unchanged.words[0x1010] = 0x3028;
    Check(pal4::GameDataMissionCapacity::Reserve(0x1000, 4,
                                                  unchanged) == 5 &&
              unchanged.calls.empty(),
          "sufficient capacities make no allocations or writes");
}

void CheckEmptyAndNegative() {
    Runtime empty;
    Check(pal4::GameDataMissionCapacity::Reserve(0x1000, 3, empty) ==
              0x6000 &&
              empty.words[0x1060] == 0x5000 &&
              empty.words[0x1064] == 0x5000 &&
              empty.words[0x1068] == 0x50cc &&
              empty.words[0x1008] == 0x6000 &&
              empty.words[0x100c] == 0x6000 &&
              empty.words[0x1010] == 0x6018 &&
              At(empty, "free 0") < At(empty, "allocate 24"),
          "null arrays allocate capacity but retain zero used entries");

    Runtime negative;
    Check(pal4::GameDataMissionCapacity::Reserve(0x1000, -1, negative) ==
              0x6000 &&
              negative.calls[0] == "allocate 0" &&
              At(negative, "allocate 0") < At(negative, "free 0") &&
              negative.words[0x1068] == 0x5000 - 68 &&
              negative.words[0x1010] == 0x6000 - 8,
          "unsigned comparison triggers zero-byte allocation for negative request");
}

}  // namespace

int main() {
    try {
        CheckBothGrow();
        CheckIndependentGrowth();
        CheckEmptyAndNegative();
        std::cout << "game_data_mission_capacity_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
