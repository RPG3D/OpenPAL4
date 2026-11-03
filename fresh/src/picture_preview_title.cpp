#include "pal4/picture_preview_title.h"

namespace pal4 {

std::uint32_t PicturePreviewTitle::Initialize(
    PicturePreviewTitleState& state, IPicturePreviewTitleRuntime& runtime) {
    return PreviewTitleString::Initialize(
        state, runtime, "biaoti20", kInlineAddress);
}

void PicturePreviewTitle::Cleanup(PicturePreviewTitleState& state,
                                  std::uint8_t& guard,
                                  IPicturePreviewTitleRuntime& runtime) {
    PreviewTitleString::Cleanup(state, guard, runtime);
}

int PicturePreviewTitle::InitializeAndRegister(
    PicturePreviewTitleState& state, IPicturePreviewTitleRuntime& runtime) {
    return PreviewTitleString::InitializeAndRegister(
        state, runtime, "biaoti20", kInlineAddress, kCleanupAddress);
}

}  // namespace pal4
