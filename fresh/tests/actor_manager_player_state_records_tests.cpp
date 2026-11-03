#include "pal4/actor_manager_player_state_records.h"

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

constexpr std::uint32_t actor = 0x1000u;

struct Runtime final : pal4::IActorManagerPlayerStateRecordRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::uint32_t state = 0xffffffffu;
    std::uint32_t update_result = 0x7a7au;
    std::uint32_t output = 0x3000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == actor + 1192u, "saved state offset");
        trace.emplace_back("read");
        return words[address];
    }
    std::uint32_t ActorSlot52(std::uint32_t actual_actor,
                              std::uint32_t mode) override {
        Check(actual_actor == actor && mode == 4u,
              "actor virtual slot 52 uses mode four");
        trace.emplace_back("slot52");
        words[actor + 1192u] = 99u;
        return state;
    }
    std::uint32_t UpdateStateAndProcess(std::uint32_t actual_actor,
                                        std::uint32_t actual_state,
                                        std::uint32_t saved_value) override {
        Check(actual_actor == actor && actual_state == state &&
                  saved_value == 7u && words[actor + 1192u] == 99u,
              "state update gets pre-virtual saved value");
        trace.emplace_back("update");
        return update_result;
    }
    void BuildMatchingRecords(std::uint32_t collection,
                              std::uint32_t actual_output,
                              std::uint32_t actual_state,
                              std::uint32_t saved_value) override {
        Check(collection == actor + 920u &&
                  actual_output == output && actual_state == state &&
                  saved_value == 7u && words[actor + 1192u] == 99u,
              "matching builder gets collection, output and saved state");
        trace.emplace_back("build");
    }
};

}  // namespace

int main() {
    try {
        Runtime update;
        update.words[actor + 1192u] = 7u;
        Check(pal4::ActorManagerPlayerStateRecords::Update(actor, update) ==
                  update.update_result &&
                  update.trace ==
                      std::vector<std::string>{"read", "slot52", "update"},
              "state processor forwards result after saving actor field");

        Runtime matching;
        matching.words[actor + 1192u] = 7u;
        Check(pal4::ActorManagerPlayerStateRecords::BuildMatching(
                  actor, matching.output, matching) == matching.output &&
                  matching.trace ==
                      std::vector<std::string>{"read", "slot52", "build"},
              "matching builder preserves output address and call order");

        Runtime zero_output;
        zero_output.words[actor + 1192u] = 7u;
        zero_output.output = 0u;
        Check(pal4::ActorManagerPlayerStateRecords::BuildMatching(
                  actor, 0u, zero_output) == 0u &&
                  zero_output.trace.back() == "build",
              "zero output is forwarded to native builder unchanged");
        std::cout << "actor_manager_player_state_records_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
