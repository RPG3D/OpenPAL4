#include "pal4/combat_end_action_task.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;

struct Runtime final : pal4::ICombatEndActionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{kTask + 44u, kSource}};
    std::vector<std::string> calls;
    bool state_three = false;
    std::uint32_t action = 0x3000u;
    std::uint8_t event_result = 0x5au;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kTask + 44u, "EndAction source field");
        calls.push_back("read_source");
        return words.at(address);
    }
    void NotifySource(std::uint32_t source) override {
        Check(source == kSource, "source virtual +480 argument");
        calls.push_back("notify");
    }
    bool SourceStateThree(std::uint32_t source) override {
        Check(source == kSource, "state query argument");
        calls.push_back("state_three");
        return state_three;
    }
    std::uint32_t ProcessDead(std::uint32_t source,
                              std::uint32_t target) override {
        Check(source == kSource && target == kSource,
              "death action uses same actor twice");
        calls.push_back("process_dead");
        return action;
    }
    std::uint8_t RegisterActorEvent(std::uint32_t value) override {
        Check(value == action, "event registration action identity");
        calls.push_back("register_event");
        return event_result;
    }
};

}  // namespace

int main() {
    try {
        Check(pal4::CombatEndActionTask::kVtable == 0x845c9cu,
              "EndAction task vtable");
        Runtime r;
        Check(pal4::CombatEndActionTask::Start(kTask, r) == 0u &&
                  r.calls == std::vector<std::string>{
                      "read_source", "notify", "read_source",
                      "state_three"},
              "non-state-three exits after notification");

        r = Runtime{};
        r.state_three = true;
        Check(pal4::CombatEndActionTask::Start(kTask, r) == 0x5au &&
                  r.calls == std::vector<std::string>{
                      "read_source", "notify", "read_source",
                      "state_three", "read_source", "process_dead",
                      "register_event"},
              "state three creates and registers death action");

        r = Runtime{};
        r.state_three = true;
        r.action = 0u;
        r.event_result = 0xfeu;
        Check(pal4::CombatEndActionTask::Start(kTask, r) == 0xfeu,
              "registration receives action result even when null");
        std::cout << "Combat EndAction tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
