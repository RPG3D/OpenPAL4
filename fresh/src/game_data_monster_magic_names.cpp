#include "pal4/game_data_monster_magic_names.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::uint32_t kNamesBase = 0x8eed50;
constexpr std::uint32_t kDestructorAddress = 0x4dd720;
constexpr std::uint32_t kTextBytes = 16;
constexpr std::array<std::string_view, 5> kNames{
    "MONSTER_MAGIC_1", "MONSTER_MAGIC_2", "MONSTER_MAGIC_3",
    "MONSTER_MAGIC_4", "MONSTER_MAGIC_5"};
constexpr std::uint32_t kFieldsBase = 0x8eecf0;
constexpr std::uint32_t kFieldsDestructorAddress = 0x4dd970;
constexpr std::array<std::string_view, 6> kFields{
    "MA_NAME", "MA_ICON", "MA_MODEL", "MA_TEXTURE", "MA_DESC",
    "EFF_FINAL_HIT"};
constexpr std::uint32_t kDropFieldsBase = 0x8eecc0;
constexpr std::uint32_t kDropFieldsDestructorAddress = 0x4ddb00;
constexpr std::array<std::string_view, 3> kDropFields{
    "MA_MAX_DROP_MONEY", "MA_MIN_DROP_MONEY", "MA_ATK_ADDED_RAGE"};
constexpr std::uint32_t kDataTypesBase = 0x8eec80;
constexpr std::uint32_t kDataTypesDestructorAddress = 0x4ddc90;
constexpr std::array<std::string_view, 4> kDataTypes{
    "MDT_1", "MDT_2", "MDT_3", "MDT_4"};

}  // namespace

int GameDataMonsterMagicNames::InitializeAndRegister(
    IGameDataMonsterMagicNamesRuntime& runtime) {
    Initialize(runtime);
    return runtime.RegisterAtexit(kDestructorAddress);
}

void GameDataMonsterMagicNames::Initialize(
    IGameDataMonsterMagicNamesRuntime& runtime) {
    const auto seed = runtime.InitialSeed();
    for (std::uint32_t i = 0; i < kNames.size(); ++i) {
        const auto address = kNamesBase + i * kTextBytes;
        runtime.WriteByte(address, seed);
        if (i < 3) {
            runtime.WriteWord(address + 4, 0);
            runtime.WriteWord(address + 8, 0);
            runtime.WriteWord(address + 12, 0);
            runtime.AssignText(address, kNames[i]);
        } else {
            (void)runtime.AllocateText(address, kNames[i]);
        }
    }
}

void GameDataMonsterMagicNames::Destroy(
    IGameDataMonsterMagicNamesRuntime& runtime) {
    for (std::uint32_t i = static_cast<std::uint32_t>(kNames.size());
         i != 0; --i)
        runtime.DestroyText(kNamesBase + (i - 1) * kTextBytes);
}

int GameDataMonsterMagicNames::InitializeFieldsAndRegister(
    IGameDataMonsterMagicNamesRuntime& runtime) {
    InitializeFields(runtime);
    return runtime.RegisterAtexit(kFieldsDestructorAddress);
}

void GameDataMonsterMagicNames::InitializeFields(
    IGameDataMonsterMagicNamesRuntime& runtime) {
    const auto seed = runtime.InitialSeed();
    for (std::uint32_t i = 0; i < kFields.size(); ++i) {
        const auto address = kFieldsBase + i * kTextBytes;
        runtime.WriteByte(address, seed);
        if (i == 0 || i == kFields.size() - 1) {
            runtime.WriteWord(address + 4, 0);
            runtime.WriteWord(address + 8, 0);
            runtime.WriteWord(address + 12, 0);
            runtime.AssignText(address, kFields[i]);
        } else {
            (void)runtime.AllocateText(address, kFields[i]);
        }
    }
}

void GameDataMonsterMagicNames::DestroyFields(
    IGameDataMonsterMagicNamesRuntime& runtime) {
    for (std::uint32_t i = static_cast<std::uint32_t>(kFields.size());
         i != 0; --i)
        runtime.DestroyText(kFieldsBase + (i - 1) * kTextBytes);
}

int GameDataMonsterMagicNames::InitializeDropFieldsAndRegister(
    IGameDataMonsterMagicNamesRuntime& runtime) {
    InitializeDropFields(runtime);
    return runtime.RegisterAtexit(kDropFieldsDestructorAddress);
}

void GameDataMonsterMagicNames::InitializeDropFields(
    IGameDataMonsterMagicNamesRuntime& runtime) {
    const auto seed = runtime.InitialSeed();
    for (std::uint32_t i = 0; i < kDropFields.size(); ++i) {
        const auto address = kDropFieldsBase + i * kTextBytes;
        runtime.WriteByte(address, seed);
        runtime.WriteWord(address + 4, 0);
        runtime.WriteWord(address + 8, 0);
        runtime.WriteWord(address + 12, 0);
        (void)runtime.AllocateText(address, kDropFields[i]);
    }
}

void GameDataMonsterMagicNames::DestroyDropFields(
    IGameDataMonsterMagicNamesRuntime& runtime) {
    for (std::uint32_t i = static_cast<std::uint32_t>(kDropFields.size());
         i != 0; --i)
        runtime.DestroyText(kDropFieldsBase + (i - 1) * kTextBytes);
}

int GameDataMonsterMagicNames::InitializeDataTypesAndRegister(
    IGameDataMonsterMagicNamesRuntime& runtime) {
    InitializeDataTypes(runtime);
    return runtime.RegisterAtexit(kDataTypesDestructorAddress);
}

void GameDataMonsterMagicNames::InitializeDataTypes(
    IGameDataMonsterMagicNamesRuntime& runtime) {
    const auto seed = runtime.InitialSeed();
    for (std::uint32_t i = 0; i < kDataTypes.size(); ++i) {
        const auto address = kDataTypesBase + i * kTextBytes;
        runtime.WriteByte(address, seed);
        runtime.WriteWord(address + 4, 0);
        runtime.WriteWord(address + 8, 0);
        runtime.WriteWord(address + 12, 0);
        if (i == 1 || i == 3)
            runtime.AssignText(address, kDataTypes[i]);
        else
            (void)runtime.AllocateText(address, kDataTypes[i]);
    }
}

void GameDataMonsterMagicNames::DestroyDataTypes(
    IGameDataMonsterMagicNamesRuntime& runtime) {
    for (std::uint32_t i = static_cast<std::uint32_t>(kDataTypes.size());
         i != 0; --i)
        runtime.DestroyText(kDataTypesBase + (i - 1) * kTextBytes);
}

}  // namespace pal4
