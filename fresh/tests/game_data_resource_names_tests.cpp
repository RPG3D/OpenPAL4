#include "pal4/game_data_resource_names.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using Mode = pal4::GameDataStringMode;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Record {
    std::uint32_t address;
    std::string text;
    Mode mode;
};

struct Runtime final : pal4::IGameDataResourceNamesRuntime {
    std::vector<Record> assigned;
    std::vector<std::uint32_t> initialized;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::string> calls;
    std::uint8_t expected_seed = 0x5a;
    std::map<std::uint32_t, int> results;

    void InitializeBuffer(std::uint32_t address, std::uint8_t seed,
                          Mode mode) override {
        Check(seed == expected_seed, "uninitialized source byte is explicit");
        initialized.push_back(address);
        initialized_modes.push_back(mode);
        calls.emplace_back("init");
    }
    int AssignText(std::uint32_t address, std::string_view text,
                   Mode mode) override {
        Check(initialized.back() == address &&
                  initialized_modes.back() == mode,
              "buffer initialized with matching mode before text");
        assigned.push_back({address, std::string(text), mode});
        calls.emplace_back("assign");
        if (auto found = results.find(address); found != results.end())
            return found->second;
        return static_cast<int>(assigned.size());
    }
    int RegisterCleanup(pal4::GameDataResourceNameGroup group) override {
        Check(group == pal4::GameDataResourceNameGroup::equipment ||
                  group == pal4::GameDataResourceNameGroup::character ||
                  group == pal4::GameDataResourceNameGroup::character_extra ||
                  group == pal4::GameDataResourceNameGroup::attack ||
                  group == pal4::GameDataResourceNameGroup::progression ||
                  group == pal4::GameDataResourceNameGroup::combat_parameters ||
                  group == pal4::GameDataResourceNameGroup::special_item_effects ||
                  group == pal4::GameDataResourceNameGroup::elemental_attributes ||
                  group == pal4::GameDataResourceNameGroup::easb ||
                  group == pal4::GameDataResourceNameGroup::easc ||
                  group == pal4::GameDataResourceNameGroup::easd ||
                  group == pal4::GameDataResourceNameGroup::ease ||
                  group == pal4::GameDataResourceNameGroup::easf ||
                  group == pal4::GameDataResourceNameGroup::easg ||
                  group == pal4::GameDataResourceNameGroup::eash ||
                  group == pal4::GameDataResourceNameGroup::easi ||
                  group == pal4::GameDataResourceNameGroup::east ||
                  group == pal4::GameDataResourceNameGroup::aca ||
                  group == pal4::GameDataResourceNameGroup::apa ||
                  group == pal4::GameDataResourceNameGroup::upgrade_parameters ||
                  group == pal4::GameDataResourceNameGroup::equipment_attributes ||
                  group == pal4::GameDataResourceNameGroup::equipment_attribute_text ||
                  group == pal4::GameDataResourceNameGroup::script_ids,
              "known cleanup group");
        calls.emplace_back("register");
        return -44;
    }
    void DestroyBuffer(std::uint32_t address) override {
        destroyed.push_back(address);
    }
    std::vector<Mode> initialized_modes;
};

template <std::size_t N>
void CheckNames(const std::vector<Record>& records, std::uint32_t base,
                const std::array<std::string_view, N>& names,
                const std::array<Mode, N>& modes) {
    Check(records.size() == N, "exact resource name count");
    for (std::size_t i = 0; i < N; ++i)
        Check(records[i].address == base + i * 16 &&
                  records[i].text == names[i] && records[i].mode == modes[i],
              "resource address, exact literal and assignment mode");
}

void CheckEquipment() {
    Runtime runtime;
    constexpr std::array<std::string_view, 6> names{
        "ERES_WEAPON", "ERES_HELMENT", "ERES_CORSELET",
        "ERES_LEGHARNESS", "ERES_ORNAMENT1", "ERES_ORNAMENT2"};
    constexpr std::array<Mode, 6> modes{
        Mode::bare_direct, Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated, Mode::reset_direct};
    Check(pal4::GameDataResourceNames::InitializeEquipment(0x5a, runtime) == 6,
          "initializer forwards final string result");
    CheckNames(runtime.assigned, 0x8ee578, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterEquipment(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 6,
          "wrapper registers cleanup after all six strings");
    pal4::GameDataResourceNames::DestroyEquipment(runtime);
    for (std::size_t i = 0; i < 6; ++i)
        Check(runtime.destroyed[i] == 0x8ee578 + (5 - i) * 16,
              "equipment array destroys in reverse order");
}

void CheckCharacter() {
    Runtime runtime;
    constexpr std::array<std::string_view, 8> names{
        "CA_MAX_HP", "CA_ADDITIONAL_RAGE", "CA_MAX_MP", "CA_PHYSICAL",
        "CA_TOUGHNESS", "CA_SPEED", "CA_LUCKY", "CA_WILL"};
    constexpr std::array<Mode, 8> modes{
        Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_direct, Mode::reset_direct};
    Check(pal4::GameDataResourceNames::InitializeCharacter(0x5a, runtime) == 8,
          "character initializer forwards final string result");
    CheckNames(runtime.assigned, 0x8ee4f8, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterCharacter(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 8,
          "character wrapper registers after all eight strings");
    pal4::GameDataResourceNames::DestroyCharacter(runtime);
    for (std::size_t i = 0; i < 8; ++i)
        Check(runtime.destroyed[i] == 0x8ee4f8 + (7 - i) * 16,
              "character array destroys in reverse order");
}

void CheckCharacterExtra() {
    Runtime runtime;
    constexpr std::array<std::string_view, 4> names{
        "CA_ADDITIONAL_CRITICAL", "CA_FEND_OFF",
        "CA_ADDITIONAL_HITTING", "CA_PAY_FOR_SPARE"};
    constexpr std::array<Mode, 4> modes{
        Mode::bare_allocated, Mode::bare_direct,
        Mode::bare_allocated, Mode::bare_direct};
    Check(pal4::GameDataResourceNames::InitializeCharacterExtra(
              0x5a, runtime) == 4,
          "extra character initializer forwards last assignment result");
    CheckNames(runtime.assigned, 0x8ee4b8, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterCharacterExtra(
              0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 4,
          "extra character wrapper registers after four strings");
    pal4::GameDataResourceNames::DestroyCharacterExtra(runtime);
    for (std::size_t i = 0; i < 4; ++i)
        Check(runtime.destroyed[i] == 0x8ee4b8 + (3 - i) * 16,
              "extra character array destroys in reverse order");
}

void CheckAttack() {
    Runtime runtime;
    constexpr std::array<std::string_view, 6> names{
        "CA_ATTACK_PHYSICAL", "CA_ATTACK_WATER", "CA_ATTACK_FIRE",
        "CA_ATTACK_THUNDER", "CA_ATTACK_AIR", "CA_ATTACK_EARTH"};
    constexpr std::array<Mode, 6> modes{
        Mode::bare_direct, Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated, Mode::reset_direct};
    Check(pal4::GameDataResourceNames::InitializeAttack(0x5a, runtime) == 6,
          "attack initializer forwards last assignment result");
    CheckNames(runtime.assigned, 0x8ee458, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterAttack(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 6,
          "attack wrapper registers after six strings");
    pal4::GameDataResourceNames::DestroyAttack(runtime);
    for (std::size_t i = 0; i < 6; ++i)
        Check(runtime.destroyed[i] == 0x8ee458 + (5 - i) * 16,
              "attack names destroy in reverse order");
}

void CheckProgression() {
    Runtime runtime;
    constexpr std::array<std::string_view, 10> names{
        "CA_LOSABLE_PROPERTY", "CA_THE_LOSABLE_NUMBER",
        "CA_LOSABLE_MONEY", "CA_MULTI_HIT", "CA_LEVEL",
        "CA_EXPERIENCE", "CA_MAGIC_POINT", "CA_HP", "CA_RAGE", "CA_MP"};
    constexpr std::array<Mode, 10> modes{
        Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_direct, Mode::reset_direct};
    Check(pal4::GameDataResourceNames::InitializeProgression(0x5a, runtime) ==
              10,
          "progression initializer forwards last assignment result");
    CheckNames(runtime.assigned, 0x8ee3b8, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterProgression(0x5a, runtime) ==
                  -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 10,
          "progression wrapper registers after ten strings");
    pal4::GameDataResourceNames::DestroyProgression(runtime);
    for (std::size_t i = 0; i < 10; ++i)
        Check(runtime.destroyed[i] == 0x8ee3b8 + (9 - i) * 16,
              "progression names destroy in reverse order");
}

void CheckCombatParameters() {
    Runtime runtime;
    constexpr std::array<std::string_view, 5> names{
        "CA_MAGIC_ATTACK_PARAM", "CA_PHYSICAL_ATTACK_PARAM",
        "CA_HP_REGAIN_PARAM", "CA_MP_REGAIN_PARAM",
        "CA_COUNTERPUNCH_RATE"};
    constexpr std::array<Mode, 5> modes{
        Mode::bare_direct, Mode::bare_direct, Mode::bare_direct,
        Mode::reset_allocated, Mode::reset_allocated};
    runtime.results[0x8ee3a8] = 0x1234;
    Check(pal4::GameDataResourceNames::InitializeCombatParameters(
              0x5a, runtime) == 0x34,
          "combat parameter initializer returns only low byte");
    CheckNames(runtime.assigned, 0x8ee368, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterCombatParameters(
              0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 5,
          "combat parameter wrapper registers after five strings");
    pal4::GameDataResourceNames::DestroyCombatParameters(runtime);
    for (std::size_t i = 0; i < 5; ++i)
        Check(runtime.destroyed[i] == 0x8ee368 + (4 - i) * 16,
              "combat parameter names destroy in reverse order");
}

void CheckSpecialItemEffects() {
    Runtime runtime;
    constexpr std::array<std::string_view, 2> names{
        "ESI_SIPHON", "ESI_MORTAL"};
    constexpr std::array<Mode, 2> modes{
        Mode::bare_allocated, Mode::bare_std_assign};
    runtime.results[0x8ee358] = 0xabcd;
    Check(pal4::GameDataResourceNames::InitializeSpecialItemEffects(
              0x5a, runtime) == 0xcd,
          "special item effect returns low byte of final string result");
    CheckNames(runtime.assigned, 0x8ee348, names, modes);
    runtime = Runtime{};
    runtime.results[0x8ee358] = 0;
    Check(pal4::GameDataResourceNames::InitializeSpecialItemEffects(
              0x5a, runtime) == 0,
          "failed special string assignment forwards zero");
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterSpecialItemEffects(
              0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 2,
          "special item wrapper registers after two strings");
    pal4::GameDataResourceNames::DestroySpecialItemEffects(runtime);
    Check(runtime.destroyed ==
              std::vector<std::uint32_t>{0x8ee358, 0x8ee348},
          "special item effects destroy in reverse order");
}

void CheckElementalAttributes() {
    Runtime runtime;
    constexpr std::array<std::string_view, 6> names{
        "EASA_NONE", "EASA_WATER", "EASA_FIRE",
        "EASA_THUNDER", "EASA_AIR", "EASA_EARTH"};
    constexpr std::array<Mode, 6> modes{
        Mode::bare_direct, Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated, Mode::reset_direct};
    Check(pal4::GameDataResourceNames::InitializeElementalAttributes(
              0x5a, runtime) == 6,
          "elemental initializer forwards final direct assignment result");
    CheckNames(runtime.assigned, 0x8ee2e8, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterElementalAttributes(
              0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 6,
          "elemental wrapper registers after six strings");
    pal4::GameDataResourceNames::DestroyElementalAttributes(runtime);
    for (std::size_t i = 0; i < 6; ++i)
        Check(runtime.destroyed[i] == 0x8ee2e8 + (5 - i) * 16,
              "elemental attributes destroy in reverse order");
}

void CheckEasb() {
    Runtime runtime;
    constexpr std::array<std::string_view, 7> names{
        "EASB_NONE", "EASB_BONDAGE", "EASB_SEAL", "EASB_FORBIDEN",
        "EASB_SLEEP", "EASB_MAD", "EASB_DISORDER"};
    constexpr std::array<Mode, 7> modes{
        Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated, Mode::reset_direct};
    Check(pal4::GameDataResourceNames::InitializeEasb(0x5a, runtime) == 7,
          "EASB initializer forwards final direct assignment result");
    CheckNames(runtime.assigned, 0x8ee278, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterEasb(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 7,
          "EASB wrapper registers after seven strings");
    pal4::GameDataResourceNames::DestroyEasb(runtime);
    for (std::size_t i = 0; i < 7; ++i)
        Check(runtime.destroyed[i] == 0x8ee278 + (6 - i) * 16,
              "EASB names destroy in reverse order");
}

void CheckEasc() {
    Runtime runtime;
    constexpr std::array<std::string_view, 7> names{
        "EASC_NONE", "EASC_REFLECTION", "EASC_WALL", "EASC_BOUND",
        "EASC_CONCEALED", "EASC_TRANSFORMING", "EASC_POISON_PROOF"};
    constexpr std::array<Mode, 7> modes{
        Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated, Mode::reset_direct};
    Check(pal4::GameDataResourceNames::InitializeEasc(0x5a, runtime) == 7,
          "EASC initializer forwards final direct assignment result");
    CheckNames(runtime.assigned, 0x8ee208, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterEasc(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 7,
          "EASC wrapper registers after seven strings");
    pal4::GameDataResourceNames::DestroyEasc(runtime);
    for (std::size_t i = 0; i < 7; ++i)
        Check(runtime.destroyed[i] == 0x8ee208 + (6 - i) * 16,
              "EASC names destroy in reverse order");
}

void CheckEasd() {
    Runtime runtime;
    constexpr std::array<std::string_view, 2> names{
        "EASD_NONE", "EASD_COMBO"};
    constexpr std::array<Mode, 2> modes{
        Mode::bare_allocated, Mode::bare_std_assign};
    runtime.results[0x8ee1f8] = 0x4567;
    Check(pal4::GameDataResourceNames::InitializeEasd(0x5a, runtime) == 0x67,
          "EASD initializer returns only final string result low byte");
    CheckNames(runtime.assigned, 0x8ee1e8, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterEasd(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 2,
          "EASD wrapper registers after two strings");
    pal4::GameDataResourceNames::DestroyEasd(runtime);
    Check(runtime.destroyed ==
              std::vector<std::uint32_t>{0x8ee1f8, 0x8ee1e8},
          "EASD names destroy in reverse order");
}

void CheckEase() {
    Runtime runtime;
    constexpr std::array<std::string_view, 4> names{
        "EASE_NONE", "EASE_WEAK", "EASE_DANGEROUS", "EASE_DEAD"};
    constexpr std::array<Mode, 4> modes{
        Mode::bare_allocated, Mode::bare_direct,
        Mode::bare_allocated, Mode::bare_direct};
    Check(pal4::GameDataResourceNames::InitializeEase(0x5a, runtime) == 4,
          "EASE initializer returns final direct assignment result");
    CheckNames(runtime.assigned, 0x8ee1a8, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterEase(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 4,
          "EASE wrapper registers after four strings");
    pal4::GameDataResourceNames::DestroyEase(runtime);
    for (std::size_t i = 0; i < 4; ++i)
        Check(runtime.destroyed[i] == 0x8ee1a8 + (3 - i) * 16,
              "EASE names destroy in reverse order");
}

void CheckEasf() {
    Runtime runtime;
    constexpr std::array<std::string_view, 2> names{
        "EASF_NONE", "EASF_ETERNAL"};
    constexpr std::array<Mode, 2> modes{
        Mode::bare_allocated, Mode::bare_std_assign};
    runtime.results[0x8ee198] = 0x5689;
    Check(pal4::GameDataResourceNames::InitializeEasf(0x5a, runtime) == 0x89,
          "EASF initializer returns only final string result low byte");
    CheckNames(runtime.assigned, 0x8ee188, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterEasf(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 2,
          "EASF wrapper registers after two strings");
    pal4::GameDataResourceNames::DestroyEasf(runtime);
    Check(runtime.destroyed ==
              std::vector<std::uint32_t>{0x8ee198, 0x8ee188},
          "EASF names destroy in reverse order");
}

void CheckEasg() {
    Runtime runtime;
    constexpr std::array<std::string_view, 5> names{
        "EASG_NONE", "EASG_DODGE", "EASG_FENDOF",
        "EASG_HITTED_MORTALLY", "EASG_CRITICAL"};
    constexpr std::array<Mode, 5> modes{
        Mode::bare_direct, Mode::bare_direct, Mode::bare_direct,
        Mode::reset_allocated, Mode::reset_allocated};
    runtime.results[0x8ee178] = 0x2356;
    Check(pal4::GameDataResourceNames::InitializeEasg(0x5a, runtime) == 0x56,
          "EASG initializer returns only allocation result low byte");
    CheckNames(runtime.assigned, 0x8ee138, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterEasg(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 5,
          "EASG wrapper registers after five strings");
    pal4::GameDataResourceNames::DestroyEasg(runtime);
    for (std::size_t i = 0; i < 5; ++i)
        Check(runtime.destroyed[i] == 0x8ee138 + (4 - i) * 16,
              "EASG names destroy in reverse order");
}

void CheckEash() {
    Runtime runtime;
    constexpr std::array<std::string_view, 3> names{
        "EASH_NONE", "EASH_DEFEND", "EASH_FLEE_FAIL"};
    constexpr std::array<Mode, 3> modes{
        Mode::bare_allocated, Mode::bare_allocated, Mode::bare_allocated};
    runtime.results[0x8ee128] = 0x678a;
    Check(pal4::GameDataResourceNames::InitializeEash(0x5a, runtime) == 0x8a,
          "EASH initializer returns only allocation result low byte");
    CheckNames(runtime.assigned, 0x8ee108, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterEash(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 3,
          "EASH wrapper registers after three strings");
    pal4::GameDataResourceNames::DestroyEash(runtime);
    Check(runtime.destroyed == std::vector<std::uint32_t>{
              0x8ee128, 0x8ee118, 0x8ee108},
          "EASH names destroy in reverse order");
}

void CheckEasi() {
    Runtime runtime;
    constexpr std::array<std::string_view, 6> names{
        "EASI_NONE", "EASI_SKILL1", "EASI_SKILL2",
        "EASI_SKILL3", "EASI_SKILL4", "EASI_SKILL5"};
    constexpr std::array<Mode, 6> modes{
        Mode::bare_direct, Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated, Mode::reset_direct};
    Check(pal4::GameDataResourceNames::InitializeEasi(0x5a, runtime) == 6,
          "EASI initializer returns final direct assignment result");
    CheckNames(runtime.assigned, 0x8ee0a8, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterEasi(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 6,
          "EASI wrapper registers after six strings");
    pal4::GameDataResourceNames::DestroyEasi(runtime);
    for (std::size_t i = 0; i < 6; ++i)
        Check(runtime.destroyed[i] == 0x8ee0a8 + (5 - i) * 16,
              "EASI names destroy in reverse order");
}

void CheckEast() {
    Runtime runtime;
    constexpr std::array<std::string_view, 9> names{
        "EAST_A", "EAST_B", "EAST_C", "EAST_D", "EAST_E",
        "EAST_F", "EAST_G", "EAST_H", "EAST_I"};
    constexpr std::array<Mode, 9> modes{
        Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_direct, Mode::reset_direct, Mode::reset_direct};
    Check(pal4::GameDataResourceNames::InitializeEast(0x5a, runtime) == 9,
          "EAST initializer returns final direct assignment result");
    CheckNames(runtime.assigned, 0x8ee018, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterEast(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 9,
          "EAST wrapper registers after nine strings");
    pal4::GameDataResourceNames::DestroyEast(runtime);
    for (std::size_t i = 0; i < 9; ++i)
        Check(runtime.destroyed[i] == 0x8ee018 + (8 - i) * 16,
              "EAST names destroy in reverse order");
}

void CheckAca() {
    Runtime runtime;
    constexpr std::array<std::string_view, 13> names{
        "ACA_SELF_IN_DANGER", "ACA_MIN_SELF_HP", "ACA_MAX_SELF_HP",
        "ACA_MATE_IN_DANGER", "ACA_MATE_DEAD", "ACA_MIN_MATE_HP",
        "ACA_MAX_MATE_HP", "ACA_ENEMY_DEAD", "ACA_ENEMY_IN_DANGER",
        "ACA_MIN_ENEMY_HP", "ACA_MAX_ENEMY_HP", "ACA_BOUT_COUNT",
        "ACA_BOUT_TIMES"};
    constexpr std::array<Mode, 13> modes{
        Mode::reset_allocated, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_allocated, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_allocated};
    runtime.results[0x8ee7d0] = 0x78ab;
    Check(pal4::GameDataResourceNames::InitializeAca(0x5a, runtime) == 0xab,
          "ACA initializer returns last allocation result low byte");
    CheckNames(runtime.assigned, 0x8ee710, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterAca(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 13,
          "ACA wrapper registers after thirteen strings");
    pal4::GameDataResourceNames::DestroyAca(runtime);
    for (std::size_t i = 0; i < 13; ++i)
        Check(runtime.destroyed[i] == 0x8ee710 + (12 - i) * 16,
              "ACA names destroy in reverse order");
}

void CheckApa() {
    Runtime runtime;
    constexpr std::array<std::string_view, 13> names{
        "APA_ASSIGN_SKILL", "APA_ASSIGN_TARGET", "APA_CURE_RATE",
        "APA_SPECIAL_ATK_RATE", "APA_OTHER_RATE", "APA_NORMAL_ATK_RATE",
        "APA_FLEE_RATE", "APA_SKILL_RATE", "APA_SUMMON_RATE",
        "APA_REVIVE_RATE", "APA_FOR_ASSIGN_TARGET",
        "APA_FOR_THE_DANGEROUS", "APA_FOR_RANDOM"};
    constexpr std::array<Mode, 13> modes{
        Mode::reset_allocated, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_allocated, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_allocated};
    runtime.results[0x8ee700] = 0x89cd;
    Check(pal4::GameDataResourceNames::InitializeApa(0x5a, runtime) == 0xcd,
          "APA initializer returns last allocation result low byte");
    CheckNames(runtime.assigned, 0x8ee640, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterApa(0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 13,
          "APA wrapper registers after thirteen strings");
    pal4::GameDataResourceNames::DestroyApa(runtime);
    for (std::size_t i = 0; i < 13; ++i)
        Check(runtime.destroyed[i] == 0x8ee640 + (12 - i) * 16,
              "APA names destroy in reverse order");
}

void CheckUpgradeParameters() {
    Runtime runtime;
    constexpr std::array<std::string_view, 22> names{
        "HpUpgradeParam1", "HpUpgradeParam2", "HpUpgradeParam3",
        "HpUpgradeParam4", "MpUpgradeParam1", "MpUpgradeParam2",
        "MpUpgradeParam3", "PhysicalUpgradeParam1",
        "PhysicalUpgradeParam2", "PhysicalUpgradeParam3",
        "ToughnessUpgradeParam1", "ToughnessUpgradeParam2",
        "ToughnessUpgradeParam3", "SpeedUpgradeParam1",
        "SpeedUpgradeParam2", "SpeedUpgradeParam3",
        "LuckyUpgradeParam1", "LuckyUpgradeParam2",
        "LuckyUpgradeParam3", "WillUpgradeParam1",
        "WillUpgradeParam2", "WillUpgradeParam3"};
    constexpr std::array<Mode, 22> modes{
        Mode::reset_direct, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_direct, Mode::mode_cstring, Mode::mode_cstring,
        Mode::mode_cstring, Mode::mode_cstring, Mode::mode_cstring,
        Mode::mode_cstring};
    Check(pal4::GameDataResourceNames::InitializeUpgradeParameters(
              0x5a, runtime) == 0x8ee9a0,
          "upgrade initializer returns the final buffer address");
    CheckNames(runtime.assigned, 0x8ee850, names, modes);
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterUpgradeParameters(
              0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 22,
          "upgrade wrapper registers cleanup after all 22 names");
    pal4::GameDataResourceNames::DestroyUpgradeParameters(runtime);
    Check(runtime.destroyed.size() == 22,
          "upgrade destructor visits every buffer");
    for (std::size_t i = 0; i < 22; ++i)
        Check(runtime.destroyed[i] == 0x8ee850 + (21 - i) * 16,
              "upgrade names destroy in reverse address order");
}

void CheckEquipmentAttributes() {
    Runtime runtime;
    constexpr std::array<std::string_view, 6> names{
        "EA_PRICE", "EA_LING_CAPACITY", "EA_SKILL_ID",
        "EA_FORGE_POTENTIAL", "EA_INT_TEMP1", "EA_INT_TEMP2"};
    constexpr std::array<Mode, 6> modes{
        Mode::bare_direct, Mode::reset_allocated, Mode::reset_allocated,
        Mode::reset_allocated, Mode::reset_allocated, Mode::reset_direct};
    Check(pal4::GameDataResourceNames::InitializeEquipmentAttributes(
              0x5a, runtime) == 6,
          "EA initializer forwards final text result");
    CheckNames(runtime.assigned, 0x8eeac0, names, modes);
    runtime = Runtime{};
    runtime.results[0x8eeb10] = -17;
    Check(pal4::GameDataResourceNames::InitializeEquipmentAttributes(
              0x5a, runtime) == -17,
          "EA initializer forwards signed final result");
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterEquipmentAttributes(
              0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 6,
          "EA wrapper registers cleanup after six names");
    pal4::GameDataResourceNames::DestroyEquipmentAttributes(runtime);
    Check(runtime.destroyed.size() == 6,
          "EA destructor visits every text buffer");
    for (std::size_t i = 0; i < 6; ++i)
        Check(runtime.destroyed[i] == 0x8eeac0 + (5 - i) * 16,
              "EA destructor runs in reverse address order");
}

void CheckEquipmentAttributeText() {
    Runtime runtime;
    constexpr std::array<std::string_view, 13> names{
        "EA_NAME", "EA_DESCRIPTION", "EA_LING_INFO", "EA_EF1",
        "EA_MODEL", "EA_TEXTURE", "EA_EF2", "EA_LING_NAME",
        "EA_LHAND", "EA_RHAND", "EA_EF3", "EA_STR_TEMP1",
        "EA_STR_TEMP2"};
    constexpr std::array<Mode, 13> modes{
        Mode::reset_allocated, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_allocated, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_direct, Mode::reset_direct, Mode::reset_direct,
        Mode::reset_allocated};
    Check(pal4::GameDataResourceNames::InitializeEquipmentAttributeText(
              0x5a, runtime) == 13,
          "13 EA names return final assignment byte");
    CheckNames(runtime.assigned, 0x8ee9f0, names, modes);
    runtime = Runtime{};
    runtime.results[0x8eeab0] = -17;
    Check(pal4::GameDataResourceNames::InitializeEquipmentAttributeText(
              0x5a, runtime) == 0xef,
          "EA text return truncates signed final result to a byte");
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterEquipmentAttributeText(
              0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 13,
          "13 EA names precede CRT cleanup registration");
    pal4::GameDataResourceNames::DestroyEquipmentAttributeText(runtime);
    Check(runtime.destroyed.size() == 13,
          "all 13 EA buffers are destroyed");
    for (std::size_t i = 0; i < 13; ++i)
        Check(runtime.destroyed[i] == 0x8ee9f0 + (12 - i) * 16,
              "13 EA buffers destroy in reverse address order");
}

void CheckScriptIds() {
    Runtime runtime;
    constexpr std::array<std::string_view, 5> names{
        "SCP_ID_1", "SCP_ID_2", "SCP_ID_3", "SCP_ID_4", "SCP_ID_5"};
    constexpr std::array<Mode, 5> modes{
        Mode::bare_direct, Mode::bare_direct, Mode::bare_direct,
        Mode::reset_allocated, Mode::reset_allocated};
    Check(pal4::GameDataResourceNames::InitializeScriptIds(
              0x5a, runtime) == 5,
          "SCP_ID initializer forwards final allocation byte");
    CheckNames(runtime.assigned, 0x8eeb90, names, modes);
    runtime = Runtime{};
    runtime.results[0x8eebd0] = -17;
    Check(pal4::GameDataResourceNames::InitializeScriptIds(
              0x5a, runtime) == 0xef,
          "SCP_ID initializer truncates final result to a byte");
    runtime = Runtime{};
    Check(pal4::GameDataResourceNames::RegisterScriptIds(
              0x5a, runtime) == -44 &&
              runtime.calls.back() == "register" &&
              runtime.assigned.size() == 5,
          "SCP_ID registration follows all five assignments");
    pal4::GameDataResourceNames::DestroyScriptIds(runtime);
    Check(runtime.destroyed.size() == 5,
          "SCP_ID destructor visits five text records");
    for (std::size_t i = 0; i < 5; ++i)
        Check(runtime.destroyed[i] == 0x8eeb90 + (4 - i) * 16,
              "SCP_ID text records destroy in reverse order");
}

}  // namespace

int main() {
    try {
        Check(pal4::GameDataResourceNames::NameAt(0x8ee710u)=="ACA_SELF_IN_DANGER" &&
              pal4::GameDataResourceNames::NameAt(0x8ee7d0u)=="ACA_BOUT_TIMES" &&
              pal4::GameDataResourceNames::NameAt(0x8ee640u)=="APA_ASSIGN_SKILL" &&
              pal4::GameDataResourceNames::NameAt(0x8ee700u)=="APA_FOR_RANDOM" &&
              !pal4::GameDataResourceNames::NameAt(0x8ee711u),"native AI field names and aligned lookup boundaries");
        CheckEquipment();
        CheckCharacter();
        CheckCharacterExtra();
        CheckAttack();
        CheckProgression();
        CheckCombatParameters();
        CheckSpecialItemEffects();
        CheckElementalAttributes();
        CheckEasb();
        CheckEasc();
        CheckEasd();
        CheckEase();
        CheckEasf();
        CheckEasg();
        CheckEash();
        CheckEasi();
        CheckEast();
        CheckAca();
        CheckApa();
        CheckUpgradeParameters();
        CheckEquipmentAttributes();
        CheckEquipmentAttributeText();
        CheckScriptIds();
        std::cout << "game_data_resource_names_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
