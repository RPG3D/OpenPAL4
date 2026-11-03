#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct CombatEndingUiNameState {
    std::uint8_t cleanup_guard = 0u;
    std::uint8_t text_flag = 0u;
    std::string name;
};

class ICombatEndingUiNameRegistrationRuntime {
public:
    virtual ~ICombatEndingUiNameRegistrationRuntime() = default;
    [[nodiscard]] virtual int RegisterAtExit(
        std::uint32_t callback_address) = 0;
};

enum class CombatEndingUiNameKind : std::uint8_t {
    ShowStep,
    ShowWindowTask,
    CloseSystemTask,
    UpdateExpTask,
    MoveDownWindowTask,
    ShowLevelUpHintTask,
    ShowPrizePropTask,
    ShowScoreText,
    ShowScoreLvText,
};

struct CombatEndingUiNameDescriptor {
    std::string_view name;
    std::uint32_t guard_address;
    std::uint32_t text_address;
    std::uint32_t cleanup_callback;
};

class CombatEndingUiNameRegistry final {
public:
    [[nodiscard]] static const CombatEndingUiNameDescriptor& Descriptor(
        CombatEndingUiNameKind kind);
    [[nodiscard]] static int Register(
        CombatEndingUiNameKind kind, CombatEndingUiNameState& state,
        ICombatEndingUiNameRegistrationRuntime& runtime);
    static void InitializeName(CombatEndingUiNameKind kind,
                               CombatEndingUiNameState& state);
    static void Cleanup(CombatEndingUiNameKind kind,
                        CombatEndingUiNameState& state);
};

class CombatEndingShowStepRegistration final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8f9578u;
    static constexpr std::uint32_t kTextAddress = 0x8f9580u;
    static constexpr std::uint32_t kCleanupCallback = 0x544f80u;
    static constexpr std::string_view kName = "uiSetupShowStep";

    // IDA 0x544e80/0x544e90/0x544f80.
    [[nodiscard]] static int Register(
        CombatEndingUiNameState& state,
        ICombatEndingUiNameRegistrationRuntime& runtime);
    static void InitializeName(CombatEndingUiNameState& state);
    static void Cleanup(CombatEndingUiNameState& state);
};

class CombatEndingShowWindowTaskRegistration final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8f9564u;
    static constexpr std::uint32_t kTextAddress = 0x8f956cu;
    static constexpr std::uint32_t kCleanupCallback = 0x5450e0u;
    static constexpr std::string_view kName = "uiShowWindowTask";

    // IDA 0x544fe0/0x544ff0/0x5450e0.
    [[nodiscard]] static int Register(
        CombatEndingUiNameState& state,
        ICombatEndingUiNameRegistrationRuntime& runtime);
    static void InitializeName(CombatEndingUiNameState& state);
    static void Cleanup(CombatEndingUiNameState& state);
};

}  // namespace pal4
