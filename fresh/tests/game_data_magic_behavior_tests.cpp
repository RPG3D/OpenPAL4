#include "pal4/game_data_magic_behavior.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataMagicBehaviorRuntime {
    std::map<std::uint32_t, std::int32_t> words{
        {0x5008, 0}, {0x5004, 0}, {0x5050, 0}, {0x5048, 39}};
    std::vector<std::string> calls;
    std::uint32_t db_calls = 0;
    std::uint32_t magic_reads = 0;
    bool skill_found = true;

    std::int32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        Check(words.contains(address), "only observed offsets are read");
        if (address == 0x5048)
            return words.at(address) + static_cast<std::int32_t>(magic_reads++);
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write");
        Check(address == 0x5000 && value == 0x842bbcu,
              "base constructor writes observed vtable");
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("db");
        return 0x1000 * ++db_calls;
    }
    bool HasSkill(std::uint32_t catalog,
                     std::int32_t magic) override {
        calls.emplace_back("has_skill");
        Check(catalog == 0x100c && magic == 39,
              "HasSkill uses first GameDB catalog and attached Skill ID");
        return skill_found;
    }
    std::uint32_t FindSkill(std::uint32_t catalog,
                std::int32_t magic) override {
        calls.emplace_back("find_skill");
        Check(catalog == 0x200c && magic == 40,
              "FindSkill uses second GameDB catalog and reread attached Skill ID");
        return 0xfeed1234u;
    }
};

void CheckBounds() {
    Runtime runtime;
    runtime.words[0x5008] = 6;
    runtime.words[0x5004] = 4;
    runtime.words[0x5050] = 5;
    Check(pal4::GameDataMagicBehavior::MembersInRange(0x5000, runtime) &&
              runtime.calls == std::vector<std::string>{
                                   "read 20488", "read 20484", "read 20560"},
          "valid upper limits read in IDA order");
    for (const auto [address, invalid] : {
             std::pair{0x5008u, 7}, std::pair{0x5004u, 5},
             std::pair{0x5050u, 6}, std::pair{0x5008u, -1},
             std::pair{0x5004u, -1}, std::pair{0x5050u, -1}}) {
        const auto old = runtime.words[address];
        runtime.words[address] = invalid;
        Check(!pal4::GameDataMagicBehavior::MembersInRange(0x5000, runtime),
              "signed out-of-range member rejected");
        runtime.words[address] = old;
    }
    runtime.words[0x5008] = -1;
    runtime.calls.clear();
    (void)pal4::GameDataMagicBehavior::MembersInRange(0x5000, runtime);
    Check(runtime.calls == std::vector<std::string>{"read 20488"},
          "first invalid value short-circuits later reads");
}

void CheckProcessing() {
    Runtime runtime;
    Check(pal4::GameDataMagicBehavior::ResolveAttachedSkill(0x5000, runtime) == 0xfeed1234u &&
              runtime.calls == std::vector<std::string>{
                                   "db", "read 20552", "has_skill", "db",
                                   "read 20552", "find_skill"},
          "both Skill lookups have their own GameDB lookup and member read");
    runtime = Runtime{};
    runtime.skill_found = false;
    Check(pal4::GameDataMagicBehavior::ResolveAttachedSkill(0x5000, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                                   "db", "read 20552", "has_skill"},
          "missing Skill suppresses FindSkill and second GameDB call");
}

void CheckAccessAndBase() {
    Runtime runtime;
    runtime.words[0x5050] = -9;
    Check(pal4::GameDataMagicBehavior::MemberAt50(0x5000, runtime) == -9,
          "getter returns signed member at +0x50");
    for (int value : {2, 4}) {
        runtime.words[0x5050] = value;
        Check(pal4::GameDataMagicBehavior::MemberIsTwoOrFour(
                  0x5000, runtime),
              "two and four are accepted");
    }
    for (int value : {-1, 0, 1, 3, 5}) {
        runtime.words[0x5050] = value;
        Check(!pal4::GameDataMagicBehavior::MemberIsTwoOrFour(
                  0x5000, runtime),
              "other values are rejected");
    }
    runtime.calls.clear();
    Check(pal4::GameDataMagicBehavior::InitializeTextFieldBase(
              0x5000, runtime) == 0x5000 &&
              runtime.calls == std::vector<std::string>{"write"},
          "base constructor writes vtable and returns this");
}

}  // namespace

int main() {
    try {
        CheckBounds();
        CheckProcessing();
        CheckAccessAndBase();
        std::cout << "game_data_magic_behavior_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
