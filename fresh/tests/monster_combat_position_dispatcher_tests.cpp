#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_combat_position_dispatcher.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct FakeActionRuntime final : pal4::IMonsterActionDispatchRuntime {
    std::vector<std::string>* calls{};
    std::uint32_t result{0x7777u};

    explicit FakeActionRuntime(std::vector<std::string>* log) : calls(log) {}

    std::uint32_t ManagerInstance() override {
        calls->emplace_back("instance");
        return 0x9000u;
    }
    std::uint32_t IterateAndCallback(std::uint32_t instance,
        std::uint32_t callback, std::uint32_t context) override {
        Check(instance == 0x9000u &&
                  callback == pal4::MonsterCombatPositionDispatcher::kCallback &&
                  context == 0x5000u,
              "combat dispatcher callback and context");
        calls->emplace_back("iterate");
        return result;
    }
};

struct Runtime final : pal4::IMonsterCombatPositionRuntime {
    std::vector<std::string> calls;
    FakeActionRuntime action{&calls};
    std::int32_t count{2};
    std::vector<std::array<std::uint32_t, 3>> records;
    std::vector<std::uint32_t> ends;

    std::int32_t PositionCount(std::uint32_t source) override {
        Check(source == 0x8000u, "position count source");
        calls.emplace_back("count");
        return count;
    }
    std::uint32_t PositionId(std::uint32_t source,
                             std::uint32_t index) override {
        Check(source == 0x8000u && index < 2u,
              "position ID source and index");
        calls.emplace_back("id:" + std::to_string(index));
        return 0x1000u + index;
    }
    std::int32_t PositionValue(std::uint32_t source,
        const std::string& key, std::int32_t fallback) override {
        Check(source == 0x8000u && fallback == 0 &&
                  (key == "AAB_MST_ESSBEH_combat_pos_0" ||
                   key == "AAB_MST_ESSBEH_combat_pos_1"),
              "indexed combat-position key and default");
        calls.emplace_back(std::string("value:") + key.back());
        return key.back() == '0' ? -3 : 0;
    }
    void AppendRecord(pal4::MonsterBehaviorManagerState& state,
        std::uint32_t array_address, std::uint32_t end_address,
        std::uint32_t count_value,
        const std::array<std::uint32_t, 3>& record) override {
        Check(array_address == 0x5710u && count_value == 1u,
              "12-byte record array and insertion count");
        calls.emplace_back("append");
        ends.push_back(end_address);
        records.push_back(record);
        state.words[0x718u / 4u] += 12u;
    }
    void AddManagerText(std::uint32_t address,
                        std::uint32_t text_address) override {
        Check(address == 0x5000u && text_address == 0xabcdu,
              "manager text field forwarding");
        calls.emplace_back("text");
    }
    pal4::IMonsterActionDispatchRuntime& ActionRuntime() override {
        calls.emplace_back("action-runtime");
        return action;
    }
};

pal4::MonsterBehaviorManagerState InitialState() {
    pal4::MonsterBehaviorManagerState state{};
    state.words[0x654u / 4u] = 0x8000u;
    state.words[0x718u / 4u] = 0xa000u;
    state.words[0x730u / 4u] = 0x4567u;
    state.words[1u] = 0xabcdu;
    return state;
}

void TwoPositions() {
    auto state = InitialState();
    Runtime runtime;
    Check(pal4::MonsterCombatPositionDispatcher::Process(
              state, 0x5000u, runtime) == 0x7777u &&
              runtime.ends == std::vector<std::uint32_t>{
                  0xa000u, 0xa00cu} &&
              runtime.records == std::vector<std::array<std::uint32_t, 3>>{
                  {0x1000u, 0xfffffffdu, 0x4567u},
                  {0x1001u, 0u, 0x4567u}} &&
              state.words[0x718u / 4u] == 0xa018u &&
              runtime.calls == std::vector<std::string>{
                  "count", "id:0", "value:0", "append",
                  "count", "id:1", "value:1", "append", "count",
                  "text", "action-runtime", "instance", "iterate"},
          "combat positions append ordered records before text and callback");
}

void EmptyAndStub() {
    auto state = InitialState();
    Runtime empty;
    empty.count = 0;
    empty.action.result = 0u;
    Check(pal4::MonsterCombatPositionDispatcher::Process(
              state, 0x5000u, empty) == 0u &&
              empty.records.empty() &&
              empty.calls == std::vector<std::string>{
                  "count", "text", "action-runtime", "instance", "iterate"},
          "empty configuration still adds text and traverses collection");
    pal4::MonsterCombatPositionRuntimeStub stub;
    Check(pal4::MonsterCombatPositionDispatcher::Process(
              state, 0x5000u, stub) == 0u,
          "config, array and monster services have linkable stubs");
}

}  // namespace

int main() {
    TwoPositions();
    EmptyAndStub();
    std::cout << "monster combat position cases passed\n";
}
