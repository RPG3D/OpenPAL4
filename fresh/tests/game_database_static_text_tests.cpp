#include "pal4/game_database_static_text.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

using Init = pal4::GameDatabaseTextInit;
using Assign = pal4::GameDatabaseTextAssign;

struct Call {
    std::string operation;
    std::uint32_t address = 0u;
    std::string literal;
    Init initialization = Init::ClearWords;
    Assign assignment = Assign::SetExceptionMessage;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDatabaseStaticTextRuntime {
    std::vector<Call> calls;
    std::vector<std::int32_t> assign_results;
    std::size_t next_assignment = 0u;
    int atexit_result = -23;

    void InitializeText(const std::uint32_t address,
                        const Init mode) override {
        calls.push_back({"initialize", address, {}, mode});
    }
    std::int32_t Assign(const std::uint32_t address,
                        const std::string_view literal,
                        const pal4::GameDatabaseTextAssign mode) override {
        calls.push_back({"assign", address, std::string(literal),
                         Init::ClearWords, mode});
        return assign_results.at(next_assignment++);
    }
    void DestroyRange(const std::uint32_t first,
                      const std::uint32_t stride,
                      const std::uint32_t count,
                      const std::uint32_t element_destructor) override {
        Check(stride == 16u && element_destructor == 0x51f9b0u,
              "GameDB text cleanup uses 16-byte records and original callback");
        calls.push_back({"destroy", first, std::to_string(count)});
    }
    int RegisterAtExit(const std::uint32_t callback) override {
        calls.push_back({"atexit", callback});
        return atexit_result;
    }
};

struct ExpectedText {
    std::string_view literal;
    Init initialization;
    Assign assignment;
};

void CheckGroup(const Runtime& runtime, const std::uint32_t first,
                const auto& expected) {
    for (std::size_t i = 0u; i < expected.size(); ++i) {
        const auto address = first + static_cast<std::uint32_t>(i) * 16u;
        Check(runtime.calls.at(2u * i) ==
                  Call{"initialize", address, {},
                       expected[i].initialization},
              "GameDB text initialization address and mode");
        Check(runtime.calls.at(2u * i + 1u) ==
                  Call{"assign", address,
                       std::string(expected[i].literal),
                       Init::ClearWords, expected[i].assignment},
              "GameDB text literal and assignment mode");
    }
}

constexpr std::array<ExpectedText, 5> kSound{{
    {"SOUND_WOUNDED1", Init::ClearWords, Assign::SetExceptionMessage},
    {"SOUND_WOUNDED2", Init::ClearWords, Assign::SetExceptionMessage},
    {"SOUND_WOUNDED3", Init::ClearWords, Assign::SetExceptionMessage},
    {"SOUND_WOUNDED4", Init::ResetFlag, Assign::CopyAndTerminate},
    {"SOUND_WOUNDED5", Init::ResetFlag, Assign::CopyAndTerminate},
}};

constexpr std::array<ExpectedText, 5> kEffect{{
    {"EFE_WATER", Init::ClearWords, Assign::SetExceptionMessage},
    {"EFE_FIRE", Init::ClearWords, Assign::SetExceptionMessage},
    {"EFE_THUNDER", Init::ClearWords, Assign::SetExceptionMessage},
    {"EFE_AIR", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EFE_EARTH", Init::ResetFlag, Assign::CopyAndTerminate},
}};

constexpr std::array<ExpectedText, 4> kMap{{
    {"ePT_TIANHE", Init::ClearWords, Assign::CopyAndSetSize},
    {"ePT_LINSHA", Init::ClearWords, Assign::SetExceptionMessage},
    {"ePT_MENGLI", Init::ClearWords, Assign::CopyAndSetSize},
    {"ePT_ZIYING", Init::ClearWords, Assign::SetExceptionMessage},
}};

constexpr std::array<ExpectedText, 7> kEquipment{{
    {"EET_SWORD", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EET_DSWORD", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EET_INSTRUMENT", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EET_HELMET", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EET_CORSELET", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EET_LEGHARNESS", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EET_ORNAMENT", Init::ResetFlag, Assign::SetExceptionMessage},
}};

constexpr std::array<ExpectedText, 5> kEquipmentCategory{{
    {"EEC_WEAPON", Init::ClearWords, Assign::SetExceptionMessage},
    {"EEC_HELMENT", Init::ClearWords, Assign::SetExceptionMessage},
    {"EEC_CORSELET", Init::ClearWords, Assign::SetExceptionMessage},
    {"EEC_LEGHARNESS", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EEC_ORNAMENT", Init::ResetFlag, Assign::CopyAndTerminate},
}};

constexpr std::array<ExpectedText, 3> kEquipmentProcess{{
    {"EPT_MELT", Init::ClearWords, Assign::CopyAndTerminate},
    {"EPT_FORGING", Init::ClearWords, Assign::CopyAndTerminate},
    {"EPT_ADD_MAGIC", Init::ClearWords, Assign::CopyAndTerminate},
}};

constexpr std::array<ExpectedText, 5> kItemCategory{{
    {"EPC_CURE", Init::ClearWords, Assign::SetExceptionMessage},
    {"EPC_ATTACKABLE", Init::ClearWords, Assign::SetExceptionMessage},
    {"EPC_AUXILIARY", Init::ClearWords, Assign::SetExceptionMessage},
    {"EPC_MATERIAL", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EPC_SCENARIO", Init::ResetFlag, Assign::CopyAndTerminate},
}};

constexpr std::array<ExpectedText, 4> kShopType{{
    {"EST_FORGE", Init::ClearWords, Assign::CopyAndSetSize},
    {"EST_GROCERY", Init::ClearWords, Assign::SetExceptionMessage},
    {"EST_DRUGSTORE", Init::ClearWords, Assign::CopyAndSetSize},
    {"EST_EATERY", Init::ClearWords, Assign::SetExceptionMessage},
}};

constexpr std::array<ExpectedText, 3> kItemType{{
    {"EGT_EQUIPMENT", Init::ClearWords, Assign::CopyAndTerminate},
    {"EGT_PROPERTY", Init::ClearWords, Assign::CopyAndTerminate},
    {"EGT_PREDESCRIPTION", Init::ClearWords, Assign::CopyAndTerminate},
}};

constexpr std::array<ExpectedText, 7> kRace{{
    {"ERR_HUMAN", Init::ResetFlag, Assign::CopyAndTerminate},
    {"ERR_DEMON", Init::ResetFlag, Assign::CopyAndTerminate},
    {"ERR_FAIRY", Init::ResetFlag, Assign::CopyAndTerminate},
    {"ERR_GHOST", Init::ResetFlag, Assign::CopyAndSetSize},
    {"ERR_IMMORTAL", Init::ResetFlag, Assign::CopyAndSetSize},
    {"ERR_SPIRIT", Init::ResetFlag, Assign::CopyAndSetSize},
    {"ERR_BEAST", Init::ResetFlag, Assign::SetExceptionMessage},
}};

constexpr std::array<ExpectedText, 8> kEat{{
    {"EATS_SELF_IN_DANGER", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EATS_SELF_WOUNDED", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EATS_TEAMMATE_SLAUGHTERED", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EATS_TEAMMATE_IN_DANGER", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EATS_TEAMMATE_WOUNDED", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EATS_NORMAL", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EATS_BOUT_COUNT", Init::ResetFlag, Assign::SetExceptionMessage},
    {"EATS_BOUT_TIMES", Init::ResetFlag, Assign::SetExceptionMessage},
}};

constexpr std::array<ExpectedText, 4> kEnemyAction{{
    {"EAES_ENEMY_SLAUGHTERED", Init::ClearWords, Assign::CopyAndSetSize},
    {"EAES_ENEMY_IN_DANGER", Init::ClearWords, Assign::SetExceptionMessage},
    {"EAES_ENEMY_WOUNDED", Init::ClearWords, Assign::CopyAndSetSize},
    {"EAES_NORMAL", Init::ClearWords, Assign::SetExceptionMessage},
}};

constexpr std::array<ExpectedText, 6> kCombatPosition{{
    {"ECP_FIRST", Init::ClearWords, Assign::SetExceptionMessage},
    {"ECP_SECOND", Init::ResetFlag, Assign::CopyAndTerminate},
    {"ECP_THIRD", Init::ResetFlag, Assign::CopyAndTerminate},
    {"ECP_FOURTH", Init::ResetFlag, Assign::CopyAndTerminate},
    {"ECP_FIFTH", Init::ResetFlag, Assign::CopyAndSetSize},
    {"ECP_SIXTH", Init::ResetFlag, Assign::SetExceptionMessage},
}};

constexpr std::array<ExpectedText, 4> kItemUsage{{
    {"EST_USE", Init::ClearWords, Assign::CopyAndSetSize},
    {"EST_COMBAT", Init::ClearWords, Assign::SetExceptionMessage},
    {"EST_MAP", Init::ClearWords, Assign::CopyAndSetSize},
    {"EST_COMMON", Init::ClearWords, Assign::SetExceptionMessage},
}};

constexpr std::array<ExpectedText, 7> kAction{{
    {"EACT_ATTACK", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EACT_AUXILIARY", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EACT_CURE", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EACT_PASSIVE", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EACT_SUMMON", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EACT_REVIVE", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EACT_UNUSABLE", Init::ResetFlag, Assign::SetExceptionMessage},
}};

constexpr std::array<ExpectedText, 3> kActionState{{
    {"ESAT_STAY", Init::ClearWords, Assign::CopyAndTerminate},
    {"ESAT_MOVE", Init::ClearWords, Assign::CopyAndTerminate},
    {"EAST_TRANSLATE", Init::ClearWords, Assign::CopyAndTerminate},
}};

constexpr std::array<ExpectedText, 15> kSkillEffectText{{
    {"ESTT_HITTED_BY_MAGIC", Init::ResetFlag, Assign::SetExceptionMessage},
    {"ESTT_EVER_LASTING", Init::ResetFlag, Assign::SetExceptionMessage},
    {"ESTT_USE", Init::ResetFlag, Assign::SetExceptionMessage},
    {"ESTT_HITTED", Init::ResetFlag, Assign::SetExceptionMessage},
    {"ESTT_HIT_TARGET", Init::ResetFlag, Assign::SetExceptionMessage},
    {"ESTT_RESERVED", Init::ResetFlag, Assign::SetExceptionMessage},
    {"ESTT_BOUT", Init::ResetFlag, Assign::SetExceptionMessage},
    {"ESTT_HITTED_BY_WATER", Init::ResetFlag, Assign::SetExceptionMessage},
    {"ESTT_HITTED_BY_FIRE", Init::ResetFlag, Assign::SetExceptionMessage},
    {"ESTT_HITTED_BY_THUNDER", Init::ResetFlag, Assign::SetExceptionMessage},
    {"ESTT_HITTED_BY_AIR", Init::ResetFlag, Assign::SetExceptionMessage},
    {"ESTT_HITTED_BY_EARTH", Init::ResetFlag, Assign::SetExceptionMessage},
    {"ESTT_HITTED_AND_REACT", Init::ResetFlag, Assign::CopyAndSetSize},
    {"ESTT_HIT_TARGET_FOR_SELF", Init::ResetFlag, Assign::SetExceptionMessage},
    {"ESTT_ON_ATTACK", Init::ResetFlag, Assign::SetExceptionMessage},
}};

constexpr std::array<ExpectedText, 6> kEatText{{
    {"EAT_SELF", Init::ClearWords, Assign::SetExceptionMessage},
    {"EAT_SINGLE_TEAMMATE", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EAT_SINGLE_ENEMY", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EAT_WHOLE_TEAMMATE", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EAT_WHOLE_ENEMY", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EAT_DESIGNATE", Init::ResetFlag, Assign::SetExceptionMessage},
}};

constexpr std::array<ExpectedText, 2> kActionPattern{{
    {"EPAT_SINGLE", Init::ClearWords, Assign::CopyAndTerminate},
    {"EPAT_TEAM", Init::ClearWords, Assign::AssignWithReferenceCheck},
}};

constexpr std::array<ExpectedText, 2> kActionRange{{
    {"EPAR_NEAR", Init::ClearWords, Assign::CopyAndTerminate},
    {"EPAR_FAR", Init::ClearWords, Assign::AssignWithReferenceCheck},
}};

constexpr std::array<ExpectedText, 7> kMineSkillText{{
    {"EMS_TRAP", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EMS_EXPLORE_CASE", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EMS_PROBE_TREASURE", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EMS_UNLOCK_CASE", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EMS_SCATTER_POISON", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EMS_EXPLORE_MINE", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EMS_DIG_MINE", Init::ResetFlag, Assign::SetExceptionMessage},
}};

void SoundInitializationAndRegistration() {
    Runtime direct;
    direct.assign_results = {1, 2, 3, 4, 0x1234};
    Check(pal4::GameDatabaseStaticText::InitializeSound(direct) == 0x34u,
          "sound initializer forwards the low byte of final assignment");
    CheckGroup(direct, 0x8efa58u, kSound);
    Check(direct.calls.size() == 10u,
          "sound initializer processes exactly five records");

    Runtime registered;
    registered.assign_results = {1, 2, 3, 4, 5};
    Check(pal4::GameDatabaseStaticText::RegisterSound(registered) == -23,
          "sound CRT initializer forwards atexit result");
    CheckGroup(registered, 0x8efa58u, kSound);
    Check(registered.calls.back() == Call{"atexit", 0x50cf70u} &&
              registered.calls.size() == 11u,
          "sound cleanup is registered after all literals");
    registered.calls.clear();
    pal4::GameDatabaseStaticText::CleanupSound(registered);
    Check(registered.calls == std::vector<Call>{
              {"destroy", 0x8efa58u, "5"}},
          "sound cleanup destroys five 16-byte records");
}

void EffectInitializationAndRegistration() {
    Runtime direct;
    direct.assign_results = {1, 2, 3, 4, -1};
    Check(pal4::GameDatabaseStaticText::InitializeEffect(direct) == 0xffu,
          "effect initializer forwards final return as a byte");
    CheckGroup(direct, 0x8efa08u, kEffect);

    Runtime registered;
    registered.assign_results = {1, 2, 3, 4, 5};
    Check(pal4::GameDatabaseStaticText::RegisterEffect(registered) == -23,
          "effect CRT initializer forwards atexit result");
    Check(registered.calls.back() == Call{"atexit", 0x50d130u} &&
              registered.calls.size() == 11u,
          "effect cleanup registration follows all literal assignments");
    registered.calls.clear();
    pal4::GameDatabaseStaticText::CleanupEffect(registered);
    Check(registered.calls == std::vector<Call>{
              {"destroy", 0x8efa08u, "5"}},
          "effect cleanup destroys five 16-byte records");
}

void MapInitializationAndRegistration() {
    Runtime direct;
    direct.assign_results = {1, 2, 3, -19};
    Check(pal4::GameDatabaseStaticText::InitializeMap(direct) == -19,
          "map initializer forwards the full final assignment result");
    CheckGroup(direct, 0x8ef9c8u, kMap);
    Check(direct.calls.size() == 8u,
          "map initializer processes exactly four records");

    Runtime registered;
    registered.assign_results = {1, 2, 3, 4};
    Check(pal4::GameDatabaseStaticText::RegisterMap(registered) == -23,
          "map CRT initializer forwards atexit result");
    Check(registered.calls.back() == Call{"atexit", 0x50d2c0u} &&
              registered.calls.size() == 9u,
          "map cleanup registration follows all literal assignments");
    registered.calls.clear();
    pal4::GameDatabaseStaticText::CleanupMap(registered);
    Check(registered.calls == std::vector<Call>{
              {"destroy", 0x8ef9c8u, "4"}},
          "map cleanup destroys four 16-byte records");
}

void EquipmentInitializationAndRegistration() {
    Runtime direct;
    direct.assign_results = {1, 2, 3, 4, 5, 6, -37};
    Check(pal4::GameDatabaseStaticText::InitializeEquipment(direct) == -37,
          "equipment initializer forwards the full final result");
    CheckGroup(direct, 0x8ef958u, kEquipment);
    Check(direct.calls.size() == 14u,
          "equipment initializer processes seven records");

    Runtime registered;
    registered.assign_results = {1, 2, 3, 4, 5, 6, 7};
    Check(pal4::GameDatabaseStaticText::RegisterEquipment(registered) == -23,
          "equipment CRT initializer forwards atexit result");
    Check(registered.calls.back() == Call{"atexit", 0x50d580u} &&
              registered.calls.size() == 15u,
          "equipment cleanup registration follows seven assignments");
    registered.calls.clear();
    pal4::GameDatabaseStaticText::CleanupEquipment(registered);
    Check(registered.calls == std::vector<Call>{
              {"destroy", 0x8ef958u, "7"}},
          "equipment cleanup destroys seven records");
}

void EquipmentCategoryInitializationAndRegistration() {
    Runtime direct;
    direct.assign_results = {1, 2, 3, 4, 0x1234};
    Check(pal4::GameDatabaseStaticText::InitializeEquipmentCategory(direct) ==
              0x34u,
          "equipment category initializer forwards the final low byte");
    CheckGroup(direct, 0x8ef908u, kEquipmentCategory);
    Check(direct.calls.size() == 10u,
          "equipment category initializer processes five records");

    Runtime registered;
    registered.assign_results = {1, 2, 3, 4, 5};
    Check(pal4::GameDatabaseStaticText::RegisterEquipmentCategory(registered)
              == -23,
          "equipment category CRT initializer forwards atexit result");
    Check(registered.calls.back() == Call{"atexit", 0x50d740u} &&
              registered.calls.size() == 11u,
          "equipment category cleanup registration follows all assignments");
    registered.calls.clear();
    pal4::GameDatabaseStaticText::CleanupEquipmentCategory(registered);
    Check(registered.calls == std::vector<Call>{
              {"destroy", 0x8ef908u, "5"}},
          "equipment category cleanup destroys five records");
}

void CheckAdditionalGroup(const std::uint32_t first,
                          const std::uint32_t cleanup_callback,
                          const auto& expected,
                          auto initialize, auto register_cleanup,
                          auto cleanup, const bool byte_return) {
    Runtime direct;
    direct.assign_results.assign(expected.size(), 1);
    direct.assign_results.back() = byte_return ? 0x1234 : -19;
    const auto result = initialize(direct);
    Check(result == (byte_return ? 0x34 : -19),
          "GameDB static text initializer forwards the correct return width");
    CheckGroup(direct, first, expected);
    Check(direct.calls.size() == 2u * expected.size(),
          "GameDB static text initializer processes every record");

    Runtime registered;
    registered.assign_results.assign(expected.size(), 1);
    Check(register_cleanup(registered) == -23,
          "GameDB static text CRT wrapper forwards atexit result");
    CheckGroup(registered, first, expected);
    Check(registered.calls.size() == 2u * expected.size() + 1u &&
              registered.calls.back() ==
                  Call{"atexit", cleanup_callback},
          "GameDB static text cleanup is registered after all assignments");
    registered.calls.clear();
    cleanup(registered);
    Check(registered.calls == std::vector<Call>{
              {"destroy", first, std::to_string(expected.size())}},
          "GameDB static text cleanup range matches its record count");
}

void AdditionalTextGroups() {
    using pal4::GameDatabaseStaticText;
    CheckAdditionalGroup(0x8ef8d8u, 0x50d8d0u, kEquipmentProcess,
                         GameDatabaseStaticText::InitializeEquipmentProcess,
                         GameDatabaseStaticText::RegisterEquipmentProcess,
                         GameDatabaseStaticText::CleanupEquipmentProcess,
                         true);
    CheckAdditionalGroup(0x8ef888u, 0x50da90u, kItemCategory,
                         GameDatabaseStaticText::InitializeItemCategory,
                         GameDatabaseStaticText::RegisterItemCategory,
                         GameDatabaseStaticText::CleanupItemCategory,
                         true);
    CheckAdditionalGroup(0x8ef848u, 0x50dc20u, kShopType,
                         GameDatabaseStaticText::InitializeShopType,
                         GameDatabaseStaticText::RegisterShopType,
                         GameDatabaseStaticText::CleanupShopType,
                         false);
    CheckAdditionalGroup(0x8ef818u, 0x50ddb0u, kItemType,
                         GameDatabaseStaticText::InitializeItemType,
                         GameDatabaseStaticText::RegisterItemType,
                         GameDatabaseStaticText::CleanupItemType,
                         true);
    CheckAdditionalGroup(0x8ef7a8u, 0x50e070u, kRace,
                         GameDatabaseStaticText::InitializeRace,
                         GameDatabaseStaticText::RegisterRace,
                         GameDatabaseStaticText::CleanupRace,
                         false);
    CheckAdditionalGroup(0x8ef728u, 0x50e360u, kEat,
                         GameDatabaseStaticText::InitializeEat,
                         GameDatabaseStaticText::RegisterEat,
                         GameDatabaseStaticText::CleanupEat,
                         false);
    CheckAdditionalGroup(0x8ef6e8u, 0x50e4f0u, kEnemyAction,
                         GameDatabaseStaticText::InitializeEnemyAction,
                         GameDatabaseStaticText::RegisterEnemyAction,
                         GameDatabaseStaticText::CleanupEnemyAction,
                         false);
    CheckAdditionalGroup(0x8ef688u, 0x50e740u, kCombatPosition,
                         GameDatabaseStaticText::InitializeCombatPosition,
                         GameDatabaseStaticText::RegisterCombatPosition,
                         GameDatabaseStaticText::CleanupCombatPosition,
                         false);
    CheckAdditionalGroup(0x8ef648u, 0x50e8d0u, kItemUsage,
                         GameDatabaseStaticText::InitializeItemUsage,
                         GameDatabaseStaticText::RegisterItemUsage,
                         GameDatabaseStaticText::CleanupItemUsage,
                         false);
    CheckAdditionalGroup(0x8ef5d8u, 0x50eb90u, kAction,
                         GameDatabaseStaticText::InitializeAction,
                         GameDatabaseStaticText::RegisterAction,
                         GameDatabaseStaticText::CleanupAction,
                         false);
    CheckAdditionalGroup(0x8ef5a8u, 0x50ed20u, kActionState,
                         GameDatabaseStaticText::InitializeActionState,
                         GameDatabaseStaticText::RegisterActionState,
                         GameDatabaseStaticText::CleanupActionState,
                         true);
    CheckAdditionalGroup(0x8ef4b8u, 0x50f0b0u, kSkillEffectText,
                         GameDatabaseStaticText::InitializeSkillEffectText,
                         GameDatabaseStaticText::RegisterSkillEffectText,
                         GameDatabaseStaticText::CleanupSkillEffectText,
                         false);
    CheckAdditionalGroup(0x8ef458u, 0x50f300u, kEatText,
                         GameDatabaseStaticText::InitializeEatText,
                         GameDatabaseStaticText::RegisterEatText,
                         GameDatabaseStaticText::CleanupEatText,
                         false);
    CheckAdditionalGroup(0x8ef438u, 0x50f490u, kActionPattern,
                         GameDatabaseStaticText::InitializeActionPattern,
                         GameDatabaseStaticText::RegisterActionPattern,
                         GameDatabaseStaticText::CleanupActionPattern,
                         true);
    CheckAdditionalGroup(0x8ef418u, 0x50f620u, kActionRange,
                         GameDatabaseStaticText::InitializeActionRange,
                         GameDatabaseStaticText::RegisterActionRange,
                         GameDatabaseStaticText::CleanupActionRange,
                         true);
    CheckAdditionalGroup(0x8ef3a8u, 0x50f8e0u, kMineSkillText,
                         GameDatabaseStaticText::InitializeMineSkillText,
                         GameDatabaseStaticText::RegisterMineSkillText,
                         GameDatabaseStaticText::CleanupMineSkillText,
                         false);
}

}  // namespace

int main() {
    try {
        SoundInitializationAndRegistration();
        EffectInitializationAndRegistration();
        MapInitializationAndRegistration();
        EquipmentInitializationAndRegistration();
        EquipmentCategoryInitializationAndRegistration();
        AdditionalTextGroups();
        Check(pal4::GameDatabaseStaticText::NameAt(0x8ef728u)=="EATS_SELF_IN_DANGER" &&
              pal4::GameDatabaseStaticText::NameAt(0x8ef798u)=="EATS_BOUT_TIMES" &&
              pal4::GameDatabaseStaticText::NameAt(0x8ef6e8u)=="EAES_ENEMY_SLAUGHTERED" &&
              pal4::GameDatabaseStaticText::NameAt(0x8ef718u)=="EAES_NORMAL" &&
              !pal4::GameDatabaseStaticText::NameAt(0x8ef729u),"AI enum log names retain original aligned table boundaries");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
