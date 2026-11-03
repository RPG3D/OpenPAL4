#pragma once

#include "pal4/preview_title_string.h"

#include <cstdint>

namespace pal4 {

class RoleStateReceiverNames final {
public:
    static constexpr std::uint32_t kDragInline = 0x8e78ccu;
    static constexpr std::uint32_t kCanInline = 0x8e7834u;
    static constexpr std::uint32_t kDragCleanup = 0x474150u;
    static constexpr std::uint32_t kCanCleanup = 0x474230u;

    [[nodiscard]] static std::uint32_t CreateDrag(
        PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime);
    [[nodiscard]] static int InitializeDrag(
        PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime);
    static void DestroyDrag(PreviewTitleStringState& state,
                            IPreviewTitleStringRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateCan(
        PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime);
    [[nodiscard]] static int InitializeCan(
        PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime);
    static void DestroyCan(PreviewTitleStringState& state,
                           IPreviewTitleStringRuntime& runtime);
};

}  // namespace pal4
