#include "pal4/utilities_string_id_table.h"

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

struct Runtime final : pal4::IUtilitiesStringIdCapacityRuntime {
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
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call: " + call);
}

Runtime WithTwoEntries() {
    Runtime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2010;
    runtime.words[0x1068] = 0x2010;
    runtime.words[0x2000] = 7;
    runtime.words[0x2004] = 70;
    runtime.words[0x2008] = 9;
    runtime.words[0x200c] = 90;
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
    Check(pal4::UtilitiesStringIdTable::Reserve(
              0x1000, 4, runtime) == 2 &&
              runtime.words[0x1060] == 0x5000 &&
              runtime.words[0x1064] == 0x5010 &&
              runtime.words[0x1068] == 0x5020 &&
              runtime.words[0x5000] == 7 &&
              runtime.words[0x5004] == 70 &&
              runtime.words[0x5008] == 9 &&
              runtime.words[0x500c] == 90,
          "record array grows to four 8-byte value pairs");
    Check(runtime.words[0x1008] == 0x6000 &&
              runtime.words[0x100c] == 0x6010 &&
              runtime.words[0x1010] == 0x6020 &&
              runtime.words[0x6000] == 7 &&
              runtime.words[0x6004] == 0 &&
              runtime.words[0x6008] == 9 &&
              runtime.words[0x600c] == 1,
          "index array grows independently and retains positions");
    Check(At(runtime, "allocate 32") <
              At(runtime, "write 20480 7") &&
              At(runtime, "write 20492 90") <
                  At(runtime, "free 8192") &&
              At(runtime, "free 8192") <
                  At(runtime, "write 24576 7") &&
              At(runtime, "write 24588 1") <
                  At(runtime, "free 12288"),
          "old values copy before their respective buffers are freed");
}

void CheckIndependentGrowth() {
    auto records_only = WithTwoEntries();
    records_only.words[0x1010] = 0x3028;
    Check(pal4::UtilitiesStringIdTable::Reserve(
              0x1000, 4, records_only) == 5 &&
              records_only.next_allocation == 1 &&
              records_only.words[0x1008] == 0x3000,
          "record-only growth returns unchanged index capacity");

    auto index_only = WithTwoEntries();
    index_only.words[0x1068] = 0x2028;
    Check(pal4::UtilitiesStringIdTable::Reserve(
              0x1000, 4, index_only) == 2 &&
              index_only.next_allocation == 1 &&
              index_only.calls.front() == "allocate 32" &&
              index_only.words[0x1060] == 0x2000,
          "index-only growth returns old active key count");

    auto unchanged = WithTwoEntries();
    unchanged.words[0x1068] = 0x2028;
    unchanged.words[0x1010] = 0x3028;
    Check(pal4::UtilitiesStringIdTable::Reserve(
              0x1000, 4, unchanged) == 5 &&
              unchanged.calls.empty(),
          "sufficient capacities make no allocations or writes");
}

void CheckEmptyAndNegative() {
    Runtime empty;
    Check(pal4::UtilitiesStringIdTable::Reserve(
              0x1000, 3, empty) == 0 &&
              empty.words[0x1060] == 0x5000 &&
              empty.words[0x1064] == 0x5000 &&
              empty.words[0x1068] == 0x5018 &&
              empty.words[0x1008] == 0x6000 &&
              empty.words[0x100c] == 0x6000 &&
              empty.words[0x1010] == 0x6018 &&
              At(empty, "free 0") < At(empty, "write 4200 20504"),
          "empty table reserves both arrays with zero active pairs");

    Runtime negative;
    Check(pal4::UtilitiesStringIdTable::Reserve(
              0x1000, -1, negative) == 0 &&
              negative.calls[0] == "allocate 0" &&
              negative.words[0x1068] == 0x5000 - 8 &&
              negative.words[0x1010] == 0x6000 - 8,
          "negative request retains unsigned comparison and zero allocation");
}

}  // namespace

int main() {
    try {
        CheckBothGrow();
        CheckIndependentGrowth();
        CheckEmptyAndNegative();
        std::cout << "utilities_string_id_capacity_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
