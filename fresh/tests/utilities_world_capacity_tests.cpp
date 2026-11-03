#include "pal4/utilities_world_table.h"

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

struct Runtime final : pal4::IUtilitiesWorldCapacityRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1060, 0x4000}, {0x1064, 0x4050}, {0x1068, 0x4078},
        {0x1008, 0x3000}, {0x100c, 0x3010}, {0x1010, 0x3018},
        {0x3000, 9}, {0x3004, 0}, {0x3008, 2}, {0x300c, 1}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> destroyed;
    std::uint32_t next_allocation = 0x5000;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocations.push_back(bytes);
        calls.emplace_back("allocate");
        const auto result = next_allocation;
        next_allocation += 0x1000;
        return result;
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free");
    }
    void CopyRecord(std::uint32_t to, std::uint32_t from) override {
        Check(to == 0x5000 + (from - 0x4000),
              "40-byte record copy address");
        calls.emplace_back("copy");
    }
    void DestroyRecord(std::uint32_t address) override {
        destroyed.push_back(address);
        calls.emplace_back("destroy");
    }
};

void CheckBothGrow() {
    Runtime runtime;
    Check(pal4::UtilitiesWorldTable::Reserve(0x1000, 4, runtime) ==
              0x6010, "growth returns new index end");
    Check(runtime.allocations == std::vector<std::uint32_t>{160, 32} &&
              runtime.freed == std::vector<std::uint32_t>{0x4000, 0x3000} &&
              runtime.destroyed == std::vector<std::uint32_t>{
                  0x4000, 0x4028},
          "both buffers grow independently with old rows destroyed");
    Check(runtime.words.at(0x1060) == 0x5000 &&
              runtime.words.at(0x1064) == 0x5050 &&
              runtime.words.at(0x1068) == 0x50a0 &&
              runtime.words.at(0x1008) == 0x6000 &&
              runtime.words.at(0x100c) == 0x6010 &&
              runtime.words.at(0x1010) == 0x6020 &&
              runtime.words.at(0x6000) == 9 &&
              runtime.words.at(0x600c) == 1,
          "new pointer triplets and index contents");
    Check(runtime.calls[0] == "allocate" &&
              runtime.calls[1] == "copy" &&
              runtime.calls[2] == "copy" &&
              runtime.calls[3] == "destroy" &&
              runtime.calls[4] == "destroy" &&
              runtime.calls[5] == "free",
          "record relocation precedes old row destruction");
}

void CheckIndependentAndUnchanged() {
    Runtime keys_only;
    keys_only.words[0x1068] = 0x50a0;
    Check(pal4::UtilitiesWorldTable::Reserve(0x1000, 4,
                                             keys_only) == 0x5010 &&
              keys_only.allocations == std::vector<std::uint32_t>{32} &&
              keys_only.destroyed.empty(),
          "sufficient row capacity leaves rows untouched");

    Runtime records_only;
    records_only.words[0x1010] = 0x3040;
    Check(pal4::UtilitiesWorldTable::Reserve(0x1000, 4,
                                             records_only) == 8 &&
              records_only.allocations ==
                  std::vector<std::uint32_t>{160} &&
              records_only.words.at(0x1008) == 0x3000,
          "sufficient index capacity returns old capacity");

    Runtime unchanged;
    unchanged.words[0x1068] = 0x50a0;
    unchanged.words[0x1010] = 0x3040;
    Check(pal4::UtilitiesWorldTable::Reserve(0x1000, 4,
                                             unchanged) == 8 &&
              unchanged.allocations.empty(),
          "unchanged buffers require no allocation");
}

void CheckEmpty() {
    Runtime runtime;
    runtime.words[0x1060] = 0;
    runtime.words[0x1064] = 0;
    runtime.words[0x1068] = 0;
    runtime.words[0x1008] = 0;
    runtime.words[0x100c] = 0;
    runtime.words[0x1010] = 0;
    Check(pal4::UtilitiesWorldTable::Reserve(0x1000, 1,
                                             runtime) == 0x6000 &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{40, 8} &&
              runtime.words.at(0x1064) == 0x5000 &&
              runtime.words.at(0x100c) == 0x6000,
          "empty buffers allocate one row and one key");

    Runtime negative;
    negative.words[0x1060] = 0;
    negative.words[0x1064] = 0;
    negative.words[0x1068] = 0;
    negative.words[0x1008] = 0;
    negative.words[0x100c] = 0;
    negative.words[0x1010] = 0;
    Check(pal4::UtilitiesWorldTable::Reserve(0x1000, -1,
                                             negative) == 0x6000 &&
              negative.allocations ==
                  std::vector<std::uint32_t>{0, 0} &&
              negative.words.at(0x1068) == 0x4fd8 &&
              negative.words.at(0x1010) == 0x5ff8,
          "negative request preserves native unsigned comparison and zero allocation");
}

}  // namespace

int main() {
    try {
        CheckBothGrow();
        CheckIndependentAndUnchanged();
        CheckEmpty();
        std::cout << "utilities_world_capacity_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
