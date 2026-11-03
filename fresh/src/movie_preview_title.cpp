#include "pal4/movie_preview_title.h"

namespace pal4 {

std::uint32_t MoviePreviewTitle::Initialize(
    MoviePreviewTitleState& state, IMoviePreviewTitleRuntime& runtime) {
    return PreviewTitleString::Initialize(
        state, runtime, "biaoti30", kInlineAddress);
}

void MoviePreviewTitle::Cleanup(MoviePreviewTitleState& state,
                                std::uint8_t& guard,
                                IMoviePreviewTitleRuntime& runtime) {
    PreviewTitleString::Cleanup(state, guard, runtime);
}

int MoviePreviewTitle::InitializeAndRegister(
    MoviePreviewTitleState& state, IMoviePreviewTitleRuntime& runtime) {
    return PreviewTitleString::InitializeAndRegister(
        state, runtime, "biaoti30", kInlineAddress, kCleanupAddress);
}

}  // namespace pal4
