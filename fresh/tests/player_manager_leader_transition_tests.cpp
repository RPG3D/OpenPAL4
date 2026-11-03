#include "pal4/player_manager_leader_transition.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kManager = 0x5000u;
constexpr std::uint32_t Slot(std::uint32_t index) {
    return kManager + index * 0xb14u;
}

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PlayerManagerLeaderRuntimeStub {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::map<std::uint32_t, std::int32_t> priorities;
    std::vector<std::string> events;
    std::vector<std::uint32_t> sorted_input;
    bool reverse_sort{};
    bool throw_sort{};

    std::uint32_t ReadDword(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return memory[address];
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address) + ":" +
                         std::to_string(value));
        memory[address] = value;
    }
    void SetStateFlag(std::uint32_t slot, std::uint32_t value) override {
        events.push_back("flag:" + std::to_string(slot) + ":" +
                         std::to_string(value));
        memory[slot + 0x1f0u] = value;
    }
    void SetAction(std::uint32_t component, std::uint32_t name) override {
        Check(name == pal4::PlayerManagerLeaderTransition::kActionName,
              "target action points to IDA C01 string");
        events.push_back("action:" + std::to_string(component));
    }
    void CopyTransform(std::uint32_t slot, std::uint32_t virtual_slot,
                       std::uint32_t source, std::uint32_t option) override {
        Check(option == 0u, "transform virtual call passes zero option");
        events.push_back("copy:" + std::to_string(slot) + ":" +
                         std::to_string(virtual_slot) + ":" +
                         std::to_string(source));
    }
    void SetPlayerState(std::uint32_t manager,
                        std::uint32_t state) override {
        Check(manager == kManager, "state transfer receives manager");
        events.push_back("player-state:" + std::to_string(state));
    }
    std::uint32_t Arena() override {
        events.emplace_back("arena");
        return 0x9000u;
    }
    void OnRemove(std::uint32_t arena, std::uint32_t slot) override {
        Check(arena == 0x9000u, "remove uses arena singleton");
        events.push_back("remove:" + std::to_string(slot));
    }
    void OnAdd(std::uint32_t arena, std::uint32_t slot) override {
        Check(arena == 0x9000u, "add uses fresh arena lookup");
        events.push_back("add:" + std::to_string(slot));
    }
    std::uint32_t ActorControl() override {
        events.emplace_back("control");
        return 0xa000u;
    }
    void BindActor(std::uint32_t control, std::uint32_t slot) override {
        Check(control == 0xa000u, "actor binding uses control singleton");
        events.push_back("bind:" + std::to_string(slot));
    }
    void SortActive(std::vector<std::uint32_t>& slots,
                    std::uint32_t comparator) override {
        Check(comparator == pal4::PlayerManagerLeaderTransition::kComparator,
              "small-array sort uses IDA comparator address");
        sorted_input = slots;
        events.emplace_back("sort");
        if (throw_sort) throw std::runtime_error("sort failure");
        if (reverse_sort) std::reverse(slots.begin(), slots.end());
    }
    std::int32_t GetPriority(std::uint32_t script) override {
        events.push_back("get:" + std::to_string(script));
        return priorities[script];
    }
    void SetPriority(std::uint32_t script, std::int32_t value) override {
        events.push_back("set:" + std::to_string(script) + ":" +
                         std::to_string(value));
        priorities[script] = value;
    }
};

void UnchangedAndInactivePaths() {
    Runtime runtime;
    runtime.memory[kManager + 0x2c50u] = 2u;
    pal4::PlayerManagerLeaderTransition::Switch(kManager, 2u, runtime);
    Check(runtime.events == std::vector<std::string>{
              "read:" + std::to_string(kManager + 0x2c50u)},
          "same index reads only the current index");

    runtime.events.clear();
    runtime.memory[Slot(1u) + 0xb08u] = 0u;
    pal4::PlayerManagerLeaderTransition::Switch(kManager, 1u, runtime);
    Check(runtime.memory[kManager + 0x2c50u] == 1u &&
          runtime.events == std::vector<std::string>{
              "read:" + std::to_string(kManager + 0x2c50u),
              "write:" + std::to_string(kManager + 0x2c50u) + ":1",
              "read:" + std::to_string(Slot(1u) + 0xb08u)},
          "inactive target keeps new index without a leader transition");
}

void FullTransition() {
    Runtime runtime;
    runtime.memory[kManager + 0x2c50u] = 0u;
    runtime.memory[Slot(0u) + 0xb0cu] = 7u;
    for (std::uint32_t i = 0u; i < 4u; ++i)
        runtime.memory[Slot(i) + 0xb08u] = 1u;
    runtime.reverse_sort = true;
    runtime.priorities[Slot(3u) + 0x654u] = 12;
    runtime.priorities[Slot(2u) + 0x654u] = 34;
    pal4::PlayerManagerLeaderTransition::Switch(kManager, 2u, runtime);

    Check(runtime.memory[kManager + 0x2c50u] == 2u &&
          runtime.sorted_input == std::vector<std::uint32_t>{
              Slot(0u), Slot(1u), Slot(2u), Slot(3u)},
          "all four active slots are collected in address order");
    for (std::uint32_t i = 0u; i < 4u; ++i)
        Check(runtime.memory[Slot(i) + 0x1f0u] == (i == 2u ? 1u : 0u),
              "only requested player retains active state flag");
    Check(runtime.priorities[Slot(3u) + 0x654u] == 34 &&
          runtime.priorities[Slot(2u) + 0x654u] == 12,
          "priority values swap between sorted first and new leader");

    const auto& e = runtime.events;
    const auto at = [&e](const std::string& name) {
        const auto found = std::find(e.begin(), e.end(), name);
        Check(found != e.end(), "expected transition event was emitted");
        return static_cast<std::size_t>(found - e.begin());
    };
    const auto write = at("write:" + std::to_string(kManager + 0x2c50u) + ":2");
    const auto last_clear = at("flag:" + std::to_string(Slot(3u)) + ":0");
    const auto activate = at("flag:" + std::to_string(Slot(2u)) + ":1");
    const auto action = at("action:" + std::to_string(Slot(2u) + 0x170u));
    const auto position = at("copy:" + std::to_string(Slot(2u)) + ":4:" +
                             std::to_string(Slot(0u) + 0xacu));
    const auto angle = at("copy:" + std::to_string(Slot(2u)) + ":8:" +
                          std::to_string(Slot(0u) + 0xa0u));
    const auto state = at("player-state:7");
    const auto remove = at("remove:" + std::to_string(Slot(0u)));
    const auto add = at("add:" + std::to_string(Slot(2u)));
    const auto bind = at("bind:" + std::to_string(Slot(2u)));
    const auto sort = at("sort");
    const auto first_get = at("get:" + std::to_string(Slot(3u) + 0x654u));
    const auto new_get = at("get:" + std::to_string(Slot(2u) + 0x654u));
    const auto first_set = at("set:" + std::to_string(Slot(3u) + 0x654u) + ":34");
    const auto new_set = at("set:" + std::to_string(Slot(2u) + 0x654u) + ":12");
    Check(write < last_clear && last_clear < activate && activate < action &&
          action < position && position < angle && angle < state &&
          state < remove && remove < add && add < bind && bind < sort &&
          sort < first_get && first_get < new_get && new_get < first_set &&
          first_set < new_set,
          "leader transition preserves all observed call stages");
    Check(e[remove - 1u] == "arena" && e[add - 1u] == "arena" &&
          e[bind - 1u] == "control",
          "arena and actor-control getters occur immediately before use");
}

void SingleActiveAndSortFailure() {
    Runtime runtime;
    runtime.memory[kManager + 0x2c50u] = 0u;
    runtime.memory[Slot(1u) + 0xb08u] = 1u;
    runtime.priorities[Slot(1u) + 0x654u] = 55;
    pal4::PlayerManagerLeaderTransition::Switch(kManager, 1u, runtime);
    Check(runtime.sorted_input == std::vector<std::uint32_t>{Slot(1u)} &&
          runtime.priorities[Slot(1u) + 0x654u] == 55,
          "one active player still passes through sort and self-swap");
    Check(std::count(runtime.events.begin(), runtime.events.end(),
                     "get:" + std::to_string(Slot(1u) + 0x654u)) == 2 &&
          std::count(runtime.events.begin(), runtime.events.end(),
                     "set:" + std::to_string(Slot(1u) + 0x654u) + ":55") == 2,
          "self-swap retains both virtual get and set calls");

    runtime.events.clear();
    runtime.memory[kManager + 0x2c50u] = 0u;
    runtime.throw_sort = true;
    bool threw = false;
    try {
        pal4::PlayerManagerLeaderTransition::Switch(kManager, 1u, runtime);
    } catch (const std::runtime_error&) { threw = true; }
    Check(threw && std::find(runtime.events.begin(), runtime.events.end(),
                             "get:" + std::to_string(Slot(1u) + 0x654u)) ==
                       runtime.events.end(),
          "sort failure unwinds before priority changes");
}

void ComparePlayerStates() {
    Runtime runtime;
    const auto left = Slot(0u) + 0x654u;
    const auto right = Slot(1u) + 0x654u;
    runtime.priorities[left] = -2;
    runtime.priorities[right] = 3;
    Check(pal4::PlayerManagerLeaderTransition::ComparePlayerStates(
              Slot(0u), Slot(1u), runtime),
          "signed lower priority sorts before higher priority");
    Check(runtime.events == std::vector<std::string>{
              "get:" + std::to_string(right),
              "get:" + std::to_string(left)},
          "comparator reads right script before left script");
    runtime.events.clear();
    runtime.priorities[left] = 3;
    Check(!pal4::PlayerManagerLeaderTransition::ComparePlayerStates(
              Slot(0u), Slot(1u), runtime),
          "equal priorities do not compare less");
    runtime.priorities[left] = 4;
    Check(!pal4::PlayerManagerLeaderTransition::ComparePlayerStates(
              Slot(0u), Slot(1u), runtime),
          "higher priority does not compare less");
}

}  // namespace

int main() {
    UnchangedAndInactivePaths();
    FullTransition();
    SingleActiveAndSortFailure();
    ComparePlayerStates();
}
