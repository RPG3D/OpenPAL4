#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

enum class GameDataResourceNameGroup {
    equipment,
    character,
    character_extra,
    attack,
    progression,
    combat_parameters,
    special_item_effects,
    elemental_attributes,
    easb,
    easc,
    easd,
    ease,
    easf,
    easg,
    eash,
    easi,
    east,
    aca,
    apa,
    upgrade_parameters,
    equipment_attributes,
    equipment_attribute_text,
    script_ids,
};

enum class GameDataStringMode {
    bare_allocated,
    bare_direct,
    reset_allocated,
    reset_direct,
    bare_std_assign,
    mode_cstring,
};

class IGameDataResourceNamesRuntime {
public:
    virtual ~IGameDataResourceNamesRuntime() = default;
    virtual void InitializeBuffer(std::uint32_t address, std::uint8_t seed,
                                  GameDataStringMode mode) = 0;
    [[nodiscard]] virtual int AssignText(std::uint32_t address,
                                         std::string_view text,
                                         GameDataStringMode mode) = 0;
    [[nodiscard]] virtual int RegisterCleanup(
        GameDataResourceNameGroup group) = 0;
    virtual void DestroyBuffer(std::uint32_t address) = 0;
};

class GameDataResourceNames final {
public:
    [[nodiscard]] static std::optional<std::string_view> NameAt(
        std::uint32_t address);
    // IDA 0x4c8a50/0x4c8a60/0x4c8c80: six equipment names.
    [[nodiscard]] static int RegisterEquipment(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static int InitializeEquipment(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyEquipment(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4c8ca0/0x4c8cb0/0x4c8f70: eight character names.
    [[nodiscard]] static int RegisterCharacter(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static int InitializeCharacter(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyCharacter(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4c8f90/0x4c8fa0/0x4c9100: four additional character names.
    [[nodiscard]] static int RegisterCharacterExtra(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static int InitializeCharacterExtra(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyCharacterExtra(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4c9120/0x4c9130/0x4c9350: six attack names.
    [[nodiscard]] static int RegisterAttack(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static int InitializeAttack(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyAttack(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4c9370/0x4c9380/0x4c9680: ten progression names.
    [[nodiscard]] static int RegisterProgression(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static int InitializeProgression(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyProgression(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4c96a0/0x4c96b0/0x4c9840: five combat parameter names.
    [[nodiscard]] static int RegisterCombatParameters(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeCombatParameters(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyCombatParameters(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4c9860/0x4c9870/0x4c99d0: two special item effects.
    [[nodiscard]] static int RegisterSpecialItemEffects(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeSpecialItemEffects(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroySpecialItemEffects(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4c99f0/0x4c9a00/0x4c9c20: six elemental attributes.
    [[nodiscard]] static int RegisterElementalAttributes(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static int InitializeElementalAttributes(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyElementalAttributes(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4c9c40/0x4c9c50/0x4c9ee0: seven EASB names.
    [[nodiscard]] static int RegisterEasb(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static int InitializeEasb(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyEasb(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4c9f00/0x4c9f10/0x4ca1a0: seven EASC names.
    [[nodiscard]] static int RegisterEasc(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static int InitializeEasc(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyEasc(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4ca1c0/0x4ca1d0/0x4ca330: two EASD names.
    [[nodiscard]] static int RegisterEasd(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeEasd(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyEasd(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4ca350/0x4ca360/0x4ca4c0: four EASE names.
    [[nodiscard]] static int RegisterEase(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static int InitializeEase(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyEase(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4ca4e0/0x4ca4f0/0x4ca650: two EASF names.
    [[nodiscard]] static int RegisterEasf(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeEasf(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyEasf(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4ca670/0x4ca680/0x4ca810: five EASG names.
    [[nodiscard]] static int RegisterEasg(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeEasg(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyEasg(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4ca830/0x4ca840/0x4ca9a0: three EASH names.
    [[nodiscard]] static int RegisterEash(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeEash(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyEash(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4ca9c0/0x4ca9d0/0x4cabf0: six EASI names.
    [[nodiscard]] static int RegisterEasi(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static int InitializeEasi(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyEasi(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4cac10/0x4cac20/0x4caf10: nine EAST names.
    [[nodiscard]] static int RegisterEast(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static int InitializeEast(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyEast(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4caf60/0x4caf70/0x4cb2b0: 13 ACA names.
    [[nodiscard]] static int RegisterAca(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeAca(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyAca(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4cb2d0/0x4cb2e0/0x4cb620: 13 APA names.
    [[nodiscard]] static int RegisterApa(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeApa(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyApa(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4cf340/0x4cf350/0x4cf730: 22 upgrade-parameter names.
    [[nodiscard]] static int RegisterUpgradeParameters(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeUpgradeParameters(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyUpgradeParameters(IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4d1fc0/0x4d1fd0/0x4d21f0: six EA names and CRT cleanup.
    [[nodiscard]] static int RegisterEquipmentAttributes(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static int InitializeEquipmentAttributes(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyEquipmentAttributes(
        IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4d2210/0x4d2220/0x4d2560: 13 EA text names and CRT cleanup.
    [[nodiscard]] static int RegisterEquipmentAttributeText(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeEquipmentAttributeText(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyEquipmentAttributeText(
        IGameDataResourceNamesRuntime& runtime);

    // IDA 0x4d89f0/0x4d8a00/0x4d8b90: five SCP_ID names.
    [[nodiscard]] static int RegisterScriptIds(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeScriptIds(
        std::uint8_t seed, IGameDataResourceNamesRuntime& runtime);
    static void DestroyScriptIds(IGameDataResourceNamesRuntime& runtime);
};

}  // namespace pal4
