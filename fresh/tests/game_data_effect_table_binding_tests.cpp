#include "pal4/game_data_effect_table_binding.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataEffectTableBindingRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    bool fail_initialization = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t LogBind(std::string_view prefix,
                          std::uint32_t name) override {
        Check(name == 0x2034, "binding table name offset");
        calls.emplace_back(prefix);
        return prefix == "endBindField, table:" ? 0x77 : 0x11;
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x206c && source == 0x2014,
              "Effect assignment offsets");
        calls.emplace_back("assign");
    }
    void InitializeStructures(pal4::GameDataEffectTableKind kind,
                              std::uint32_t binding) override {
        Check(binding == 0x206c, "structure binding address");
        switch (kind) {
        case pal4::GameDataEffectTableKind::role:
            calls.emplace_back("role structures");
            break;
        case pal4::GameDataEffectTableKind::upgrade:
            calls.emplace_back("upgrade structures");
            break;
        case pal4::GameDataEffectTableKind::combat_prize:
            calls.emplace_back("prize structures");
            break;
        }
        if (fail_initialization)
            throw std::runtime_error("initialization failed");
    }
    void BindEffects(std::uint32_t binding) override {
        Check(binding == 0x206c, "Effect bind offset");
        calls.emplace_back("bind");
    }
};

Runtime Matching() {
    Runtime runtime;
    runtime.words[0x2080] = 0x9000;
    runtime.words[0x2084] = 0x9008;
    runtime.words[0x2018] = 0x4000;
    runtime.words[0x201c] = 0x4078;
    return runtime;
}

using Refresh = std::uint32_t (*)(
    std::uint32_t, pal4::IGameDataEffectTableBindingRuntime&);

void CheckOne(Refresh refresh, const char* expected_structure) {
    auto matching = Matching();
    Check(refresh(0x2000, matching) == 0 && matching.calls.empty(),
          "matched nonnull fields skip binding and return zero");
    Runtime null_fields;
    Check(refresh(0x2000, null_fields) == 0x2014 &&
              null_fields.calls.empty(),
          "matched null field arrays return embedded source address");
    auto empty_fields = Matching();
    empty_fields.words[0x201c] = 0x4000;
    empty_fields.words[0x2084] = 0x9000;
    Check(refresh(0x2000, empty_fields) == 0 &&
              empty_fields.calls.empty(),
          "nonnull empty source and bound arrays remain matched");

    auto mismatch = Matching();
    mismatch.words[0x2084] = 0x9004;
    Check(refresh(0x2000, mismatch) == 0x77 &&
              mismatch.calls == std::vector<std::string>{
                  "beginBindField, table:", "assign",
                  expected_structure, "bind", "endBindField, table:"},
          "mismatch logs, assigns, initializes and binds in order");

    auto failure = Matching();
    failure.words[0x2084] = 0x9004;
    failure.fail_initialization = true;
    bool threw = false;
    try {
        static_cast<void>(refresh(0x2000, failure));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failure.calls == std::vector<std::string>{
              "beginBindField, table:", "assign", expected_structure},
          "initialization failure stops before native bind and end log");
}

}  // namespace

int main() {
    try {
        CheckOne(pal4::GameDataEffectTableBinding::RefreshRole,
                 "role structures");
        CheckOne(pal4::GameDataEffectTableBinding::RefreshUpgrade,
                 "upgrade structures");
        CheckOne(pal4::GameDataEffectTableBinding::RefreshCombatPrize,
                 "prize structures");
        std::cout << "game_data_effect_table_binding_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
