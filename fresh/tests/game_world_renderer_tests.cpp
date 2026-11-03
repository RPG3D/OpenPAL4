#include "pal4/game_world_renderer.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameWorldRenderRuntime {
public:
    void RenderWorld(std::uint32_t world) override {
        ++calls;
        last_world = world;
    }
    void UpdateUvAnimation(pal4::GameWorldObjectState& state,
                           std::uint32_t word_offset,
                           std::uint32_t delta_bits) override {
        Check(delta_bits == 0x3f000000u && state.words[12] == 1u,
              "animation receives unchanged float bits and ready state");
        offsets.push_back(word_offset);
    }
    unsigned calls = 0;
    std::uint32_t last_world = 0u;
    std::vector<std::uint32_t> offsets;
};

}  // namespace

int main() {
    pal4::GameWorldObjectState state;
    state.words[4] = 0x9000u;
    Runtime runtime;
    Check(!pal4::GameWorldRenderer::Render(state, runtime) &&
              runtime.calls == 0u,
          "unready world is not rendered");
    Check(!pal4::GameWorldRenderer::UpdateRenderParameters(
              state, 0x3f000000u, runtime) && runtime.offsets.empty(),
          "unready world does not update UV animations");
    state.words[12] = 1u;
    Check(pal4::GameWorldRenderer::Render(state, runtime) &&
              runtime.calls == 1u && runtime.last_world == 0x9000u,
          "ready world is rendered and reports success");
    Check(pal4::GameWorldRenderer::UpdateRenderParameters(
              state, 0x3f000000u, runtime) &&
              runtime.offsets == std::vector<std::uint32_t>{
                  7u, 14u, 19u, 24u, 29u, 34u},
          "six UV animation facets update in native order");
    state.words[4] = 0u;
    Check(pal4::GameWorldRenderer::Render(state, runtime) &&
              runtime.calls == 2u && runtime.last_world == 0u,
          "ready flag alone gates the native render call");
}
