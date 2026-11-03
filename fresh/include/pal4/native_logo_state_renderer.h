#pragma once
#include "pal4/logo_state.h"
#include <functional>

namespace pal4 {
class NativeLogoStateLifecycle;
class NativeScriptMoviePlayer;
class NativeBinkFramePresenter;
struct PalScreenEffectState;
class IEffectRenderRuntime;
class NativePalConsoleRenderer;
class NativePalFontManager;

struct NativeLogoRenderServices {
    std::function<void(std::uint32_t, std::uint32_t)> pre_render;
    std::function<void(std::uint32_t, std::uint32_t)> render_ui;
};

// Borrows every service. Render inside the active camera update, once per
// frame, using its RwCamera* slot. Handler-only waits own script progression;
// this renderer owns Bink decode/presentation. Leave before retiring owners.
class NativeLogoStateRenderer final : private ILogoStateRenderRuntime {
public:
    NativeLogoStateRenderer(NativeLogoStateLifecycle&, NativeScriptMoviePlayer&,
        NativeBinkFramePresenter&, PalScreenEffectState&, IEffectRenderRuntime&,
        NativePalConsoleRenderer&, NativePalFontManager&, std::uint32_t width,
        std::uint32_t height, NativeLogoRenderServices);
    void Render(std::uint32_t frame, bool obey_movie_clock = true);
private:
    friend class NativeLogoStateSession;
    float ReadFloat(std::uint32_t) override;
    std::uint32_t UIFrameManager() override;
    std::uint32_t ScreenEffect() override;
    std::uint32_t BinkPlayer() override;
    bool IsBinkPlaying(std::uint32_t) override;
    void PreRenderUI(std::uint32_t, std::uint32_t) override;
    void DrawScreenEffect(std::uint32_t, std::uint32_t) override;
    void RenderBink(std::uint32_t, std::uint32_t) override;
    void RenderUI(std::uint32_t, std::uint32_t) override;
    std::uint32_t Console() override;
    void RenderConsole(std::uint32_t, std::uint32_t) override;
    std::uint32_t GameHeight() override;
    void DrawVersionText(std::string_view, std::int32_t, std::int32_t,
        std::int32_t, std::int32_t, std::uint32_t, std::uint32_t, std::uint32_t) override;
    NativeLogoStateLifecycle& logo_;
    NativeScriptMoviePlayer& movie_;
    NativeBinkFramePresenter& presenter_;
    PalScreenEffectState& screen_;
    IEffectRenderRuntime& effects_;
    NativePalConsoleRenderer& console_;
    NativePalFontManager& fonts_;
    std::uint32_t width_, height_;
    bool obey_movie_clock_ = true;
    NativeLogoRenderServices services_;
};
}
