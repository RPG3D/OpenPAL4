#pragma once
#include "pal4/native_bink_state_dispatch.h"
#include "pal4/opening_video_controller.h"
#include "pal4/native_state_script_frame_services.h"
#include <array>

namespace pal4 {
class NativeScriptMoviePlayer;
class NativeMovieOwnerRouter;
class NativeBinkFramePresenter;
class NativeScriptFrameRunner;
class INativeScriptHandlerWaitRuntime;
struct PalScreenEffectState;
class IEffectRenderRuntime;
class IPreteritRuntime;
class PalConsoleState;

// Owns the original eight-byte state12 image; borrows actual game services.
// Render owns decode; Update owns waits/script and screen advancement.
class NativeOpeningVideoState final : public INativeBinkStateDispatch,
                                      private IOpeningVideoRuntime {
public:
    NativeOpeningVideoState(std::uint32_t ui_manager, NativeScriptMoviePlayer&,
        NativeBinkFramePresenter&, NativeScriptFrameRunner&,
        INativeScriptHandlerWaitRuntime&, PalScreenEffectState&,
        IEffectRenderRuntime&, IPreteritRuntime&, PalConsoleState&,
        INativeBinkStateHost&, std::uint32_t width, std::uint32_t height);
    NativeOpeningVideoState(std::uint32_t ui_manager, NativeScriptMoviePlayer&,
        NativeBinkFramePresenter&, NativeStateScriptFrameServices,
        INativeScriptHandlerWaitRuntime&, PalScreenEffectState&,
        IEffectRenderRuntime&, IPreteritRuntime&, PalConsoleState&,
        INativeBinkStateHost&, std::uint32_t width, std::uint32_t height);
    ~NativeOpeningVideoState() override;
    NativeOpeningVideoState(const NativeOpeningVideoState&) = delete;
    NativeOpeningVideoState& operator=(const NativeOpeningVideoState&) = delete;
    std::uint32_t Address() const override;
    bool Enter(std::uint32_t previous_id, std::uint32_t argument) override;
    void Leave() override;
    void Render(std::uint32_t frame, bool obey_movie_clock = true) override;
    std::uint32_t Update(float elapsed) override;
    bool Entered() const noexcept { return entered_; }
    // Borrow one router for render, completion and playback checks. Configure
    // before entering state12; the router must outlive this state.
    void BindMovieRouter(NativeMovieOwnerRouter&);
    std::uint32_t PreviousStateId() const noexcept { return words_[1]; }
private:
    std::uint32_t ReadWord(std::uint32_t) override;
    void WriteWord(std::uint32_t, std::uint32_t) override;
    void Free(std::uint32_t) override;
    std::uint32_t UIFrameManager() override;
    void SetUIFlag(std::uint32_t, std::uint32_t) override;
    std::uint32_t InitializeConsole() override;
    std::uint32_t BinkPlayer() override;
    void RenderVideo(std::uint32_t, std::uint32_t) override;
    std::uint8_t IsPlaying(std::uint32_t) override;
    std::uint32_t ScriptExecutor() override;
    void ProcessScriptWaitHandlers(std::uint32_t, double) override;
    std::uint32_t ScreenEffect() override;
    void AdvanceScreenEffect(std::uint32_t, float) override;
    std::uint32_t ConstructXmlDocument() override;
    std::uint32_t GameInstance() override;
    void InitializeStateAndUI(std::uint32_t, std::uint32_t, std::uint32_t) override;
    std::uint32_t DestroyXmlDocument(std::uint32_t) override;
    std::array<std::uint32_t, 2> words_{};
    std::uint32_t manager_, width_, height_, render_frame_ = 0u;
    NativeScriptMoviePlayer& movie_;
    NativeMovieOwnerRouter* movie_router_{};
    NativeBinkFramePresenter& presenter_;
    NativeStateScriptFrameServices script_;
    INativeScriptHandlerWaitRuntime& waits_;
    PalScreenEffectState& screen_;
    IEffectRenderRuntime& effects_;
    IPreteritRuntime& preterit_;
    PalConsoleState& console_;
    INativeBinkStateHost& host_;
    bool entered_ = false, obey_movie_clock_ = true;
};
}
