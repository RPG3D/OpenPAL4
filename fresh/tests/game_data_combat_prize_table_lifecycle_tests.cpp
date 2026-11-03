#include "pal4/game_data_combat_prize_table_lifecycle.h"

#include <algorithm>
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

struct Runtime final : pal4::IGameDataEffectTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x2000;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 140, "CombatPrize table size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void ConstructEffect(std::uint32_t object,
                         std::uint32_t name) override {
        Check(object == 0x2000 && name == 0x1234,
              "Effect constructor arguments");
        calls.emplace_back("construct");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte:" + std::to_string(address));
    }
    void ClearEffectData(std::uint32_t object) override {
        Check(object == 0x2000, "Effect clear address");
        calls.emplace_back("clear");
    }
    void CleanupTextRecords(std::uint32_t) override {
        throw std::runtime_error("unexpected text cleanup");
    }
    void ProcessElements(std::uint32_t address) override {
        Check(address == 0x206c, "binding cleanup address");
        calls.emplace_back("process");
    }
    void DestroyRecord(std::uint32_t) override {
        throw std::runtime_error("unexpected record cleanup");
    }
    void DestroyAuxRecord(std::uint32_t address) override {
        calls.push_back("aux:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint32_t DestructEffect(std::uint32_t object) override {
        Check(object == 0x2000, "Effect destructor address");
        calls.emplace_back("destruct");
        return 0x55;
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(),
                                 runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "missing expected operation");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckConstruct() {
    Runtime runtime;
    using Table = pal4::GameDataCombatPrizeTableLifecycle;
    Check(Table::Construct(0x1234, runtime) == 0x2000 &&
              runtime.calls.size() == 15 &&
              runtime.calls[0] == "allocate" &&
              runtime.calls[1] == "construct" &&
              runtime.words[0x2000] == Table::kVtable,
          "construction installs CombatPrize vtable after base construction");
    for (std::uint32_t group : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x2000 + group] == 0x34,
              "embedded flag receives source low byte");
        for (std::uint32_t offset : {4u, 8u, 12u})
            Check(runtime.words[0x2000 + group + offset] == 0,
                  "embedded pointer triplets begin empty");
    }
    Runtime failure;
    failure.allocation = 0;
    Check(Table::Construct(0x1234, failure) == 0 &&
              failure.calls == std::vector<std::string>{"allocate"},
          "allocation failure skips all later work");
}

void CheckDestroyAndClear() {
    Runtime runtime;
    runtime.words[0x2060] = 0x4000;
    runtime.words[0x2064] = 0x40a0;
    runtime.words[0x2068] = 0x4200;
    runtime.words[0x2070] = 0x5000;
    runtime.words[0x2074] = 0x5078;
    runtime.words[0x2078] = 0x5100;
    runtime.words[0x2080] = 0x6000;
    runtime.words[0x2084] = 0x6010;
    runtime.words[0x2088] = 0x6020;
    Check(pal4::GameDataCombatPrizeTableLifecycle::Destroy(
              0x2000, runtime) == 0x55 &&
              runtime.words[0x2000] == 0x84468c &&
              runtime.calls[0] == "word:8192" &&
              runtime.calls[1] == "clear" &&
              At(runtime, "process") < At(runtime, "free:24576") &&
              At(runtime, "free:24576") < At(runtime, "aux:20480") &&
              At(runtime, "aux:20480") < At(runtime, "aux:20540") &&
              At(runtime, "aux:20540") < At(runtime, "free:20480") &&
              At(runtime, "free:20480") < At(runtime, "free:16384") &&
              runtime.calls.back() == "destruct",
          "cleanup order and 60-byte auxiliary stride");
    for (std::uint32_t offset : {96u, 100u, 104u, 112u, 116u,
                                 120u, 128u, 132u, 136u})
        Check(runtime.words[0x2000 + offset] == 0,
              "cleanup clears all three pointer triplets");

    Runtime clear;
    clear.words[0x2060] = 0x4000;
    clear.words[0x2064] = 0x40a0;
    clear.words[0x2068] = 0x4200;
    Check(pal4::GameDataCombatPrizeTableLifecycle::ClearLoadedRecords(
              0x2000, clear) == 0x40a0 &&
              clear.words[0x2064] == 0x4000 &&
              clear.words[0x2068] == 0x4200 &&
              clear.calls == std::vector<std::string>{
                  "clear", "word:8292"},
          "clear returns old end and keeps record capacity");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckDestroyAndClear();
        std::cout << "game_data_combat_prize_table_lifecycle_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
