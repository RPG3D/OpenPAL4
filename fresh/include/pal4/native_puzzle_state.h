#pragma once
#include "pal4/puzzle_ui_state.h"
#include "pal4/world_ui_frame.h"
#include <functional>

namespace pal4 {
class NativePuzzleStateOwner final {
public:
    std::uint32_t Construct();
    void Destroy() noexcept { image_=0u; }
    std::uint32_t Address() const noexcept;
private:std::uint32_t image_{};
};
struct NativePuzzleStateServices {
    std::uint32_t manager{},console{};
    std::function<std::uint32_t(std::uint32_t)> render;
    std::function<std::optional<std::uint32_t>(std::uint32_t,std::string_view)> config;
    std::function<void(std::int32_t mode,std::int32_t type)> show;
    std::function<std::uint32_t(double)> update;
    std::function<void(std::uint32_t camera)> world_frame;
};
// Borrows actual UI/config/world owners. Shared state8 code owns the Mode/Type
// dispatch and updates; its58FFA0 rendering is shared with Save9/ModalFrame10.
class NativePuzzleStateRuntime final : private IPuzzleUiRuntime,private IWorldUiFrameRuntime {
public:
    NativePuzzleStateRuntime(std::uint32_t arena,NativePuzzleStateServices);
    bool Enter(std::uint32_t config);
    void Leave() noexcept {} // Actual5A92E0 is the no-op return-one exit slot.
    std::uint32_t Update(float);
    std::uint32_t Render(std::uint32_t frame);
private:
    std::uint32_t UIFrameManager() override;
    void SetUIFlag(std::uint32_t,std::uint32_t) override;
    std::uint32_t Console() override;
    void WriteWord(std::uint32_t,std::uint32_t) override;
    std::optional<std::uint32_t> ReadIntegerAttribute(std::uint32_t,std::string_view) override;
    void DispatchEvent(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override;
    std::uint32_t UpdateUI(std::uint32_t,double) override;
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint32_t ArenaManager() override;
    void ProcessWorldFrame(std::uint32_t,std::uint32_t) override;
    std::uint32_t RenderUI(std::uint32_t,std::uint32_t) override;
    std::uint32_t arena_,frame_{};NativePuzzleStateServices services_;
};
} // namespace pal4
