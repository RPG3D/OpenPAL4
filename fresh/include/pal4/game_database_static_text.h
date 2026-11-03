#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

enum class GameDatabaseTextInit : std::uint8_t {
    ClearWords,
    ResetFlag
};

enum class GameDatabaseTextAssign : std::uint8_t {
    SetExceptionMessage,
    CopyAndTerminate,
    CopyAndSetSize,
    AssignWithReferenceCheck
};

class IGameDatabaseStaticTextRuntime {
public:
    virtual ~IGameDatabaseStaticTextRuntime() = default;
    virtual void InitializeText(std::uint32_t address,
                                GameDatabaseTextInit mode) = 0;
    [[nodiscard]] virtual std::int32_t Assign(
        std::uint32_t address, std::string_view literal,
        GameDatabaseTextAssign mode) = 0;
    virtual void DestroyRange(std::uint32_t first, std::uint32_t stride,
                              std::uint32_t count,
                              std::uint32_t element_destructor) = 0;
    [[nodiscard]] virtual int RegisterAtExit(std::uint32_t callback) = 0;
};

class GameDatabaseStaticText final {
public:
    [[nodiscard]] static std::optional<std::string_view> NameAt(
        std::uint32_t address);
    static constexpr std::uint32_t kSoundAddress = 0x8efa58u;
    static constexpr std::uint32_t kEffectAddress = 0x8efa08u;
    static constexpr std::uint32_t kMapAddress = 0x8ef9c8u;
    static constexpr std::uint32_t kEquipmentAddress = 0x8ef958u;
    static constexpr std::uint32_t kEquipmentCategoryAddress = 0x8ef908u;
    static constexpr std::uint32_t kEquipmentProcessAddress = 0x8ef8d8u;
    static constexpr std::uint32_t kItemCategoryAddress = 0x8ef888u;
    static constexpr std::uint32_t kShopTypeAddress = 0x8ef848u;
    static constexpr std::uint32_t kItemTypeAddress = 0x8ef818u;
    static constexpr std::uint32_t kRaceAddress = 0x8ef7a8u;
    static constexpr std::uint32_t kEatAddress = 0x8ef728u;
    static constexpr std::uint32_t kEnemyActionAddress = 0x8ef6e8u;
    static constexpr std::uint32_t kCombatPositionAddress = 0x8ef688u;
    static constexpr std::uint32_t kItemUsageAddress = 0x8ef648u;
    static constexpr std::uint32_t kActionAddress = 0x8ef5d8u;
    static constexpr std::uint32_t kActionStateAddress = 0x8ef5a8u;
    static constexpr std::uint32_t kSkillEffectTextAddress = 0x8ef4b8u;
    static constexpr std::uint32_t kEatTextAddress = 0x8ef458u;
    static constexpr std::uint32_t kActionPatternAddress = 0x8ef438u;
    static constexpr std::uint32_t kActionRangeAddress = 0x8ef418u;
    static constexpr std::uint32_t kMineSkillTextAddress = 0x8ef3a8u;
    static constexpr std::uint32_t kTextBytes = 16u;
    static constexpr std::uint32_t kElementDestructor = 0x51f9b0u;

    // IDA 0x50cdd0, 0x50cde0, 0x50cf70.
    [[nodiscard]] static int RegisterSound(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeSound(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupSound(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50cf90, 0x50cfa0, 0x50d130.
    [[nodiscard]] static int RegisterEffect(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeEffect(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupEffect(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50d150, 0x50d160, 0x50d2c0.
    [[nodiscard]] static int RegisterMap(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::int32_t InitializeMap(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupMap(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50d2e0, 0x50d2f0, 0x50d580.
    [[nodiscard]] static int RegisterEquipment(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::int32_t InitializeEquipment(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupEquipment(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50d5a0, 0x50d5b0, 0x50d740.
    [[nodiscard]] static int RegisterEquipmentCategory(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeEquipmentCategory(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupEquipmentCategory(
        IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50d760-0x50d8d0.
    [[nodiscard]] static int RegisterEquipmentProcess(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeEquipmentProcess(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupEquipmentProcess(
        IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50d8f0-0x50da90.
    [[nodiscard]] static int RegisterItemCategory(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeItemCategory(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupItemCategory(
        IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50dab0-0x50dc20.
    [[nodiscard]] static int RegisterShopType(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::int32_t InitializeShopType(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupShopType(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50dc40-0x50ddb0.
    [[nodiscard]] static int RegisterItemType(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeItemType(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupItemType(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50ddd0-0x50e070.
    [[nodiscard]] static int RegisterRace(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::int32_t InitializeRace(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupRace(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50e090-0x50e360.
    [[nodiscard]] static int RegisterEat(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::int32_t InitializeEat(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupEat(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50e380-0x50e4f0.
    [[nodiscard]] static int RegisterEnemyAction(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::int32_t InitializeEnemyAction(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupEnemyAction(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50e510-0x50e740.
    [[nodiscard]] static int RegisterCombatPosition(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::int32_t InitializeCombatPosition(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupCombatPosition(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50e760-0x50e8d0.
    [[nodiscard]] static int RegisterItemUsage(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::int32_t InitializeItemUsage(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupItemUsage(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50e8f0-0x50eb90.
    [[nodiscard]] static int RegisterAction(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::int32_t InitializeAction(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupAction(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50ebb0-0x50ed20.
    [[nodiscard]] static int RegisterActionState(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeActionState(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupActionState(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50ed40-0x50f0b0.
    [[nodiscard]] static int RegisterSkillEffectText(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::int32_t InitializeSkillEffectText(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupSkillEffectText(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50f0d0-0x50f300.
    [[nodiscard]] static int RegisterEatText(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::int32_t InitializeEatText(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupEatText(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50f320-0x50f490.
    [[nodiscard]] static int RegisterActionPattern(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeActionPattern(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupActionPattern(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50f4b0-0x50f620.
    [[nodiscard]] static int RegisterActionRange(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeActionRange(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupActionRange(IGameDatabaseStaticTextRuntime& runtime);

    // IDA 0x50f640-0x50f8e0.
    [[nodiscard]] static int RegisterMineSkillText(
        IGameDatabaseStaticTextRuntime& runtime);
    [[nodiscard]] static std::int32_t InitializeMineSkillText(
        IGameDatabaseStaticTextRuntime& runtime);
    static void CleanupMineSkillText(IGameDatabaseStaticTextRuntime& runtime);
};

}  // namespace pal4
