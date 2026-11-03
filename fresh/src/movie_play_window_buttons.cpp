#include "pal4/movie_play_window_buttons.h"

#include <array>

namespace pal4 {
namespace {

struct ButtonDefinition {
    std::string_view name;
    std::uint32_t offset;
    std::uint32_t handler;
};

constexpr std::array<ButtonDefinition, 3> kButtons{{
    {"MoviePlayWindow/BtnClose", 0x34u, 0x449960u},
    {"MoviePlayWindow/BtnPause", 0x30u, 0x449970u},
    {"MoviePlayWindow/BtnPlay", 0x2cu, 0x449980u},
}};

void ReleaseReference(std::uint32_t reference,
                      IMoviePlayWindowButtonsRuntime& runtime) {
    if (reference == 0u) return;
    const auto count = runtime.ReferenceCount(reference) - 1u;
    runtime.SetReferenceCount(reference, count);
    if (count == 0u) runtime.DeleteReference(reference, 1u);
}

}  // namespace

void MoviePlayWindowButtons::Initialize(
    MoviePlayWindowState& state, std::uint32_t object_address,
    IMoviePlayWindowButtonsRuntime& runtime) {
    for (const auto& button : kButtons) {
        const auto window = runtime.FindWindow(button.name);
        state.Write(button.offset, window);
        const auto callback = runtime.AllocateCallback(0x18u);
        if (callback != 0u) {
            runtime.WriteWord(callback + 8u, button.handler);
            runtime.WriteWord(callback + 0xcu, 0u);
            runtime.WriteWord(callback + 0x10u, object_address);
            runtime.WriteWord(callback, kCallbackVtable);
        }
        ReleaseReference(runtime.SubscribeClicked(window, callback), runtime);
    }
}

}  // namespace pal4
