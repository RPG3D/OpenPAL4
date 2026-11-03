#pragma once

#include "pal4/native_effect_render_runtime.h"
#include "pal4/native_movie_frame_sink.h"

#include <cstdint>
#include <span>

namespace pal4 {

// Uploads decoded BGRA pixels through the RenderWare SDK and draws them into
// the active camera using the existing screen rectangle adapter.
class NativeBinkFramePresenter final : public INativeMovieFrameSink {
public:
    ~NativeBinkFramePresenter();
    NativeBinkFramePresenter() = default;
    NativeBinkFramePresenter(const NativeBinkFramePresenter&) = delete;
    NativeBinkFramePresenter& operator=(const NativeBinkFramePresenter&) = delete;

    [[nodiscard]] bool Upload(std::span<const std::uint8_t> bgra_pixels,
                              std::uint32_t width,
                              std::uint32_t height) override;
    [[nodiscard]] bool Draw(std::uint32_t camera_slot,
                            std::uint32_t width, std::uint32_t height);

private:
    NativeEffectRenderRuntime render_;
    std::uint32_t texture_{};
    std::uint32_t width_{};
    std::uint32_t height_{};
};

}  // namespace pal4
