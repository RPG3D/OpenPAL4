#pragma once
#include "pal4/world_ui_frame.h"
#include <functional>

namespace pal4 {
// Borrows live world/UI owners and a caller-owned camera-pointer slot only for
// Render. No simulation, timer, camera-update, or script callbacks are exposed.
class NativeFrozenWorldUiFrame final : private IWorldUiFrameRuntime {
public:
    using World = std::function<void(std::uint32_t camera)>;
    using Ui = std::function<std::uint32_t(std::uint32_t frame_slot)>;
    NativeFrozenWorldUiFrame(std::uint32_t arena, std::uint32_t ui_manager,
                            World world, Ui ui);
    NativeFrozenWorldUiFrame(const NativeFrozenWorldUiFrame&) = delete;
    NativeFrozenWorldUiFrame& operator=(const NativeFrozenWorldUiFrame&) = delete;
    std::uint32_t Render(std::uint32_t frame_slot);
private:
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t ArenaManager() override;
    void ProcessWorldFrame(std::uint32_t arena, std::uint32_t camera) override;
    std::uint32_t UIFrameManager() override;
    std::uint32_t RenderUI(std::uint32_t manager, std::uint32_t frame) override;
    std::uint32_t arena_, manager_, frame_slot_{}, camera_{};
    World world_;
    Ui ui_;
};
}
