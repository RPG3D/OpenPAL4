#include "pal4/game_data_resource_names.h"

#include <array>

namespace pal4 {
namespace {

struct NameEntry {
    std::string_view text;
    GameDataStringMode mode;
};

using Mode = GameDataStringMode;

constexpr std::array<NameEntry, 6> kEquipment{{
    {"ERES_WEAPON", Mode::bare_direct},
    {"ERES_HELMENT", Mode::reset_allocated},
    {"ERES_CORSELET", Mode::reset_allocated},
    {"ERES_LEGHARNESS", Mode::reset_allocated},
    {"ERES_ORNAMENT1", Mode::reset_allocated},
    {"ERES_ORNAMENT2", Mode::reset_direct},
}};

constexpr std::array<NameEntry, 8> kCharacter{{
    {"CA_MAX_HP", Mode::reset_allocated},
    {"CA_ADDITIONAL_RAGE", Mode::reset_allocated},
    {"CA_MAX_MP", Mode::reset_allocated},
    {"CA_PHYSICAL", Mode::reset_allocated},
    {"CA_TOUGHNESS", Mode::reset_allocated},
    {"CA_SPEED", Mode::reset_allocated},
    {"CA_LUCKY", Mode::reset_direct},
    {"CA_WILL", Mode::reset_direct},
}};

constexpr std::array<NameEntry, 4> kCharacterExtra{{
    {"CA_ADDITIONAL_CRITICAL", Mode::bare_allocated},
    {"CA_FEND_OFF", Mode::bare_direct},
    {"CA_ADDITIONAL_HITTING", Mode::bare_allocated},
    {"CA_PAY_FOR_SPARE", Mode::bare_direct},
}};

constexpr std::array<NameEntry, 6> kAttack{{
    {"CA_ATTACK_PHYSICAL", Mode::bare_direct},
    {"CA_ATTACK_WATER", Mode::reset_allocated},
    {"CA_ATTACK_FIRE", Mode::reset_allocated},
    {"CA_ATTACK_THUNDER", Mode::reset_allocated},
    {"CA_ATTACK_AIR", Mode::reset_allocated},
    {"CA_ATTACK_EARTH", Mode::reset_direct},
}};

constexpr std::array<NameEntry, 10> kProgression{{
    {"CA_LOSABLE_PROPERTY", Mode::reset_allocated},
    {"CA_THE_LOSABLE_NUMBER", Mode::reset_allocated},
    {"CA_LOSABLE_MONEY", Mode::reset_allocated},
    {"CA_MULTI_HIT", Mode::reset_allocated},
    {"CA_LEVEL", Mode::reset_allocated},
    {"CA_EXPERIENCE", Mode::reset_direct},
    {"CA_MAGIC_POINT", Mode::reset_direct},
    {"CA_HP", Mode::reset_direct},
    {"CA_RAGE", Mode::reset_direct},
    {"CA_MP", Mode::reset_direct},
}};

constexpr std::array<NameEntry, 5> kCombatParameters{{
    {"CA_MAGIC_ATTACK_PARAM", Mode::bare_direct},
    {"CA_PHYSICAL_ATTACK_PARAM", Mode::bare_direct},
    {"CA_HP_REGAIN_PARAM", Mode::bare_direct},
    {"CA_MP_REGAIN_PARAM", Mode::reset_allocated},
    {"CA_COUNTERPUNCH_RATE", Mode::reset_allocated},
}};

constexpr std::array<NameEntry, 5> kScriptIds{{
    {"SCP_ID_1", Mode::bare_direct},
    {"SCP_ID_2", Mode::bare_direct},
    {"SCP_ID_3", Mode::bare_direct},
    {"SCP_ID_4", Mode::reset_allocated},
    {"SCP_ID_5", Mode::reset_allocated},
}};

constexpr std::array<NameEntry, 2> kSpecialItemEffects{{
    {"ESI_SIPHON", Mode::bare_allocated},
    {"ESI_MORTAL", Mode::bare_std_assign},
}};

constexpr std::array<NameEntry, 6> kElementalAttributes{{
    {"EASA_NONE", Mode::bare_direct},
    {"EASA_WATER", Mode::reset_allocated},
    {"EASA_FIRE", Mode::reset_allocated},
    {"EASA_THUNDER", Mode::reset_allocated},
    {"EASA_AIR", Mode::reset_allocated},
    {"EASA_EARTH", Mode::reset_direct},
}};

constexpr std::array<NameEntry, 7> kEasb{{
    {"EASB_NONE", Mode::reset_allocated},
    {"EASB_BONDAGE", Mode::reset_allocated},
    {"EASB_SEAL", Mode::reset_allocated},
    {"EASB_FORBIDEN", Mode::reset_allocated},
    {"EASB_SLEEP", Mode::reset_allocated},
    {"EASB_MAD", Mode::reset_allocated},
    {"EASB_DISORDER", Mode::reset_direct},
}};

constexpr std::array<NameEntry, 7> kEasc{{
    {"EASC_NONE", Mode::reset_allocated},
    {"EASC_REFLECTION", Mode::reset_allocated},
    {"EASC_WALL", Mode::reset_allocated},
    {"EASC_BOUND", Mode::reset_allocated},
    {"EASC_CONCEALED", Mode::reset_allocated},
    {"EASC_TRANSFORMING", Mode::reset_allocated},
    {"EASC_POISON_PROOF", Mode::reset_direct},
}};

constexpr std::array<NameEntry, 2> kEasd{{
    {"EASD_NONE", Mode::bare_allocated},
    {"EASD_COMBO", Mode::bare_std_assign},
}};

constexpr std::array<NameEntry, 4> kEase{{
    {"EASE_NONE", Mode::bare_allocated},
    {"EASE_WEAK", Mode::bare_direct},
    {"EASE_DANGEROUS", Mode::bare_allocated},
    {"EASE_DEAD", Mode::bare_direct},
}};

constexpr std::array<NameEntry, 2> kEasf{{
    {"EASF_NONE", Mode::bare_allocated},
    {"EASF_ETERNAL", Mode::bare_std_assign},
}};

constexpr std::array<NameEntry, 5> kEasg{{
    {"EASG_NONE", Mode::bare_direct},
    {"EASG_DODGE", Mode::bare_direct},
    {"EASG_FENDOF", Mode::bare_direct},
    {"EASG_HITTED_MORTALLY", Mode::reset_allocated},
    {"EASG_CRITICAL", Mode::reset_allocated},
}};

constexpr std::array<NameEntry, 3> kEash{{
    {"EASH_NONE", Mode::bare_allocated},
    {"EASH_DEFEND", Mode::bare_allocated},
    {"EASH_FLEE_FAIL", Mode::bare_allocated},
}};

constexpr std::array<NameEntry, 6> kEasi{{
    {"EASI_NONE", Mode::bare_direct},
    {"EASI_SKILL1", Mode::reset_allocated},
    {"EASI_SKILL2", Mode::reset_allocated},
    {"EASI_SKILL3", Mode::reset_allocated},
    {"EASI_SKILL4", Mode::reset_allocated},
    {"EASI_SKILL5", Mode::reset_direct},
}};

constexpr std::array<NameEntry, 9> kEast{{
    {"EAST_A", Mode::reset_allocated},
    {"EAST_B", Mode::reset_allocated},
    {"EAST_C", Mode::reset_allocated},
    {"EAST_D", Mode::reset_allocated},
    {"EAST_E", Mode::reset_allocated},
    {"EAST_F", Mode::reset_allocated},
    {"EAST_G", Mode::reset_direct},
    {"EAST_H", Mode::reset_direct},
    {"EAST_I", Mode::reset_direct},
}};

constexpr std::array<NameEntry, 13> kAca{{
    {"ACA_SELF_IN_DANGER", Mode::reset_allocated},
    {"ACA_MIN_SELF_HP", Mode::reset_direct},
    {"ACA_MAX_SELF_HP", Mode::reset_direct},
    {"ACA_MATE_IN_DANGER", Mode::reset_direct},
    {"ACA_MATE_DEAD", Mode::reset_allocated},
    {"ACA_MIN_MATE_HP", Mode::reset_direct},
    {"ACA_MAX_MATE_HP", Mode::reset_direct},
    {"ACA_ENEMY_DEAD", Mode::reset_direct},
    {"ACA_ENEMY_IN_DANGER", Mode::reset_direct},
    {"ACA_MIN_ENEMY_HP", Mode::reset_direct},
    {"ACA_MAX_ENEMY_HP", Mode::reset_direct},
    {"ACA_BOUT_COUNT", Mode::reset_direct},
    {"ACA_BOUT_TIMES", Mode::reset_allocated},
}};

constexpr std::array<NameEntry, 13> kApa{{
    {"APA_ASSIGN_SKILL", Mode::reset_allocated},
    {"APA_ASSIGN_TARGET", Mode::reset_direct},
    {"APA_CURE_RATE", Mode::reset_direct},
    {"APA_SPECIAL_ATK_RATE", Mode::reset_direct},
    {"APA_OTHER_RATE", Mode::reset_allocated},
    {"APA_NORMAL_ATK_RATE", Mode::reset_direct},
    {"APA_FLEE_RATE", Mode::reset_direct},
    {"APA_SKILL_RATE", Mode::reset_direct},
    {"APA_SUMMON_RATE", Mode::reset_direct},
    {"APA_REVIVE_RATE", Mode::reset_direct},
    {"APA_FOR_ASSIGN_TARGET", Mode::reset_direct},
    {"APA_FOR_THE_DANGEROUS", Mode::reset_direct},
    {"APA_FOR_RANDOM", Mode::reset_allocated},
}};

constexpr std::array<NameEntry, 22> kUpgradeParameters{{
    {"HpUpgradeParam1", Mode::reset_direct},
    {"HpUpgradeParam2", Mode::reset_direct},
    {"HpUpgradeParam3", Mode::reset_direct},
    {"HpUpgradeParam4", Mode::reset_direct},
    {"MpUpgradeParam1", Mode::reset_direct},
    {"MpUpgradeParam2", Mode::reset_direct},
    {"MpUpgradeParam3", Mode::reset_direct},
    {"PhysicalUpgradeParam1", Mode::reset_direct},
    {"PhysicalUpgradeParam2", Mode::reset_direct},
    {"PhysicalUpgradeParam3", Mode::reset_direct},
    {"ToughnessUpgradeParam1", Mode::reset_direct},
    {"ToughnessUpgradeParam2", Mode::reset_direct},
    {"ToughnessUpgradeParam3", Mode::reset_direct},
    {"SpeedUpgradeParam1", Mode::reset_direct},
    {"SpeedUpgradeParam2", Mode::reset_direct},
    {"SpeedUpgradeParam3", Mode::reset_direct},
    {"LuckyUpgradeParam1", Mode::mode_cstring},
    {"LuckyUpgradeParam2", Mode::mode_cstring},
    {"LuckyUpgradeParam3", Mode::mode_cstring},
    {"WillUpgradeParam1", Mode::mode_cstring},
    {"WillUpgradeParam2", Mode::mode_cstring},
    {"WillUpgradeParam3", Mode::mode_cstring},
}};

constexpr std::array<NameEntry, 6> kEquipmentAttributes{{
    {"EA_PRICE", Mode::bare_direct},
    {"EA_LING_CAPACITY", Mode::reset_allocated},
    {"EA_SKILL_ID", Mode::reset_allocated},
    {"EA_FORGE_POTENTIAL", Mode::reset_allocated},
    {"EA_INT_TEMP1", Mode::reset_allocated},
    {"EA_INT_TEMP2", Mode::reset_direct},
}};

constexpr std::array<NameEntry, 13> kEquipmentAttributeText{{
    {"EA_NAME", Mode::reset_allocated},
    {"EA_DESCRIPTION", Mode::reset_direct},
    {"EA_LING_INFO", Mode::reset_direct},
    {"EA_EF1", Mode::reset_direct},
    {"EA_MODEL", Mode::reset_allocated},
    {"EA_TEXTURE", Mode::reset_direct},
    {"EA_EF2", Mode::reset_direct},
    {"EA_LING_NAME", Mode::reset_direct},
    {"EA_LHAND", Mode::reset_direct},
    {"EA_RHAND", Mode::reset_direct},
    {"EA_EF3", Mode::reset_direct},
    {"EA_STR_TEMP1", Mode::reset_direct},
    {"EA_STR_TEMP2", Mode::reset_allocated},
}};

template <std::size_t N>
std::optional<std::string_view> NameInArray(
    const std::uint32_t address, const std::uint32_t base,
    const std::array<NameEntry, N>& names) {
    if (address < base || (address - base) % 16u != 0u)
        return std::nullopt;
    const auto index = (address - base) / 16u;
    if (index >= names.size()) return std::nullopt;
    return names[index].text;
}

template <std::size_t N>
int InitializeArray(std::uint32_t base, const std::array<NameEntry, N>& names,
                    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    int result = 0;
    for (std::size_t i = 0; i < N; ++i) {
        const std::uint32_t address = base + static_cast<std::uint32_t>(i * 16);
        runtime.InitializeBuffer(address, seed, names[i].mode);
        result = runtime.AssignText(address, names[i].text, names[i].mode);
    }
    return result;
}

void DestroyArray(std::uint32_t base, std::uint32_t count,
                  IGameDataResourceNamesRuntime& runtime) {
    for (std::uint32_t i = count; i != 0; --i)
        runtime.DestroyBuffer(base + (i - 1) * 16);
}

}  // namespace

std::optional<std::string_view> GameDataResourceNames::NameAt(
    const std::uint32_t address) {
    if (const auto name = NameInArray(address, 0x8ee578u, kEquipment))
        return name;
    if (const auto name = NameInArray(address, 0x8ee4f8u, kCharacter))
        return name;
    if (const auto name = NameInArray(address, 0x8ee4b8u, kCharacterExtra))
        return name;
    if (const auto name = NameInArray(address, 0x8ee458u, kAttack))
        return name;
    if (const auto name = NameInArray(address, 0x8ee3b8u, kProgression))
        return name;
    if (const auto name = NameInArray(address, 0x8ee368u, kCombatParameters))
        return name;
    if (const auto name = NameInArray(address, 0x8ee348u, kSpecialItemEffects))
        return name;
    if (const auto name = NameInArray(address, 0x8ee2e8u, kElementalAttributes))
        return name;
    if (const auto name = NameInArray(address, 0x8ee278u, kEasb))
        return name;
    if (const auto name = NameInArray(address, 0x8ee018u, kEast))
        return name;
    if (const auto name = NameInArray(address, 0x8eeb90u, kScriptIds))
        return name;
    if (const auto name = NameInArray(address, 0x8ee710u, kAca))
        return name;
    if (const auto name = NameInArray(address, 0x8ee640u, kApa))
        return name;
    return std::nullopt;
}

int GameDataResourceNames::RegisterEquipment(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeEquipment(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::equipment);
}

int GameDataResourceNames::InitializeEquipment(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return InitializeArray(0x8ee578, kEquipment, seed, runtime);
}

void GameDataResourceNames::DestroyEquipment(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee578, 6, runtime);
}

int GameDataResourceNames::RegisterCharacter(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeCharacter(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::character);
}

int GameDataResourceNames::InitializeCharacter(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return InitializeArray(0x8ee4f8, kCharacter, seed, runtime);
}

void GameDataResourceNames::DestroyCharacter(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee4f8, 8, runtime);
}

int GameDataResourceNames::RegisterCharacterExtra(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeCharacterExtra(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::character_extra);
}

int GameDataResourceNames::InitializeCharacterExtra(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return InitializeArray(0x8ee4b8, kCharacterExtra, seed, runtime);
}

void GameDataResourceNames::DestroyCharacterExtra(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee4b8, 4, runtime);
}

int GameDataResourceNames::RegisterAttack(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeAttack(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::attack);
}

int GameDataResourceNames::InitializeAttack(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return InitializeArray(0x8ee458, kAttack, seed, runtime);
}

void GameDataResourceNames::DestroyAttack(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee458, 6, runtime);
}

int GameDataResourceNames::RegisterProgression(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeProgression(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::progression);
}

int GameDataResourceNames::InitializeProgression(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return InitializeArray(0x8ee3b8, kProgression, seed, runtime);
}

void GameDataResourceNames::DestroyProgression(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee3b8, 10, runtime);
}

int GameDataResourceNames::RegisterCombatParameters(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeCombatParameters(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::combat_parameters);
}

std::uint8_t GameDataResourceNames::InitializeCombatParameters(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return static_cast<std::uint8_t>(
        InitializeArray(0x8ee368, kCombatParameters, seed, runtime));
}

void GameDataResourceNames::DestroyCombatParameters(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee368, 5, runtime);
}

int GameDataResourceNames::RegisterSpecialItemEffects(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeSpecialItemEffects(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::special_item_effects);
}

std::uint8_t GameDataResourceNames::InitializeSpecialItemEffects(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return static_cast<std::uint8_t>(
        InitializeArray(0x8ee348, kSpecialItemEffects, seed, runtime));
}

void GameDataResourceNames::DestroySpecialItemEffects(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee348, 2, runtime);
}

int GameDataResourceNames::RegisterElementalAttributes(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeElementalAttributes(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::elemental_attributes);
}

int GameDataResourceNames::InitializeElementalAttributes(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return InitializeArray(0x8ee2e8, kElementalAttributes, seed, runtime);
}

void GameDataResourceNames::DestroyElementalAttributes(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee2e8, 6, runtime);
}

int GameDataResourceNames::RegisterEasb(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeEasb(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::easb);
}

int GameDataResourceNames::InitializeEasb(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return InitializeArray(0x8ee278, kEasb, seed, runtime);
}

void GameDataResourceNames::DestroyEasb(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee278, 7, runtime);
}

int GameDataResourceNames::RegisterEasc(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeEasc(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::easc);
}

int GameDataResourceNames::InitializeEasc(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return InitializeArray(0x8ee208, kEasc, seed, runtime);
}

void GameDataResourceNames::DestroyEasc(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee208, 7, runtime);
}

int GameDataResourceNames::RegisterEasd(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeEasd(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::easd);
}

std::uint8_t GameDataResourceNames::InitializeEasd(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return static_cast<std::uint8_t>(
        InitializeArray(0x8ee1e8, kEasd, seed, runtime));
}

void GameDataResourceNames::DestroyEasd(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee1e8, 2, runtime);
}

int GameDataResourceNames::RegisterEase(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeEase(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::ease);
}

int GameDataResourceNames::InitializeEase(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return InitializeArray(0x8ee1a8, kEase, seed, runtime);
}

void GameDataResourceNames::DestroyEase(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee1a8, 4, runtime);
}

int GameDataResourceNames::RegisterEasf(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeEasf(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::easf);
}

std::uint8_t GameDataResourceNames::InitializeEasf(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return static_cast<std::uint8_t>(
        InitializeArray(0x8ee188, kEasf, seed, runtime));
}

void GameDataResourceNames::DestroyEasf(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee188, 2, runtime);
}

int GameDataResourceNames::RegisterEasg(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeEasg(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::easg);
}

std::uint8_t GameDataResourceNames::InitializeEasg(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return static_cast<std::uint8_t>(
        InitializeArray(0x8ee138, kEasg, seed, runtime));
}

void GameDataResourceNames::DestroyEasg(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee138, 5, runtime);
}

int GameDataResourceNames::RegisterEash(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeEash(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::eash);
}

std::uint8_t GameDataResourceNames::InitializeEash(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return static_cast<std::uint8_t>(
        InitializeArray(0x8ee108, kEash, seed, runtime));
}

void GameDataResourceNames::DestroyEash(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee108, 3, runtime);
}

int GameDataResourceNames::RegisterEasi(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeEasi(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::easi);
}

int GameDataResourceNames::InitializeEasi(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return InitializeArray(0x8ee0a8, kEasi, seed, runtime);
}

void GameDataResourceNames::DestroyEasi(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee0a8, 6, runtime);
}

int GameDataResourceNames::RegisterEast(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeEast(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::east);
}

int GameDataResourceNames::InitializeEast(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return InitializeArray(0x8ee018, kEast, seed, runtime);
}

void GameDataResourceNames::DestroyEast(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee018, 9, runtime);
}

int GameDataResourceNames::RegisterAca(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeAca(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::aca);
}

std::uint8_t GameDataResourceNames::InitializeAca(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return static_cast<std::uint8_t>(
        InitializeArray(0x8ee710, kAca, seed, runtime));
}

void GameDataResourceNames::DestroyAca(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee710, 13, runtime);
}

int GameDataResourceNames::RegisterApa(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeApa(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::apa);
}

std::uint8_t GameDataResourceNames::InitializeApa(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return static_cast<std::uint8_t>(
        InitializeArray(0x8ee640, kApa, seed, runtime));
}

void GameDataResourceNames::DestroyApa(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee640, 13, runtime);
}

int GameDataResourceNames::RegisterUpgradeParameters(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeUpgradeParameters(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::upgrade_parameters);
}

std::uint32_t GameDataResourceNames::InitializeUpgradeParameters(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeArray(0x8ee850, kUpgradeParameters, seed, runtime);
    return 0x8ee9a0;
}

void GameDataResourceNames::DestroyUpgradeParameters(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee850, 22, runtime);
}

int GameDataResourceNames::RegisterEquipmentAttributes(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeEquipmentAttributes(seed, runtime);
    return runtime.RegisterCleanup(
        GameDataResourceNameGroup::equipment_attributes);
}

int GameDataResourceNames::InitializeEquipmentAttributes(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return InitializeArray(0x8eeac0, kEquipmentAttributes, seed, runtime);
}

void GameDataResourceNames::DestroyEquipmentAttributes(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8eeac0, 6, runtime);
}

int GameDataResourceNames::RegisterEquipmentAttributeText(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeEquipmentAttributeText(seed, runtime);
    return runtime.RegisterCleanup(
        GameDataResourceNameGroup::equipment_attribute_text);
}

std::uint8_t GameDataResourceNames::InitializeEquipmentAttributeText(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return static_cast<std::uint8_t>(
        InitializeArray(0x8ee9f0, kEquipmentAttributeText, seed, runtime));
}

void GameDataResourceNames::DestroyEquipmentAttributeText(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8ee9f0, 13, runtime);
}

int GameDataResourceNames::RegisterScriptIds(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    (void)InitializeScriptIds(seed, runtime);
    return runtime.RegisterCleanup(GameDataResourceNameGroup::script_ids);
}

std::uint8_t GameDataResourceNames::InitializeScriptIds(
    std::uint8_t seed, IGameDataResourceNamesRuntime& runtime) {
    return static_cast<std::uint8_t>(
        InitializeArray(0x8eeb90, kScriptIds, seed, runtime));
}

void GameDataResourceNames::DestroyScriptIds(
    IGameDataResourceNamesRuntime& runtime) {
    DestroyArray(0x8eeb90, 5, runtime);
}

}  // namespace pal4
