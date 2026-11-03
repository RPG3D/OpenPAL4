#pragma once
#include <cstdint>

namespace pal4 {
class IRenderWarePresentation {
public:
    virtual ~IRenderWarePresentation() = default;
    [[nodiscard]] virtual std::uint32_t ShowRaster(
        std::uint32_t camera, std::uint32_t window, std::uint32_t flags) = 0;
};
}
