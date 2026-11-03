#pragma once

#include "pal4/minimap_scene_ui.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct MinimapSceneUiEventData {
    std::uint32_t kind{};
    std::uint32_t value{};
};

class IMinimapSceneUiMapVisibilityRuntime {
public:
    virtual ~IMinimapSceneUiMapVisibilityRuntime() = default;
    virtual void SetWindowVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual bool DialogIsVisible(std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t MinimapManager() = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class IMinimapSceneUiToggleRuntime : public IMinimapSceneUiMapVisibilityRuntime {
public:
    virtual void PlayMusic(std::string_view file, std::uint32_t mode) = 0;
};

class IMinimapSceneUiRunModeRuntime {
public:
    virtual ~IMinimapSceneUiRunModeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    [[nodiscard]] virtual std::int32_t ActorControlMode(std::uint32_t control) = 0;
    virtual void SetActorControlMode(std::uint32_t control,
                                     std::uint32_t context, std::int32_t mode) = 0;
    virtual void UpdateCharacterMovementState(std::uint32_t owner) = 0;
};

class IMinimapSceneUiActionsRuntime : public IMinimapSceneUiToggleRuntime,
                                    public IMinimapSceneUiRunModeRuntime {
public:
    virtual ~IMinimapSceneUiActionsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Game() = 0;
    virtual void InitializeStateAndUi(std::uint32_t game,
                                      std::uint32_t state,
                                      std::uint32_t payload) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ProcessEvent(std::uint32_t dispatcher,
                              std::uint32_t key_cell,
                              MinimapSceneUiEventData data) = 0;
    virtual void PlayMusic(std::string_view file,
                           std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    [[nodiscard]] virtual std::int32_t ActorControlMode(
        std::uint32_t control) = 0;
    virtual void SetActorControlMode(std::uint32_t control,
                                     std::uint32_t context,
                                     std::int32_t mode) = 0;
    virtual void UpdateCharacterMovementState(
        std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t InputManager() = 0;
    virtual void ResetInputState(std::uint32_t manager) = 0;
};

struct MinimapSceneUiAttributeLookup {
    std::uint32_t position{};
    std::uint32_t end{};
};

class IMinimapSceneUiHelpRuntime : public IMinimapSceneUiActionsRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeDocument() = 0;
    virtual void DestroyDocument(std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::uint32_t key_cell) = 0;
    virtual void BeginFrameName(std::string_view name) = 0;
    virtual void EndFrameName() = 0;
    [[nodiscard]] virtual MinimapSceneUiAttributeLookup FindAttribute(
        std::uint32_t document, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrameAttribute(
        std::string_view name, std::uint32_t frame) = 0;
    virtual void AddAttribute(std::uint32_t document,
                              std::uint32_t attribute) = 0;
    virtual void DestroyAttribute(std::uint32_t attribute) = 0;
    virtual void SetExistingAttribute(std::uint32_t attribute_value,
                                      std::uint32_t frame) = 0;
};

class MinimapSceneUiActions final {
public:
    static constexpr std::uint32_t kSystemEventKeyCell = 0x8a80b8u;
    static constexpr std::uint32_t kHelpEventKeyCell = 0x8b1c30u;
    static constexpr std::uint32_t kHelpFrameKeyCell = 0x8b13acu;

    [[nodiscard]] static std::uint8_t SwitchToSystem(
        IMinimapSceneUiActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t SystemClick(
        std::uint32_t ignored_event,
        IMinimapSceneUiActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t HelpClick(
        std::uint32_t ignored_event,
        IMinimapSceneUiHelpRuntime& runtime);
    [[nodiscard]] static std::uint8_t ToggleState(
        MinimapSceneUiState& state, std::uint32_t owner,
        std::uint32_t ignored_event,
        IMinimapSceneUiToggleRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetStateAndPlaySound(
        MinimapSceneUiState& state, std::uint32_t owner,
        std::uint8_t enabled,
        IMinimapSceneUiToggleRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetFlagAndUpdate(
        MinimapSceneUiState& state, std::uint32_t owner,
        std::uint8_t value,
        IMinimapSceneUiMapVisibilityRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateVisibilityAndMinimapState(
        const MinimapSceneUiState& state, std::uint32_t owner,
        IMinimapSceneUiMapVisibilityRuntime& runtime);
    static void AdvanceRunMode(std::uint32_t owner,
                               std::uint32_t context,
                               IMinimapSceneUiRunModeRuntime& runtime);
    [[nodiscard]] static std::uint8_t RunClick(
        std::uint32_t owner, std::uint32_t context,
        std::uint32_t ignored_event,
        IMinimapSceneUiActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetFlagTrue(
        MinimapSceneUiState& state, std::uint32_t ignored_event);
    [[nodiscard]] static std::uint8_t SetFlagFalseAndResetInput(
        MinimapSceneUiState& state, std::uint32_t ignored_event,
        IMinimapSceneUiActionsRuntime& runtime);
};

}  // namespace pal4
