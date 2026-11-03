#include "pal4/cegui_render_helpers.h"

namespace pal4 {

std::uint32_t CeguiRenderHelperStub::ReadDword(
    const std::uint32_t address) {
    return words_.at(address);
}

std::uint32_t CeguiRenderHelperStub::SetRenderState(
    std::uint32_t /*state_id*/, std::uint32_t /*value*/) {
    return 0u;
}

std::uint32_t CeguiRenderHelperStub::DrawIndexed(
    std::uint32_t /*primitive_type*/, std::uint32_t /*vertices*/,
    std::uint32_t /*vertex_count*/, std::uint32_t /*indices*/,
    std::uint32_t /*index_count*/) {
    return 0u;
}

void CeguiRenderHelperStub::WriteDword(
    const std::uint32_t address, const std::uint32_t value) {
    words_[address] = value;
}

std::uint32_t CeguiRenderHelpers::MinGameStateExtent(
    const std::uint32_t game_state_base,
    ICeguiRenderHelperRuntime& runtime) {
    const auto record = runtime.ReadDword(game_state_base + 0x55cu);
    const auto second = runtime.ReadDword(record + 0x5cu);
    const auto first = runtime.ReadDword(record + 0x58u);
    return second < first ? second : first;
}

std::uint32_t CeguiRenderHelpers::SetRenderStates(
    ICeguiRenderHelperRuntime& runtime) {
    constexpr std::uint32_t ids[6]{12u, 6u, 8u, 7u, 9u, 2u};
    constexpr std::uint32_t values[6]{1u, 0u, 0u, 2u, 2u, 3u};
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        result = runtime.SetRenderState(ids[index], values[index]);
    }
    return result;
}

std::optional<std::uint32_t> CeguiRenderHelpers::FlushGeometry(
    CeguiRendererState& state, ICeguiRenderHelperRuntime& runtime) {
    const auto count = state.ReadDword(264u);
    if (count == 0u) {
        return std::nullopt;
    }
    const auto result = runtime.DrawIndexed(
        3u, state.ReadDword(204u), count,
        state.ReadDword(220u), 6u * (count / 4u));
    state.WriteDword(264u, 0u);
    return result;
}

}  // namespace pal4
