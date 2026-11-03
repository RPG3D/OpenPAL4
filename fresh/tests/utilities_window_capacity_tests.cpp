#include "pal4/utilities_window_table.h"

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

struct Runtime final : pal4::IUtilitiesWindowCapacityRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations{0x5000, 0x6000};
    std::size_t next_allocation = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        Check(next_allocation < allocations.size(), "unexpected allocation");
        return allocations[next_allocation++];
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.push_back("copy " + std::to_string(destination) + " " +
                        std::to_string(source));
        for (std::uint32_t i = 0; i < 44; i += 4)
            words[destination + i] = words[source + i];
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
    runtime.words[0x1064] = 0x2058;
    runtime.words[0x1068] = 0x2058;
    runtime.words[0x2000] = 7;
    runtime.words[0x2008] = 0xaaaa;
    runtime.words[0x2018] = 0xbbbb;
    runtime.words[0x202c] = 9;
    runtime.words[0x2034] = 0xcccc;
    runtime.words[0x2044] = 0xdddd;
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
    Check(pal4::UtilitiesWindowTable::Reserve(
              0x1000, 4, runtime) == 0x6010 &&
              runtime.words[0x1060] == 0x5000 &&
              runtime.words[0x1064] == 0x5058 &&
              runtime.words[0x1068] == 0x50b0 &&
              runtime.words[0x5000] == 7 &&
              runtime.words[0x5008] == 0xaaaa &&
              runtime.words[0x5018] == 0xbbbb &&
              runtime.words[0x502c] == 9 &&
              runtime.words[0x5034] == 0xcccc &&
              runtime.words[0x5044] == 0xdddd,
          "record buffer grows to four 44-byte rows and retains texts");
    Check(runtime.words[0x1008] == 0x6000 &&
              runtime.words[0x100c] == 0x6010 &&
              runtime.words[0x1010] == 0x6020 &&
              runtime.words[0x6000] == 7 &&
              runtime.words[0x6004] == 0 &&
              runtime.words[0x6008] == 9 &&
              runtime.words[0x600c] == 1,
          "key buffer grows independently and retains physical indices");
    Check(At(runtime, "allocate 176") <
              At(runtime, "copy 20480 8192") &&
              At(runtime, "copy 20524 8236") <
                  At(runtime, "destroy 8192") &&
              At(runtime, "destroy 8236") < At(runtime, "free 8192") &&
              At(runtime, "free 8192") < At(runtime, "allocate 32") &&
              At(runtime, "allocate 32") < At(runtime, "free 12288"),
          "deep row copies and cleanup precede key growth");
}

void CheckIndependentGrowth() {
    auto records_only = WithTwoEntries();
    records_only.words[0x1010] = 0x3028;
    Check(pal4::UtilitiesWindowTable::Reserve(
              0x1000, 4, records_only) == 5 &&
              records_only.next_allocation == 1 &&
              records_only.words[0x1008] == 0x3000,
          "row growth returns unchanged key capacity");

    auto keys_only = WithTwoEntries();
    keys_only.words[0x1068] = 0x20b0;
    Check(pal4::UtilitiesWindowTable::Reserve(
              0x1000, 4, keys_only) == 0x5010 &&
              keys_only.next_allocation == 1 &&
              keys_only.calls.front() == "allocate 32" &&
              keys_only.words[0x1060] == 0x2000 &&
              keys_only.words[0x1064] == 0x2058,
          "key growth leaves record storage intact");

    auto unchanged = WithTwoEntries();
    unchanged.words[0x1068] = 0x20b0;
    unchanged.words[0x1010] = 0x3028;
    Check(pal4::UtilitiesWindowTable::Reserve(
              0x1000, 4, unchanged) == 5 &&
              unchanged.calls.empty(),
          "sufficient capacities cause no writes or allocations");
}

void CheckEmptyAndNegative() {
    Runtime empty;
    Check(pal4::UtilitiesWindowTable::Reserve(
              0x1000, 3, empty) == 0x6000 &&
              empty.words[0x1060] == 0x5000 &&
              empty.words[0x1064] == 0x5000 &&
              empty.words[0x1068] == 0x5084 &&
              empty.words[0x1008] == 0x6000 &&
              empty.words[0x100c] == 0x6000 &&
              empty.words[0x1010] == 0x6018 &&
              At(empty, "free 0") < At(empty, "allocate 24"),
          "empty table reserves two arrays with zero active entries");

    Runtime negative;
    Check(pal4::UtilitiesWindowTable::Reserve(
              0x1000, -1, negative) == 0x6000 &&
              negative.calls[0] == "allocate 0" &&
              negative.words[0x1068] == 0x5000 - 44 &&
              negative.words[0x1010] == 0x6000 - 8,
          "negative request preserves unsigned capacity comparison");
}

}  // namespace

int main() {
    try {
        CheckBothGrow();
        CheckIndependentGrowth();
        CheckEmptyAndNegative();
        std::cout << "utilities_window_capacity_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
