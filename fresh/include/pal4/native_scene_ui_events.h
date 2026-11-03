#pragma once

#include "pal4/ui_frame_manager_singleton.h"

#include <array>
#include <functional>

namespace pal4 {

class NativePalGameIVTextureStorage;
class NativePalGameIVStateList;

// Borrowed endpoints for the two scene UI events. This adapter implements the
// verified descriptors, not the frame manager's complete event registry.
class NativeSceneUiEvents final : private IUiFrameManagerStateEventRuntime,
                                  private IUiFrameManagerEventRuntime {
public:
    using Visibility = std::function<std::uint8_t(bool)>;
    using JumpHint = std::function<std::uint8_t(bool, const std::array<float, 2>&)>;
    NativeSceneUiEvents(NativePalGameIVTextureStorage& game,
                        NativePalGameIVStateList& states,
                        std::uint32_t manager, Visibility visibility, JumpHint jump);
    [[nodiscard]] std::uint8_t ShowSceneUi(std::uint32_t type);
    [[nodiscard]] std::uint32_t ManagerAddress() const noexcept { return manager_; }
    [[nodiscard]] std::uint8_t ShowJumpHint(
        std::uint32_t type, const std::array<float, 2>& position);

private:
    std::uint32_t GameInstance() override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t QueryGameState(std::uint32_t controller) override;
    std::uint8_t ProcessEvent(std::uint32_t tree, std::uint32_t key,
                              std::uint32_t type, std::uint32_t zero) override;
    std::uint8_t ProcessEvent(std::uint32_t tree, std::uint32_t key,
                              const UiFrameEventPayload& payload) override;
    std::uint8_t ProcessUiEvent(std::uint32_t, std::uint32_t,
                                std::uint32_t, std::uint32_t,
                                std::uint32_t) override;
    NativePalGameIVTextureStorage& game_;
    NativePalGameIVStateList& states_;
    std::uint32_t manager_;
    Visibility visibility_;
    JumpHint jump_;
};

}  // namespace pal4
