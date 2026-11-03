#include "pal4/preview_title_string.h"

namespace pal4 {

std::uint32_t PreviewTitleString::Initialize(
    PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime,
    std::string_view name, std::uint32_t inline_address) {
    state.Write(0u, 0u);
    state.Write(4u, 32u);
    state.Write(8u, 0u);
    state.Write(0xcu, 0u);
    state.Write(0x10u, 0u);
    state.Write(0x14u, 0u);
    runtime.Grow(state, static_cast<std::uint32_t>(name.size()));

    const auto buffer = state.Read(4u) <= 32u ?
        inline_address : state.Read(0x94u);
    for (std::uint32_t i = 0u; i < name.size(); ++i) {
        const auto value = static_cast<std::uint8_t>(name[i]);
        if (buffer == inline_address) state.Write(0x14u + i * 4u, value);
        else runtime.WriteCodepoint(buffer + i * 4u, value);
    }
    state.Write(0u, static_cast<std::uint32_t>(name.size()));
    if (buffer == inline_address)
        state.Write(0x14u + static_cast<std::uint32_t>(name.size()) * 4u, 0u);
    else
        runtime.WriteCodepoint(
            buffer + static_cast<std::uint32_t>(name.size()) * 4u, 0u);
    return buffer;
}

void PreviewTitleString::Cleanup(PreviewTitleStringState& state,
                                 std::uint8_t& guard,
                                 IPreviewTitleStringRuntime& runtime) {
    if ((guard & 1u) != 0u) return;
    guard |= 1u;
    runtime.Destroy(state);
}

int PreviewTitleString::InitializeAndRegister(
    PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime,
    std::string_view name, std::uint32_t inline_address,
    std::uint32_t cleanup_address) {
    (void)Initialize(state, runtime, name, inline_address);
    return runtime.RegisterAtExit(cleanup_address);
}

}  // namespace pal4
