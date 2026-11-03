#pragma once

#include "pal4/cegui_renderer.h"

#include <array>
#include <cstdint>

namespace pal4 {

class CeguiRendererMetrics final {
public:
    // 0x415c10 / 0x415c20: renderer byte +260.
    [[nodiscard]] static std::uint8_t SetQueueEnabled(
        CeguiRendererState& state, std::uint8_t value) noexcept;
    [[nodiscard]] static std::uint8_t QueueEnabled(
        const CeguiRendererState& state) noexcept;

    // 0x415c30 / 0x415c40 / 0x415c50.
    [[nodiscard]] static double MainRectWidth(
        const CeguiRendererState& state) noexcept;
    [[nodiscard]] static double MainRectHeight(
        const CeguiRendererState& state) noexcept;
    [[nodiscard]] static std::array<float, 2>& MainRectSize(
        const CeguiRendererState& state,
        std::array<float, 2>& output) noexcept;

    // 0x415c80 / 0x415cb0 / 0x415cc0.
    [[nodiscard]] static std::array<std::uint32_t, 4>& CopyMainRect(
        const CeguiRendererState& state,
        std::array<std::uint32_t, 4>& output) noexcept;
    [[nodiscard]] static std::uint32_t Constant96() noexcept;
    [[nodiscard]] static std::array<std::uint32_t, 4>& CopyWideRect(
        const CeguiRendererState& state,
        std::array<std::uint32_t, 4>& output) noexcept;
};

}  // namespace pal4
