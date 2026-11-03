#include "pal4/game_data_skill_behavior.h"

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

struct Runtime final : pal4::IGameDataSkillBehaviorRuntime {
    std::map<std::uint32_t, std::int32_t> words{{0x5088, 0}, {0x5004, 0},
                                                {0x5084, 0}, {0x5080, 0},
                                                {0x5070, 42}};
    std::vector<std::string> calls;
    bool skill_found = true;
    std::uint32_t found_record = 0xfeed1234u;
    std::uint32_t database_calls = 0;
    std::uint32_t skill_reads = 0;

    std::int32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        Check(words.contains(address), "only IDA-backed member offsets read");
        if (address == 0x5070) {
            ++skill_reads;
            return words.at(address) + static_cast<std::int32_t>(skill_reads - 1);
        }
        return words.at(address);
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("db");
        return 0x1000 * ++database_calls;
    }
    bool HasSkill(std::uint32_t script_object,
                     std::int32_t skill) override {
        calls.emplace_back("has_skill");
        Check(script_object == 0x100c && skill == 42,
              "first GameDB and Skill member reach HasSkill");
        return skill_found;
    }
    std::uint32_t FindSkill(std::uint32_t script_object,
                std::int32_t skill) override {
        calls.emplace_back("find_skill");
        Check(script_object == 0x200c && skill == 43,
              "second GameDB and Skill member reread reach FindSkill");
        return found_record;
    }
};

void CheckBounds() {
    Runtime runtime;
    Check(pal4::GameDataSkillBehavior::MembersInRange(0x5000, runtime) &&
              runtime.calls == std::vector<std::string>{
                                   "read 20616", "read 20484",
                                   "read 20612", "read 20608"},
          "all four bounds read in IDA order");
    runtime.words[0x5088] = 5;
    runtime.words[0x5004] = 6;
    runtime.words[0x5084] = 2;
    Check(pal4::GameDataSkillBehavior::MembersInRange(0x5000, runtime),
          "inclusive upper valid limits");
    for (const auto [address, invalid] : {
             std::pair{0x5088u, 6}, std::pair{0x5004u, 7},
             std::pair{0x5084u, 3}, std::pair{0x5080u, -1},
             std::pair{0x5088u, -1}, std::pair{0x5004u, -1},
             std::pair{0x5084u, -1}}) {
        const auto original = runtime.words[address];
        runtime.words[address] = invalid;
        runtime.calls.clear();
        Check(!pal4::GameDataSkillBehavior::MembersInRange(0x5000, runtime),
              "signed out-of-range value rejected");
        runtime.words[address] = original;
    }
    runtime.words[0x5088] = -1;
    runtime.calls.clear();
    (void)pal4::GameDataSkillBehavior::MembersInRange(0x5000, runtime);
    Check(runtime.calls == std::vector<std::string>{"read 20616"},
          "first invalid field short-circuits later reads");
}

void CheckProcessing() {
    Runtime runtime;
    Check(pal4::GameDataSkillBehavior::ResolveAttachedSkill(0x5000, runtime) == 0xfeed1234u &&
              runtime.calls == std::vector<std::string>{
                                   "db", "read 20592", "has_skill", "db",
                                   "read 20592", "find_skill"},
          "HasSkill and FindSkill use separate GameDB lookups and member reads");
    runtime = Runtime{};
    runtime.skill_found = false;
    Check(pal4::GameDataSkillBehavior::ResolveAttachedSkill(0x5000, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                                   "db", "read 20592", "has_skill"},
          "missing Skill suppresses FindSkill and second GameDB lookup");
}

void CheckMemberAccess() {
    Runtime runtime;
    runtime.words[0x5088] = -12;
    Check(pal4::GameDataSkillBehavior::MemberAt88(0x5000, runtime) == -12,
          "member getter preserves signed 32-bit value");
    for (int value : {2, 4}) {
        runtime.words[0x5088] = value;
        Check(pal4::GameDataSkillBehavior::MemberIsTwoOrFour(
                  0x5000, runtime),
              "two and four pass the predicate");
    }
    for (int value : {-1, 0, 1, 3, 5}) {
        runtime.words[0x5088] = value;
        Check(!pal4::GameDataSkillBehavior::MemberIsTwoOrFour(
                  0x5000, runtime),
              "other values fail the predicate");
    }
}

}  // namespace

int main() {
    try {
        CheckBounds();
        CheckProcessing();
        CheckMemberAccess();
        std::cout << "game_data_skill_behavior_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
