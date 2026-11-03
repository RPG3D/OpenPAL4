#pragma once

#include "pal4/preview_title_string.h"

namespace pal4 {

using MoviePreviewTitleState = PreviewTitleStringState;
using IMoviePreviewTitleRuntime = IPreviewTitleStringRuntime;
using MoviePreviewTitleRuntimeStub = PreviewTitleStringRuntimeStub;

class MoviePreviewTitle final {
public:
    static constexpr std::uint32_t kStateAddress = 0x8e4498u;
    static constexpr std::uint32_t kInlineAddress = 0x8e44acu;
    static constexpr std::uint32_t kCleanupAddress = 0x44b100u;

    [[nodiscard]] static std::uint32_t Initialize(
        MoviePreviewTitleState& state, IMoviePreviewTitleRuntime& runtime);
    static void Cleanup(MoviePreviewTitleState& state, std::uint8_t& guard,
                        IMoviePreviewTitleRuntime& runtime);
    [[nodiscard]] static int InitializeAndRegister(
        MoviePreviewTitleState& state, IMoviePreviewTitleRuntime& runtime);
};

}  // namespace pal4
