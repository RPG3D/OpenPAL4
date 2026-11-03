#include "pal4/combat_global_strings.h"

#include <new>

namespace pal4 {
namespace {

constexpr std::uint32_t kZ01 = 0x8fa3a0u;
constexpr std::uint32_t kZ03 = 0x8fa38cu;
constexpr std::uint32_t kZ02 = 0x8fa37cu;
constexpr std::uint32_t kZ07 = 0x8fa36cu;
constexpr std::uint32_t kZ12 = 0x8fa35cu;
constexpr std::uint32_t kZ10 = 0x8fa34cu;
constexpr std::uint32_t kZ11 = 0x8fa33cu;
constexpr std::uint32_t kZ05 = 0x8fa32cu;
constexpr std::uint32_t kZ04 = 0x8fa31cu;
constexpr std::uint32_t kZ06 = 0x8fa308u;
constexpr std::uint32_t kZ08 = 0x8fa2f8u;
constexpr std::uint32_t kZ09 = 0x8fa2e8u;
constexpr std::uint32_t kSecondCleanupGuard = 0x8fa318u;

}  // namespace

std::uint8_t CombatGlobalStrings::Initialize(
    const std::uint32_t record, const std::string_view literal,
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    runtime.WriteByte(record, seed);
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
    runtime.WriteWord(record + 12u, 0u);
    runtime.SetErrorFlag(record, 1u);
    runtime.Reallocate(record,
                       static_cast<std::uint32_t>(literal.size()));
    const auto data = runtime.ReadWord(record + 4u);
    if (data == 0u) throw std::bad_alloc();
    runtime.CopyLiteral(data, literal);
    runtime.WriteWord(record + 8u,
                      static_cast<std::uint32_t>(literal.size()));
    runtime.WriteByte(data + static_cast<std::uint32_t>(literal.size()), 0u);
    return static_cast<std::uint8_t>(data);
}

void CombatGlobalStrings::Cleanup(
    const std::uint32_t record, const std::uint32_t guard_address,
    const std::uint8_t guard_bit,
    ICombatGlobalStringRuntime& runtime) {
    const auto guard = runtime.ReadByte(guard_address);
    if ((guard & guard_bit) != 0u) return;
    runtime.WriteByte(guard_address,
                      static_cast<std::uint8_t>(guard | guard_bit));
    const auto data = runtime.ReadWord(record + 4u);
    if (data != 0u) {
        const auto header = data - 1u;
        const auto references = runtime.ReadByte(header);
        if (references == 0u || references == 0xffu)
            static_cast<void>(runtime.Free(header));
        else
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(references - 1u));
    }
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
    runtime.WriteWord(record + 12u, 0u);
}

int CombatGlobalStrings::RegisterZ01(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    static_cast<void>(InitializeZ01(seed, runtime));
    return runtime.RegisterCleanup(0x576de0u);
}

std::uint8_t CombatGlobalStrings::InitializeZ01(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    return Initialize(kZ01, kDefaultAnimation, seed, runtime);
}

void CombatGlobalStrings::CleanupZ01(
    ICombatGlobalStringRuntime& runtime) {
    Cleanup(kZ01, kCleanupGuard, 1u, runtime);
}

int CombatGlobalStrings::RegisterZ03(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    static_cast<void>(InitializeZ03(seed, runtime));
    return runtime.RegisterCleanup(0x576f40u);
}

std::uint8_t CombatGlobalStrings::InitializeZ03(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    return Initialize(kZ03, kPhysicalAttackAnimation, seed, runtime);
}

void CombatGlobalStrings::CleanupZ03(
    ICombatGlobalStringRuntime& runtime) {
    Cleanup(kZ03, kCleanupGuard, 2u, runtime);
}

int CombatGlobalStrings::RegisterZ02(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    static_cast<void>(InitializeZ02(seed, runtime));
    return runtime.RegisterCleanup(0x5770a0u);
}

std::uint8_t CombatGlobalStrings::InitializeZ02(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    return Initialize(kZ02, "z02", seed, runtime);
}

void CombatGlobalStrings::CleanupZ02(
    ICombatGlobalStringRuntime& runtime) {
    Cleanup(kZ02, kCleanupGuard, 4u, runtime);
}

int CombatGlobalStrings::RegisterZ07(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    static_cast<void>(InitializeZ07(seed, runtime));
    return runtime.RegisterCleanup(0x577200u);
}

std::uint8_t CombatGlobalStrings::InitializeZ07(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    return Initialize(kZ07, kMagicCastAnimation, seed, runtime);
}

void CombatGlobalStrings::CleanupZ07(
    ICombatGlobalStringRuntime& runtime) {
    Cleanup(kZ07, kCleanupGuard, 8u, runtime);
}

int CombatGlobalStrings::RegisterZ12(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    static_cast<void>(InitializeZ12(seed, runtime));
    return runtime.RegisterCleanup(0x577360u);
}

std::uint8_t CombatGlobalStrings::InitializeZ12(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    return Initialize(kZ12, kCollapseAnimation, seed, runtime);
}

void CombatGlobalStrings::CleanupZ12(
    ICombatGlobalStringRuntime& runtime) {
    Cleanup(kZ12, kCleanupGuard, 16u, runtime);
}

int CombatGlobalStrings::RegisterZ10(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    static_cast<void>(InitializeZ10(seed, runtime));
    return runtime.RegisterCleanup(0x5774c0u);
}

std::uint8_t CombatGlobalStrings::InitializeZ10(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    return Initialize(kZ10, "z10", seed, runtime);
}

void CombatGlobalStrings::CleanupZ10(
    ICombatGlobalStringRuntime& runtime) {
    Cleanup(kZ10, kCleanupGuard, 32u, runtime);
}

int CombatGlobalStrings::RegisterZ11(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    static_cast<void>(InitializeZ11(seed, runtime));
    return runtime.RegisterCleanup(0x577620u);
}

std::uint8_t CombatGlobalStrings::InitializeZ11(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    return Initialize(kZ11, "z11", seed, runtime);
}

void CombatGlobalStrings::CleanupZ11(
    ICombatGlobalStringRuntime& runtime) {
    Cleanup(kZ11, kCleanupGuard, 64u, runtime);
}

int CombatGlobalStrings::RegisterZ05(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    static_cast<void>(InitializeZ05(seed, runtime));
    return runtime.RegisterCleanup(0x577780u);
}

std::uint8_t CombatGlobalStrings::InitializeZ05(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    return Initialize(kZ05, kFendOffAnimation, seed, runtime);
}

void CombatGlobalStrings::CleanupZ05(
    ICombatGlobalStringRuntime& runtime) {
    Cleanup(kZ05, kCleanupGuard, 128u, runtime);
}

int CombatGlobalStrings::RegisterZ04(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    static_cast<void>(InitializeZ04(seed, runtime));
    return runtime.RegisterCleanup(0x5778e0u);
}

std::uint8_t CombatGlobalStrings::InitializeZ04(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    return Initialize(kZ04, kWoundedAnimation, seed, runtime);
}

void CombatGlobalStrings::CleanupZ04(
    ICombatGlobalStringRuntime& runtime) {
    Cleanup(kZ04, kSecondCleanupGuard, 1u, runtime);
}

int CombatGlobalStrings::RegisterZ06(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    static_cast<void>(InitializeZ06(seed, runtime));
    return runtime.RegisterCleanup(0x577a40u);
}

std::uint8_t CombatGlobalStrings::InitializeZ06(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    return Initialize(kZ06, "z06", seed, runtime);
}

void CombatGlobalStrings::CleanupZ06(
    ICombatGlobalStringRuntime& runtime) {
    Cleanup(kZ06, kSecondCleanupGuard, 2u, runtime);
}

int CombatGlobalStrings::RegisterZ08(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    static_cast<void>(InitializeZ08(seed, runtime));
    return runtime.RegisterCleanup(0x577ba0u);
}

std::uint8_t CombatGlobalStrings::InitializeZ08(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    return Initialize(kZ08, "z08", seed, runtime);
}

void CombatGlobalStrings::CleanupZ08(
    ICombatGlobalStringRuntime& runtime) {
    Cleanup(kZ08, kSecondCleanupGuard, 4u, runtime);
}

int CombatGlobalStrings::RegisterZ09(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    static_cast<void>(InitializeZ09(seed, runtime));
    return runtime.RegisterCleanup(0x577d00u);
}

std::uint8_t CombatGlobalStrings::InitializeZ09(
    const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {
    return Initialize(kZ09, "z09", seed, runtime);
}

void CombatGlobalStrings::CleanupZ09(
    ICombatGlobalStringRuntime& runtime) {
    Cleanup(kZ09, kSecondCleanupGuard, 8u, runtime);
}

// Every entry has the same IDA initialization and guarded-release control flow.
#define PAL4_DEFINE_COMBAT_GLOBAL_STRING(Name, Record, Literal, Guard, Bit, Callback) \
    int CombatGlobalStrings::Register##Name(                                \
        const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {     \
        static_cast<void>(Initialize##Name(seed, runtime));                 \
        return runtime.RegisterCleanup(Callback);                           \
    }                                                                        \
    std::uint8_t CombatGlobalStrings::Initialize##Name(                     \
        const std::uint8_t seed, ICombatGlobalStringRuntime& runtime) {     \
        return Initialize(Record, Literal, seed, runtime);                   \
    }                                                                        \
    void CombatGlobalStrings::Cleanup##Name(                                 \
        ICombatGlobalStringRuntime& runtime) {                               \
        Cleanup(Record, Guard, Bit, runtime);                                 \
    }

PAL4_DEFINE_COMBAT_GLOBAL_STRING(Z14, 0x8fa2d8u, "z14",
                                 0x8fa318u, 16u, 0x577e60u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(Z15, 0x8fa2c8u, "z15",
                                 0x8fa318u, 32u, 0x577fc0u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(Z16, 0x8fa2b8u, "z16",
                                 0x8fa318u, 64u, 0x578120u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(Z17, 0x8fa2a8u, "z17",
                                 0x8fa318u, 128u, 0x578280u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(Z18, 0x8fa298u, "z18",
                                 0x8fa294u, 1u, 0x5783e0u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(HookArea, 0x8fa284u, "hook_area",
                                 0x8fa294u, 2u, 0x578540u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(HookEfPos, 0x8fa274u, "hook_ef_pos",
                                 0x8fa294u, 4u, 0x5786a0u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(CombatScene, 0x8fa264u, "CombatScene",
                                 0x8fa294u, 8u, 0x578800u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(CombatArea, 0x8fa254u, "CombatArea",
                                 0x8fa294u, 16u, 0x578960u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(StartType, 0x8fa244u, "StartType",
                                 0x8fa294u, 32u, 0x578ac0u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(CombatResult, 0x8fa234u, "CombatResult",
                                 0x8fa294u, 64u, 0x578c20u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(CbMonster, 0x8fa224u, "CbMonster",
                                 0x8fa294u, 128u, 0x578d80u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(MonsterId, 0x8fa214u, "MonsterId",
                                 0x8fa210u, 1u, 0x578ee0u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(CombatPosition, 0x8fa200u, "CombatPosition",
                                 0x8fa210u, 2u, 0x579040u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(MonsterAbState, 0x8fa1f0u, "MonsterAbState",
                                 0x8fa210u, 4u, 0x5791a0u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(MonsterGroup, 0x8fa1e0u, "MonsterGroup",
                                 0x8fa210u, 8u, 0x579300u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(EventShowFight, 0x8fa3c4u, "EventShowFight",
                                 0x8fa3c0u, 1u, 0x57a2a0u)
PAL4_DEFINE_COMBAT_GLOBAL_STRING(EventCloseSystem, 0x8fa3b0u, "EventCloseSystem",
                                 0x8fa3c0u, 2u, 0x57a400u)

#undef PAL4_DEFINE_COMBAT_GLOBAL_STRING

}  // namespace pal4
