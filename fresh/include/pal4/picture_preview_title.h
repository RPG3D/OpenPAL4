#pragma once

#include "pal4/preview_title_string.h"

namespace pal4 {

using PicturePreviewTitleState = PreviewTitleStringState;
using IPicturePreviewTitleRuntime = IPreviewTitleStringRuntime;
using PicturePreviewTitleRuntimeStub = PreviewTitleStringRuntimeStub;

class PicturePreviewTitle final {
public:
    static constexpr std::uint32_t kStateAddress = 0x8e43f0u;
    static constexpr std::uint32_t kInlineAddress = 0x8e4404u;
    static constexpr std::uint32_t kCleanupAddress = 0x44bfa0u;

    [[nodiscard]] static std::uint32_t Initialize(
        PicturePreviewTitleState& state, IPicturePreviewTitleRuntime& runtime);
    static void Cleanup(PicturePreviewTitleState& state, std::uint8_t& guard,
                        IPicturePreviewTitleRuntime& runtime);
    [[nodiscard]] static int InitializeAndRegister(
        PicturePreviewTitleState& state, IPicturePreviewTitleRuntime& runtime);
};

}  // namespace pal4
