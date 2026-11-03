#include "pal4/game_database_static_text.h"

#include <array>
#include <span>

namespace pal4 {
namespace {

using Init = GameDatabaseTextInit;
using Assign = GameDatabaseTextAssign;

struct TextDescriptor {
    std::string_view literal;
    Init initialization;
    Assign assignment;
};

constexpr std::array<TextDescriptor, 5> kSound{{
    {"SOUND_WOUNDED1", Init::ClearWords, Assign::SetExceptionMessage},
    {"SOUND_WOUNDED2", Init::ClearWords, Assign::SetExceptionMessage},
    {"SOUND_WOUNDED3", Init::ClearWords, Assign::SetExceptionMessage},
    {"SOUND_WOUNDED4", Init::ResetFlag, Assign::CopyAndTerminate},
    {"SOUND_WOUNDED5", Init::ResetFlag, Assign::CopyAndTerminate},
}};

constexpr std::array<TextDescriptor, 5> kEffect{{
    {"EFE_WATER", Init::ClearWords, Assign::SetExceptionMessage},
    {"EFE_FIRE", Init::ClearWords, Assign::SetExceptionMessage},
    {"EFE_THUNDER", Init::ClearWords, Assign::SetExceptionMessage},
    {"EFE_AIR", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EFE_EARTH", Init::ResetFlag, Assign::CopyAndTerminate},
}};

constexpr std::array<TextDescriptor, 4> kMap{{
    {"ePT_TIANHE", Init::ClearWords, Assign::CopyAndSetSize},
    {"ePT_LINSHA", Init::ClearWords, Assign::SetExceptionMessage},
    {"ePT_MENGLI", Init::ClearWords, Assign::CopyAndSetSize},
    {"ePT_ZIYING", Init::ClearWords, Assign::SetExceptionMessage},
}};

constexpr std::array<TextDescriptor, 7> kEquipment{{
    {"EET_SWORD", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EET_DSWORD", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EET_INSTRUMENT", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EET_HELMET", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EET_CORSELET", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EET_LEGHARNESS", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EET_ORNAMENT", Init::ResetFlag, Assign::SetExceptionMessage},
}};

constexpr std::array<TextDescriptor, 5> kEquipmentCategory{{
    {"EEC_WEAPON", Init::ClearWords, Assign::SetExceptionMessage},
    {"EEC_HELMENT", Init::ClearWords, Assign::SetExceptionMessage},
    {"EEC_CORSELET", Init::ClearWords, Assign::SetExceptionMessage},
    {"EEC_LEGHARNESS", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EEC_ORNAMENT", Init::ResetFlag, Assign::CopyAndTerminate},
}};

constexpr std::array<TextDescriptor, 3> kEquipmentProcess{{
    {"EPT_MELT", Init::ClearWords, Assign::CopyAndTerminate},
    {"EPT_FORGING", Init::ClearWords, Assign::CopyAndTerminate},
    {"EPT_ADD_MAGIC", Init::ClearWords, Assign::CopyAndTerminate},
}};

constexpr std::array<TextDescriptor, 5> kItemCategory{{
    {"EPC_CURE", Init::ClearWords, Assign::SetExceptionMessage},
    {"EPC_ATTACKABLE", Init::ClearWords, Assign::SetExceptionMessage},
    {"EPC_AUXILIARY", Init::ClearWords, Assign::SetExceptionMessage},
    {"EPC_MATERIAL", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EPC_SCENARIO", Init::ResetFlag, Assign::CopyAndTerminate},
}};

constexpr std::array<TextDescriptor, 4> kShopType{{
    {"EST_FORGE", Init::ClearWords, Assign::CopyAndSetSize},
    {"EST_GROCERY", Init::ClearWords, Assign::SetExceptionMessage},
    {"EST_DRUGSTORE", Init::ClearWords, Assign::CopyAndSetSize},
    {"EST_EATERY", Init::ClearWords, Assign::SetExceptionMessage},
}};

constexpr std::array<TextDescriptor, 3> kItemType{{
    {"EGT_EQUIPMENT", Init::ClearWords, Assign::CopyAndTerminate},
    {"EGT_PROPERTY", Init::ClearWords, Assign::CopyAndTerminate},
    {"EGT_PREDESCRIPTION", Init::ClearWords, Assign::CopyAndTerminate},
}};

constexpr std::array<TextDescriptor, 7> kRace{{
    {"ERR_HUMAN", Init::ResetFlag, Assign::CopyAndTerminate},
    {"ERR_DEMON", Init::ResetFlag, Assign::CopyAndTerminate},
    {"ERR_FAIRY", Init::ResetFlag, Assign::CopyAndTerminate},
    {"ERR_GHOST", Init::ResetFlag, Assign::CopyAndSetSize},
    {"ERR_IMMORTAL", Init::ResetFlag, Assign::CopyAndSetSize},
    {"ERR_SPIRIT", Init::ResetFlag, Assign::CopyAndSetSize},
    {"ERR_BEAST", Init::ResetFlag, Assign::SetExceptionMessage},
}};

template <std::size_t N>
std::optional<std::string_view> NameInArray(
    const std::uint32_t address, const std::uint32_t base,
    const std::array<TextDescriptor, N>& names) {
    if (address < base || (address - base) % 16u != 0u)
        return std::nullopt;
    const auto index = (address - base) / 16u;
    if (index >= names.size()) return std::nullopt;
    return names[index].literal;
}

constexpr std::array<TextDescriptor, 8> kEat{{
    {"EATS_SELF_IN_DANGER", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EATS_SELF_WOUNDED", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EATS_TEAMMATE_SLAUGHTERED", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EATS_TEAMMATE_IN_DANGER", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EATS_TEAMMATE_WOUNDED", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EATS_NORMAL", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EATS_BOUT_COUNT", Init::ResetFlag, Assign::SetExceptionMessage},
    {"EATS_BOUT_TIMES", Init::ResetFlag, Assign::SetExceptionMessage},
}};

constexpr std::array<TextDescriptor, 4> kEnemyAction{{
    {"EAES_ENEMY_SLAUGHTERED", Init::ClearWords, Assign::CopyAndSetSize},
    {"EAES_ENEMY_IN_DANGER", Init::ClearWords, Assign::SetExceptionMessage},
    {"EAES_ENEMY_WOUNDED", Init::ClearWords, Assign::CopyAndSetSize},
    {"EAES_NORMAL", Init::ClearWords, Assign::SetExceptionMessage},
}};

constexpr std::array<TextDescriptor, 6> kCombatPosition{{
    {"ECP_FIRST", Init::ClearWords, Assign::SetExceptionMessage},
    {"ECP_SECOND", Init::ResetFlag, Assign::CopyAndTerminate},
    {"ECP_THIRD", Init::ResetFlag, Assign::CopyAndTerminate},
    {"ECP_FOURTH", Init::ResetFlag, Assign::CopyAndTerminate},
    {"ECP_FIFTH", Init::ResetFlag, Assign::CopyAndSetSize},
    {"ECP_SIXTH", Init::ResetFlag, Assign::SetExceptionMessage},
}};

constexpr std::array<TextDescriptor, 4> kItemUsage{{
    {"EST_USE", Init::ClearWords, Assign::CopyAndSetSize},
    {"EST_COMBAT", Init::ClearWords, Assign::SetExceptionMessage},
    {"EST_MAP", Init::ClearWords, Assign::CopyAndSetSize},
    {"EST_COMMON", Init::ClearWords, Assign::SetExceptionMessage},
}};

constexpr std::array<TextDescriptor, 7> kAction{{
    {"EACT_ATTACK", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EACT_AUXILIARY", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EACT_CURE", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EACT_PASSIVE", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EACT_SUMMON", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EACT_REVIVE", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EACT_UNUSABLE", Init::ResetFlag, Assign::SetExceptionMessage},
}};

constexpr std::array<TextDescriptor, 3> kActionState{{
    {"ESAT_STAY", Init::ClearWords, Assign::CopyAndTerminate},
    {"ESAT_MOVE", Init::ClearWords, Assign::CopyAndTerminate},
    {"EAST_TRANSLATE", Init::ClearWords, Assign::CopyAndTerminate},
}};

constexpr std::array<TextDescriptor, 15> kSkillEffectText{{
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

constexpr std::array<TextDescriptor, 6> kEatText{{
    {"EAT_SELF", Init::ClearWords, Assign::SetExceptionMessage},
    {"EAT_SINGLE_TEAMMATE", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EAT_SINGLE_ENEMY", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EAT_WHOLE_TEAMMATE", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EAT_WHOLE_ENEMY", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EAT_DESIGNATE", Init::ResetFlag, Assign::SetExceptionMessage},
}};

constexpr std::array<TextDescriptor, 2> kActionPattern{{
    {"EPAT_SINGLE", Init::ClearWords, Assign::CopyAndTerminate},
    {"EPAT_TEAM", Init::ClearWords, Assign::AssignWithReferenceCheck},
}};

constexpr std::array<TextDescriptor, 2> kActionRange{{
    {"EPAR_NEAR", Init::ClearWords, Assign::CopyAndTerminate},
    {"EPAR_FAR", Init::ClearWords, Assign::AssignWithReferenceCheck},
}};

constexpr std::array<TextDescriptor, 7> kMineSkillText{{
    {"EMS_TRAP", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EMS_EXPLORE_CASE", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EMS_PROBE_TREASURE", Init::ResetFlag, Assign::CopyAndTerminate},
    {"EMS_UNLOCK_CASE", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EMS_SCATTER_POISON", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EMS_EXPLORE_MINE", Init::ResetFlag, Assign::CopyAndSetSize},
    {"EMS_DIG_MINE", Init::ResetFlag, Assign::SetExceptionMessage},
}};

std::int32_t Initialize(const std::uint32_t first,
                        const std::span<const TextDescriptor> descriptors,
                        IGameDatabaseStaticTextRuntime& runtime) {
    std::int32_t result = 0;
    for (std::uint32_t i = 0u; i < descriptors.size(); ++i) {
        const auto address = first + i * GameDatabaseStaticText::kTextBytes;
        const auto& descriptor = descriptors[i];
        runtime.InitializeText(address, descriptor.initialization);
        result = runtime.Assign(address, descriptor.literal,
                                descriptor.assignment);
    }
    return result;
}

}  // namespace

std::optional<std::string_view> GameDatabaseStaticText::NameAt(
    const std::uint32_t address) {
    if (const auto name = NameInArray(address, kEffectAddress, kEffect))
        return name;
    if (const auto name = NameInArray(address, kEquipmentAddress, kEquipment))
        return name;
    if (const auto name = NameInArray(address, kRaceAddress, kRace))
        return name;
    if (const auto name = NameInArray(address, kMapAddress, kMap))
        return name;
    if (const auto name = NameInArray(address, kSoundAddress, kSound))
        return name;
    if (const auto name = NameInArray(address, kMineSkillTextAddress,
                                      kMineSkillText))
        return name;
    if (const auto name = NameInArray(address, kEatAddress, kEat))
        return name;
    if (const auto name = NameInArray(address, kEnemyActionAddress, kEnemyAction))
        return name;
    return std::nullopt;
}

int GameDatabaseStaticText::RegisterSound(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeSound(runtime));
    return runtime.RegisterAtExit(0x50cf70u);
}

std::uint8_t GameDatabaseStaticText::InitializeSound(
    IGameDatabaseStaticTextRuntime& runtime) {
    return static_cast<std::uint8_t>(Initialize(kSoundAddress, kSound, runtime));
}

void GameDatabaseStaticText::CleanupSound(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kSoundAddress, kTextBytes, 5u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterEffect(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeEffect(runtime));
    return runtime.RegisterAtExit(0x50d130u);
}

std::uint8_t GameDatabaseStaticText::InitializeEffect(
    IGameDatabaseStaticTextRuntime& runtime) {
    return static_cast<std::uint8_t>(Initialize(kEffectAddress, kEffect,
                                                runtime));
}

void GameDatabaseStaticText::CleanupEffect(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kEffectAddress, kTextBytes, 5u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterMap(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeMap(runtime));
    return runtime.RegisterAtExit(0x50d2c0u);
}

std::int32_t GameDatabaseStaticText::InitializeMap(
    IGameDatabaseStaticTextRuntime& runtime) {
    return Initialize(kMapAddress, kMap, runtime);
}

void GameDatabaseStaticText::CleanupMap(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kMapAddress, kTextBytes, 4u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterEquipment(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeEquipment(runtime));
    return runtime.RegisterAtExit(0x50d580u);
}

std::int32_t GameDatabaseStaticText::InitializeEquipment(
    IGameDatabaseStaticTextRuntime& runtime) {
    return Initialize(kEquipmentAddress, kEquipment, runtime);
}

void GameDatabaseStaticText::CleanupEquipment(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kEquipmentAddress, kTextBytes, 7u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterEquipmentCategory(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeEquipmentCategory(runtime));
    return runtime.RegisterAtExit(0x50d740u);
}

std::uint8_t GameDatabaseStaticText::InitializeEquipmentCategory(
    IGameDatabaseStaticTextRuntime& runtime) {
    return static_cast<std::uint8_t>(Initialize(
        kEquipmentCategoryAddress, kEquipmentCategory, runtime));
}

void GameDatabaseStaticText::CleanupEquipmentCategory(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kEquipmentCategoryAddress, kTextBytes, 5u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterEquipmentProcess(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeEquipmentProcess(runtime));
    return runtime.RegisterAtExit(0x50d8d0u);
}

std::uint8_t GameDatabaseStaticText::InitializeEquipmentProcess(
    IGameDatabaseStaticTextRuntime& runtime) {
    return static_cast<std::uint8_t>(Initialize(
        kEquipmentProcessAddress, kEquipmentProcess, runtime));
}

void GameDatabaseStaticText::CleanupEquipmentProcess(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kEquipmentProcessAddress, kTextBytes, 3u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterItemCategory(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeItemCategory(runtime));
    return runtime.RegisterAtExit(0x50da90u);
}

std::uint8_t GameDatabaseStaticText::InitializeItemCategory(
    IGameDatabaseStaticTextRuntime& runtime) {
    return static_cast<std::uint8_t>(Initialize(
        kItemCategoryAddress, kItemCategory, runtime));
}

void GameDatabaseStaticText::CleanupItemCategory(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kItemCategoryAddress, kTextBytes, 5u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterShopType(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeShopType(runtime));
    return runtime.RegisterAtExit(0x50dc20u);
}

std::int32_t GameDatabaseStaticText::InitializeShopType(
    IGameDatabaseStaticTextRuntime& runtime) {
    return Initialize(kShopTypeAddress, kShopType, runtime);
}

void GameDatabaseStaticText::CleanupShopType(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kShopTypeAddress, kTextBytes, 4u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterItemType(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeItemType(runtime));
    return runtime.RegisterAtExit(0x50ddb0u);
}

std::uint8_t GameDatabaseStaticText::InitializeItemType(
    IGameDatabaseStaticTextRuntime& runtime) {
    return static_cast<std::uint8_t>(Initialize(
        kItemTypeAddress, kItemType, runtime));
}

void GameDatabaseStaticText::CleanupItemType(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kItemTypeAddress, kTextBytes, 3u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterRace(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeRace(runtime));
    return runtime.RegisterAtExit(0x50e070u);
}

std::int32_t GameDatabaseStaticText::InitializeRace(
    IGameDatabaseStaticTextRuntime& runtime) {
    return Initialize(kRaceAddress, kRace, runtime);
}

void GameDatabaseStaticText::CleanupRace(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kRaceAddress, kTextBytes, 7u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterEat(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeEat(runtime));
    return runtime.RegisterAtExit(0x50e360u);
}

std::int32_t GameDatabaseStaticText::InitializeEat(
    IGameDatabaseStaticTextRuntime& runtime) {
    return Initialize(kEatAddress, kEat, runtime);
}

void GameDatabaseStaticText::CleanupEat(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kEatAddress, kTextBytes, 8u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterEnemyAction(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeEnemyAction(runtime));
    return runtime.RegisterAtExit(0x50e4f0u);
}

std::int32_t GameDatabaseStaticText::InitializeEnemyAction(
    IGameDatabaseStaticTextRuntime& runtime) {
    return Initialize(kEnemyActionAddress, kEnemyAction, runtime);
}

void GameDatabaseStaticText::CleanupEnemyAction(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kEnemyActionAddress, kTextBytes, 4u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterCombatPosition(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeCombatPosition(runtime));
    return runtime.RegisterAtExit(0x50e740u);
}

std::int32_t GameDatabaseStaticText::InitializeCombatPosition(
    IGameDatabaseStaticTextRuntime& runtime) {
    return Initialize(kCombatPositionAddress, kCombatPosition, runtime);
}

void GameDatabaseStaticText::CleanupCombatPosition(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kCombatPositionAddress, kTextBytes, 6u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterItemUsage(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeItemUsage(runtime));
    return runtime.RegisterAtExit(0x50e8d0u);
}

std::int32_t GameDatabaseStaticText::InitializeItemUsage(
    IGameDatabaseStaticTextRuntime& runtime) {
    return Initialize(kItemUsageAddress, kItemUsage, runtime);
}

void GameDatabaseStaticText::CleanupItemUsage(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kItemUsageAddress, kTextBytes, 4u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterAction(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeAction(runtime));
    return runtime.RegisterAtExit(0x50eb90u);
}

std::int32_t GameDatabaseStaticText::InitializeAction(
    IGameDatabaseStaticTextRuntime& runtime) {
    return Initialize(kActionAddress, kAction, runtime);
}

void GameDatabaseStaticText::CleanupAction(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kActionAddress, kTextBytes, 7u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterActionState(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeActionState(runtime));
    return runtime.RegisterAtExit(0x50ed20u);
}

std::uint8_t GameDatabaseStaticText::InitializeActionState(
    IGameDatabaseStaticTextRuntime& runtime) {
    return static_cast<std::uint8_t>(Initialize(kActionStateAddress,
                                                kActionState, runtime));
}

void GameDatabaseStaticText::CleanupActionState(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kActionStateAddress, kTextBytes, 3u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterSkillEffectText(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeSkillEffectText(runtime));
    return runtime.RegisterAtExit(0x50f0b0u);
}

std::int32_t GameDatabaseStaticText::InitializeSkillEffectText(
    IGameDatabaseStaticTextRuntime& runtime) {
    return Initialize(kSkillEffectTextAddress, kSkillEffectText, runtime);
}

void GameDatabaseStaticText::CleanupSkillEffectText(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kSkillEffectTextAddress, kTextBytes, 15u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterEatText(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeEatText(runtime));
    return runtime.RegisterAtExit(0x50f300u);
}

std::int32_t GameDatabaseStaticText::InitializeEatText(
    IGameDatabaseStaticTextRuntime& runtime) {
    return Initialize(kEatTextAddress, kEatText, runtime);
}

void GameDatabaseStaticText::CleanupEatText(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kEatTextAddress, kTextBytes, 6u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterActionPattern(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeActionPattern(runtime));
    return runtime.RegisterAtExit(0x50f490u);
}

std::uint8_t GameDatabaseStaticText::InitializeActionPattern(
    IGameDatabaseStaticTextRuntime& runtime) {
    return static_cast<std::uint8_t>(Initialize(kActionPatternAddress,
                                                kActionPattern, runtime));
}

void GameDatabaseStaticText::CleanupActionPattern(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kActionPatternAddress, kTextBytes, 2u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterActionRange(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeActionRange(runtime));
    return runtime.RegisterAtExit(0x50f620u);
}

std::uint8_t GameDatabaseStaticText::InitializeActionRange(
    IGameDatabaseStaticTextRuntime& runtime) {
    return static_cast<std::uint8_t>(Initialize(kActionRangeAddress,
                                                kActionRange, runtime));
}

void GameDatabaseStaticText::CleanupActionRange(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kActionRangeAddress, kTextBytes, 2u,
                         kElementDestructor);
}

int GameDatabaseStaticText::RegisterMineSkillText(
    IGameDatabaseStaticTextRuntime& runtime) {
    static_cast<void>(InitializeMineSkillText(runtime));
    return runtime.RegisterAtExit(0x50f8e0u);
}

std::int32_t GameDatabaseStaticText::InitializeMineSkillText(
    IGameDatabaseStaticTextRuntime& runtime) {
    return Initialize(kMineSkillTextAddress, kMineSkillText, runtime);
}

void GameDatabaseStaticText::CleanupMineSkillText(
    IGameDatabaseStaticTextRuntime& runtime) {
    runtime.DestroyRange(kMineSkillTextAddress, kTextBytes, 7u,
                         kElementDestructor);
}

}  // namespace pal4
