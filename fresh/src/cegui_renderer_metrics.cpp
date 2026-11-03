#include "pal4/cegui_renderer_metrics.h"

namespace pal4 {

std::uint8_t CeguiRendererMetrics::SetQueueEnabled(
    CeguiRendererState& state, const std::uint8_t value) noexcept {
    state.WriteByte(260u, value);
    return value;
}

std::uint8_t CeguiRendererMetrics::QueueEnabled(
    const CeguiRendererState& state) noexcept {
    return state.ReadByte(260u);
}

double CeguiRendererMetrics::MainRectWidth(
    const CeguiRendererState& state) noexcept {
    return static_cast<double>(state.ReadFloat(256u)) -
           static_cast<double>(state.ReadFloat(252u));
}

double CeguiRendererMetrics::MainRectHeight(
    const CeguiRendererState& state) noexcept {
    return static_cast<double>(state.ReadFloat(248u)) -
           static_cast<double>(state.ReadFloat(244u));
}

std::array<float, 2>& CeguiRendererMetrics::MainRectSize(
    const CeguiRendererState& state,
    std::array<float, 2>& output) noexcept {
    const auto height = MainRectHeight(state);
    output[0] = static_cast<float>(MainRectWidth(state));
    output[1] = static_cast<float>(height);
    return output;
}

std::array<std::uint32_t, 4>& CeguiRendererMetrics::CopyMainRect(
    const CeguiRendererState& state,
    std::array<std::uint32_t, 4>& output) noexcept {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        output[index] = state.ReadDword(244u + 4u * index);
    }
    return output;
}

std::uint32_t CeguiRendererMetrics::Constant96() noexcept { return 96u; }

std::array<std::uint32_t, 4>& CeguiRendererMetrics::CopyWideRect(
    const CeguiRendererState& state,
    std::array<std::uint32_t, 4>& output) noexcept {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        output[index] = state.ReadDword(284u + 4u * index);
    }
    return output;
}

}  // namespace pal4
