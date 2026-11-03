#pragma once
#include "pal4/renderware_presentation.h"

namespace pal4 {
class NativeRenderWarePresentation final : public IRenderWarePresentation {
public:
    [[nodiscard]] std::uint32_t ShowRaster(
        std::uint32_t camera, std::uint32_t window, std::uint32_t flags) override;
};
}
