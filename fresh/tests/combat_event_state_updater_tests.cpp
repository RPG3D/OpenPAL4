#include "pal4/combat_event_state_updater.h"

#include <bit>
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

struct StateRuntime final : pal4::ICombatEventStateUpdateRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("state read " + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("state write " + std::to_string(address) + " " +
                        std::to_string(value));
        words[address] = value;
    }
    std::int32_t SetSlot(std::uint32_t actor, std::uint32_t slot,
                         std::int32_t value) override {
        calls.push_back("set " + std::to_string(actor) + " " +
                        std::to_string(slot) + " " + std::to_string(value));
        return slot == 9u ? 99 : 17;
    }
    std::int32_t QuerySlot(std::uint32_t actor,
                           std::uint32_t slot) override {
        calls.push_back("query " + std::to_string(actor) + " " +
                        std::to_string(slot));
        return static_cast<std::int32_t>(slot) + 10;
    }
    void RestoreSlot(std::uint32_t actor, std::uint32_t slot,
                     std::int32_t value) override {
        calls.push_back("restore " + std::to_string(actor) + " " +
                        std::to_string(slot) + " " + std::to_string(value));
    }
    void Notify(std::uint32_t actor) override {
        calls.push_back("notify " + std::to_string(actor));
    }
    std::int32_t BaseStat(std::uint32_t actor,
                          std::uint32_t slot) override {
        calls.push_back("base " + std::to_string(actor) + " " +
                        std::to_string(slot));
        return slot == 0u ? 31 : 41;
    }
};

struct EventRuntime final : pal4::ICombatEventDataRuntime {
    StateRuntime& state;
    std::map<std::uint32_t, std::uint32_t> words;

    explicit EventRuntime(StateRuntime& state_runtime) : state(state_runtime) {
        words[5044u] = 6000u;
        words[6096u] = 8000u;
        words[8152u] = 0xfffffffdu;
        words[8148u] = 1000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        state.calls.push_back("event read " + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        state.calls.push_back("event write " + std::to_string(address) + " " +
                              std::to_string(value));
    }
    std::uint32_t GameDatabase() override {
        state.calls.push_back("database");
        return 5000u;
    }
    std::uint32_t InitializeMatches(std::uint8_t key_low_byte) override {
        state.calls.push_back("matches " + std::to_string(key_low_byte));
        return 7000u;
    }
    std::uint32_t UpgradeRecordCount(std::uint32_t count_holder) override {
        state.calls.push_back("count " + std::to_string(count_holder));
        return 1u;
    }
    void InsertMatch(std::uint32_t collection,
                     std::uint32_t entry) override {
        state.calls.push_back("insert " + std::to_string(collection) + " " +
                              std::to_string(entry));
    }
    std::uint32_t MatchCount(std::uint32_t collection) override {
        state.calls.push_back("match-count " + std::to_string(collection));
        return 0u;
    }
    std::uint32_t FirstMatch(std::uint32_t) override {
        throw std::logic_error("unexpected FirstMatch");
    }
    std::int32_t LowerBound(std::uint32_t, std::uint32_t,
                            std::uint32_t) override {
        throw std::logic_error("unexpected LowerBound");
    }
    std::int32_t UpperBound(std::uint32_t, std::uint32_t,
                            std::uint32_t) override {
        throw std::logic_error("unexpected UpperBound");
    }
    void SetValidatedProperty(std::uint32_t, std::uint32_t,
                              std::int32_t) override {
        throw std::logic_error("unexpected SetValidatedProperty");
    }
    void CopyBytes(std::uint32_t, std::uint32_t,
                   std::uint32_t) override {
        throw std::logic_error("unexpected CopyBytes");
    }
    void CleanupMatchRange(std::uint32_t) override {
        throw std::logic_error("unexpected CleanupMatchRange");
    }
    int FreeMatches(std::uint32_t collection) override {
        state.calls.push_back("free " + std::to_string(collection));
        for (std::uint32_t i = 0u; i < 5u; ++i)
            state.words[1128u + i * 4u] = 999u;
        return 17;
    }
};

void SavesProcessesAndRestores() {
    StateRuntime state;
    for (std::uint32_t i = 0u; i < 5u; ++i)
        state.words[1128u + i * 4u] = 100u + i;
    EventRuntime event(state);
    Check(pal4::CombatEventStateUpdater::Process(
              1000u, -3, 0xffffffffu, state, event) == 99 &&
              state.words[1224u] == 0xfffffffdu,
          "state updater sets raw value and forwards final slot result");
    for (std::uint32_t i = 0u; i < 5u; ++i)
        Check(state.words[1128u + i * 4u] == 100u + i,
              "all five cached words are restored after event processing");

    const auto& calls = state.calls;
    const auto find = [&](const std::string& entry) {
        for (std::size_t i = 0; i < calls.size(); ++i)
            if (calls[i] == entry) return i;
        return calls.size();
    };
    Check(calls[0] == "state write 1224 4294967293" &&
              calls[1] == "set 1000 4 -3" &&
              find("state read 1128") < find("query 1000 0") &&
              find("query 1000 4") < find("database") &&
              find("matches 253") < find("count 6092") &&
              find("event read 8152") < find("event read 8148") &&
              find("insert 7000 8000") < find("free 7000") &&
              find("free 7000") < find("state write 1128 100") &&
              find("state write 1144 104") < find("restore 1000 4 14") &&
              find("restore 1000 4 14") < find("notify 1000") &&
              find("notify 1000") < find("base 1000 0") &&
              find("base 1000 0") < find("set 1000 7 31") &&
              find("set 1000 7 31") < find("base 1000 2") &&
              calls.back() == "set 1000 9 41",
          "save, reconstructed event processing, restore and final stat writes preserve order");
}

}  // namespace

int main() {
    try {
        SavesProcessesAndRestores();
        std::cout << "combat event state updater tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
