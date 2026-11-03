#pragma once
#include "pal4/ui_frame_transition_state.h"
#include "pal4/native_frozen_world_ui_frame.h"
#include <array>
#include <functional>
namespace pal4 {
class ISystemMenuRuntime;
class NativeModalFrameStateOwner final {
public:
    std::uint32_t Construct();
    void Destroy()noexcept{image_={};}
    std::uint32_t Address()const noexcept;
    std::uint32_t Frame()const noexcept{return image_[1];}
    void SetFrame(std::uint32_t value){image_[1]=value;}
private:std::array<std::uint32_t,2> image_{};
};
struct NativeModalFrameServices {
    std::function<std::optional<std::uint32_t>(std::uint32_t,std::string_view)> config;
    std::function<bool(std::uint32_t)> visible;
    std::function<bool()> return_scene;
    std::function<void(std::uint32_t)> world_frame;
};
// Borrow the real UI backend and caller's camera slot; use58FE50..FEF0 and58FFA0.
class NativeModalFrameStateRuntime final:private IUiFrameTransitionRuntime {
public:
    NativeModalFrameStateRuntime(NativeModalFrameStateOwner&,ISystemMenuRuntime&,
        std::uint32_t game,std::uint32_t arena,NativeModalFrameServices);
    bool Enter(std::uint32_t config);
    void Leave();
    std::uint8_t Update();
    std::uint32_t Render(std::uint32_t frame);
private:
    std::uint32_t ReadWord(std::uint32_t)override;
    void WriteWord(std::uint32_t,std::uint32_t)override;
    std::uint32_t UIFrameManager()override;
    void SetUIFlag(std::uint32_t,std::uint32_t)override;
    std::uint32_t Console()override;
    std::optional<std::uint32_t> ReadIntegerAttribute(std::uint32_t,std::string_view)override;
    std::uint8_t IsDialogVisible(std::uint32_t)override;
    std::uint32_t GameInstance()override;
    std::uint8_t InitializeStateAndUI(std::uint32_t,std::uint32_t)override;
    NativeModalFrameStateOwner& owner_;ISystemMenuRuntime& ui_;std::uint32_t game_;NativeModalFrameServices services_;
    NativeFrozenWorldUiFrame frozen_frame_;
};
}
