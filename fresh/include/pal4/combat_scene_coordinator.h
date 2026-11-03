#pragma once

#include <cstdint>

namespace pal4 {

class ICombatSceneRuntime {
public:
    virtual ~ICombatSceneRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void SetUIFlag(std::uint32_t manager, std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeConsole() = 0;
    [[nodiscard]] virtual std::uint32_t CreateScenarioDialog(
        std::uint32_t manager) = 0;
    virtual void HideScenarioDialog(std::uint32_t dialog) = 0;
    [[nodiscard]] virtual std::uint32_t CreateHintWindow(
        std::uint32_t manager) = 0;
    virtual void CloseHintWindow(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t GameState() = 0;
    virtual void AssignXmlNode(std::uint32_t destination,
                               std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    virtual void SetupCombatCamera(std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalDataSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t FindGameString(
        std::uint32_t table, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCommonDialog(
        std::uint32_t manager) = 0;
    virtual void ShowCommonDialog(std::uint32_t dialog,
                                  std::uint32_t message, float x, float y,
                                  std::uint32_t mode,
                                  std::uint32_t& callback) = 0;
    virtual void DestroyCallback(std::uint32_t callback,
                                 std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t CameraManager() = 0;
    virtual void ActivateCamera(std::uint32_t manager,
                                std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint8_t BeginCombat(
        std::uint32_t system, std::uint32_t xml_node,
        std::uint32_t camera_mode) = 0;
    virtual void CloseCombatUI(std::uint32_t system) = 0;
};

// Typed Entry/Leave services reuse the game algorithm without fabricated
// legacy UI/DB/callback addresses. Native dialogs own copied host callbacks.
class ICombatSceneServices {
public:
    virtual ~ICombatSceneServices() = default;
    virtual void EnableUi() = 0;
    virtual void HideConsole() = 0;
    virtual void HideScenarioDialog() = 0;
    virtual void CloseHint() = 0;
    virtual std::uint32_t GameFlag(std::uint32_t offset) = 0;
    virtual void SetGameFlag(std::uint32_t offset,std::uint32_t value) = 0;
    virtual std::uint32_t SavedFlag(std::uint32_t index) = 0;
    virtual void SetSavedFlag(std::uint32_t index,std::uint32_t value) = 0;
    virtual void AssignRequest(std::uint32_t document) = 0;
    virtual void SetupCamera() = 0;
    virtual bool ForceNormal() = 0;
    virtual std::uint32_t ChoiceMode() = 0;
    virtual void SetWaitingState() = 0;
    virtual void ShowChoice() = 0;
    virtual void ActivateCamera(bool quick) = 0;
    virtual std::uint8_t BeginCombat(std::uint32_t mode) = 0;
    virtual void CloseCombatUi() = 0;
};

class CombatSceneCoordinator final {
public:
    static constexpr std::uint32_t kGameStateFlagOffset = 1412u;
    static constexpr std::uint32_t kGameStateModeOffset = 1364u;
    static constexpr std::uint32_t kCallbackVtable = 0x8485e0u;
    static constexpr std::uint32_t kEmptyMessage = 0x841964u;

    static std::uint32_t EnterWithServices(std::uint32_t document,ICombatSceneServices&);
    static std::uint8_t StartWithServices(std::uint32_t selection,ICombatSceneServices&);
    static std::uint32_t LeaveWithServices(ICombatSceneServices&);

    // IDA 0x58adb0. The first native argument is unused.
    [[nodiscard]] static std::uint32_t Enter(
        std::uint32_t object, std::uint32_t unused,
        std::uint32_t xml_node, ICombatSceneRuntime& runtime);

    // IDA 0x58b270; also called directly by Enter.
    [[nodiscard]] static std::uint8_t StartCombat(
        std::uint32_t object, std::uint32_t mode,
        ICombatSceneRuntime& runtime);

    // IDA 0x58b250 / 0x58b2d0.
    [[nodiscard]] static std::uint32_t Leave(
        std::uint32_t object, std::uint32_t unused,
        ICombatSceneRuntime& runtime);
    [[nodiscard]] static std::uint32_t FinishLeave(
        std::uint32_t object, ICombatSceneRuntime& runtime);
};

}  // namespace pal4
