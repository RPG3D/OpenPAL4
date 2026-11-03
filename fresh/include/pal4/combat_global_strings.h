#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatGlobalStringRuntime {
public:
    virtual ~ICombatGlobalStringRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint8_t flag) = 0;
    virtual void Reallocate(std::uint32_t record,
                            std::uint32_t length) = 0;
    virtual void CopyLiteral(std::uint32_t destination,
                             std::string_view literal) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual int RegisterCleanup(
        std::uint32_t callback) = 0;
};

class CombatGlobalStrings final {
public:
    // IDA576CF0 initializes the global read by ActorInstance52AA20.
    static constexpr std::string_view kDefaultAnimation = "z01";
    static constexpr std::string_view kPhysicalAttackAnimation = "z03";
    static constexpr std::string_view kMagicCastAnimation = "z07";
    static constexpr std::string_view kCollapseAnimation = "z12";
    static constexpr std::string_view kFendOffAnimation = "z05";
    static constexpr std::string_view kWoundedAnimation = "z04";
    static constexpr std::uint32_t kCleanupGuard = 0x8fa39cu;

    // IDA 0x576ce0/0x576cf0/0x576de0.
    [[nodiscard]] static int RegisterZ01(std::uint8_t seed,
                                         ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ01(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ01(ICombatGlobalStringRuntime& runtime);

    // IDA 0x576e40/0x576e50/0x576f40.
    [[nodiscard]] static int RegisterZ03(std::uint8_t seed,
                                         ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ03(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ03(ICombatGlobalStringRuntime& runtime);

    // IDA 0x576fa0/0x576fb0/0x5770a0.
    [[nodiscard]] static int RegisterZ02(std::uint8_t seed,
                                         ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ02(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ02(ICombatGlobalStringRuntime& runtime);

    // IDA 0x577100/0x577110/0x577200.
    [[nodiscard]] static int RegisterZ07(std::uint8_t seed,
                                         ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ07(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ07(ICombatGlobalStringRuntime& runtime);

    // IDA 0x577260/0x577270/0x577360.
    [[nodiscard]] static int RegisterZ12(std::uint8_t seed,
                                         ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ12(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ12(ICombatGlobalStringRuntime& runtime);

    // IDA 0x5773c0/0x5773d0/0x5774c0.
    [[nodiscard]] static int RegisterZ10(std::uint8_t seed,
                                         ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ10(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ10(ICombatGlobalStringRuntime& runtime);

    // IDA 0x577520/0x577530/0x577620; the wrapper is misnamed Z04 in IDA.
    [[nodiscard]] static int RegisterZ11(std::uint8_t seed,
                                         ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ11(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ11(ICombatGlobalStringRuntime& runtime);

    // IDA 0x577680/0x577690/0x577780.
    [[nodiscard]] static int RegisterZ05(std::uint8_t seed,
                                         ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ05(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ05(ICombatGlobalStringRuntime& runtime);

    // IDA 0x5777e0/0x5777f0/0x5778e0.
    [[nodiscard]] static int RegisterZ04(std::uint8_t seed,
                                         ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ04(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ04(ICombatGlobalStringRuntime& runtime);

    // IDA 0x577940/0x577950/0x577a40.
    [[nodiscard]] static int RegisterZ06(std::uint8_t seed,
                                         ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ06(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ06(ICombatGlobalStringRuntime& runtime);

    // IDA 0x577aa0/0x577ab0/0x577ba0; wrapper initializes Z08.
    [[nodiscard]] static int RegisterZ08(std::uint8_t seed,
                                         ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ08(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ08(ICombatGlobalStringRuntime& runtime);

    // IDA 0x577c00/0x577c10/0x577d00; wrapper initializes Z09.
    [[nodiscard]] static int RegisterZ09(std::uint8_t seed,
                                         ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ09(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ09(ICombatGlobalStringRuntime& runtime);

    // IDA 0x577d60/0x577d70/0x577e60.
    [[nodiscard]] static int RegisterZ14(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ14(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ14(ICombatGlobalStringRuntime& runtime);
    // IDA 0x577ec0/0x577ed0/0x577fc0.
    [[nodiscard]] static int RegisterZ15(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ15(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ15(ICombatGlobalStringRuntime& runtime);
    // IDA 0x578020/0x578030/0x578120.
    [[nodiscard]] static int RegisterZ16(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ16(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ16(ICombatGlobalStringRuntime& runtime);
    // IDA 0x578180/0x578190/0x578280.
    [[nodiscard]] static int RegisterZ17(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ17(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ17(ICombatGlobalStringRuntime& runtime);
    // IDA 0x5782e0/0x5782f0/0x5783e0.
    [[nodiscard]] static int RegisterZ18(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeZ18(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupZ18(ICombatGlobalStringRuntime& runtime);
    // IDA 0x578440/0x578450/0x578540.
    [[nodiscard]] static int RegisterHookArea(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeHookArea(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupHookArea(ICombatGlobalStringRuntime& runtime);
    // IDA 0x5785a0/0x5785b0/0x5786a0.
    [[nodiscard]] static int RegisterHookEfPos(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeHookEfPos(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupHookEfPos(ICombatGlobalStringRuntime& runtime);
    // IDA 0x578700/0x578710/0x578800.
    [[nodiscard]] static int RegisterCombatScene(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeCombatScene(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupCombatScene(ICombatGlobalStringRuntime& runtime);
    // IDA 0x578860/0x578870/0x578960.
    [[nodiscard]] static int RegisterCombatArea(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeCombatArea(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupCombatArea(ICombatGlobalStringRuntime& runtime);
    // IDA 0x5789c0/0x5789d0/0x578ac0.
    [[nodiscard]] static int RegisterStartType(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeStartType(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupStartType(ICombatGlobalStringRuntime& runtime);
    // IDA 0x578b20/0x578b30/0x578c20.
    [[nodiscard]] static int RegisterCombatResult(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeCombatResult(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupCombatResult(ICombatGlobalStringRuntime& runtime);
    // IDA 0x578c80/0x578c90/0x578d80.
    [[nodiscard]] static int RegisterCbMonster(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeCbMonster(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupCbMonster(ICombatGlobalStringRuntime& runtime);
    // IDA 0x578de0/0x578df0/0x578ee0.
    [[nodiscard]] static int RegisterMonsterId(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeMonsterId(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupMonsterId(ICombatGlobalStringRuntime& runtime);
    // IDA 0x578f40/0x578f50/0x579040.
    [[nodiscard]] static int RegisterCombatPosition(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeCombatPosition(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupCombatPosition(ICombatGlobalStringRuntime& runtime);
    // IDA 0x5790a0/0x5790b0/0x5791a0.
    [[nodiscard]] static int RegisterMonsterAbState(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeMonsterAbState(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupMonsterAbState(ICombatGlobalStringRuntime& runtime);
    // IDA 0x579200/0x579210/0x579300.
    [[nodiscard]] static int RegisterMonsterGroup(std::uint8_t seed,
                                           ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeMonsterGroup(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupMonsterGroup(ICombatGlobalStringRuntime& runtime);

    // IDA 0x57a1a0/0x57a1b0/0x57a2a0.
    [[nodiscard]] static int RegisterEventShowFight(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeEventShowFight(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupEventShowFight(ICombatGlobalStringRuntime& runtime);

    // IDA 0x57a300/0x57a310/0x57a400.
    [[nodiscard]] static int RegisterEventCloseSystem(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeEventCloseSystem(
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void CleanupEventCloseSystem(ICombatGlobalStringRuntime& runtime);


private:
    [[nodiscard]] static std::uint8_t Initialize(
        std::uint32_t record, std::string_view literal,
        std::uint8_t seed, ICombatGlobalStringRuntime& runtime);
    static void Cleanup(std::uint32_t record, std::uint32_t guard_address,
                        std::uint8_t guard_bit,
                        ICombatGlobalStringRuntime& runtime);
};

}  // namespace pal4
