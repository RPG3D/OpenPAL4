#include "pal4/cast_magic_sequence_task.h"

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
constexpr std::uint32_t kTarget = 0x3000u;

struct Runtime final : pal4::ICastMagicCompletionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget}};
    std::vector<std::string> calls;
    bool active = true;
    bool state_three = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    bool CombatActive() override {
        calls.push_back("combat_active");
        return active;
    }
    bool SourceStateThree(std::uint32_t source) override {
        Check(source == kSource, "state source");
        calls.push_back("state_three");
        return state_three;
    }
    void NotifyDefeatedSource(std::uint32_t source) override {
        Check(source == kSource, "defeated source");
        calls.push_back("notify_defeated");
    }
    std::uint32_t ProcessDead(std::uint32_t source,
                              std::uint32_t target) override {
        Check(source == kSource && target == kTarget, "dead action actors");
        calls.push_back("process_dead");
        return 0xaaaau;
    }
    std::uint32_t EndAction(std::uint32_t source) override {
        Check(source == kSource, "end action source");
        calls.push_back("end_action");
        return 0xbbbbu;
    }
};

}  // namespace

int main() {
    try {
    Runtime r;
    r.active = false;
    Check(pal4::CastMagicSequenceTask::Complete(kTask, r) == 0u &&
              r.calls == std::vector<std::string>{"combat_active"},
          "inactive combat exits before task reads");

    r = Runtime{};
    Check(pal4::CastMagicSequenceTask::Complete(kTask, r) == 0xbbbbu &&
              r.calls == std::vector<std::string>{
                  "combat_active", "read:4140", "state_three",
                  "read:4140", "end_action"},
          "ordinary state ends source action");

    r = Runtime{};
    r.state_three = true;
    Check(pal4::CastMagicSequenceTask::Complete(kTask, r) == 0xaaaau &&
              r.calls == std::vector<std::string>{
                  "combat_active", "read:4140", "state_three",
                  "read:4140", "notify_defeated", "read:4144",
                  "read:4140", "process_dead"},
          "state three notifies then processes defeated source");
    std::cout << "CastMagic completion tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
