#include "pal4/matfx_world_sector_renderer.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

using Op = pal4::WorldSectorRenderCall;
constexpr std::uint32_t kRenderData = 0x3000u;
constexpr std::uint32_t kHeader = kRenderData + 24u;
constexpr std::uint32_t kMesh = kHeader + 72u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Event {
    Op op;
    std::vector<std::uint32_t> arguments;
    std::string text;
};

struct Runtime final : pal4::IWorldSectorRenderRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<Event> events;
    std::uint32_t render_state = 1u;
    std::uint32_t effects = 0u;
    std::uint32_t dual = 0u;
    std::uint32_t passes = 0u;

    Runtime() {
        words[kHeader + 4u] = 1u;
        words[kHeader + 8u] = 0u;
        words[kHeader + 12u] = 4u;
        words[kHeader + 48u] = 0x444u;
        words[kHeader + 52u] = 0x555u;
        words[kMesh] = 0x6000u;
        words[kMesh + 8u] = 0x666u;
        words[kMesh + 12u] = 2u;
        words[kMesh + 16u] = 3u;
        words[kMesh + 20u] = 4u;
        words[kMesh + 24u] = 5u;
        words[0x6000u] = 0x777u;
        words[0x8f94b4u] = 0x111u;
        words[0x8f94b8u] = 0x222u;
        words[0x8f94bcu] = 0x333u;
        words[0x8f94c0u] = 0x444u;
        words[0x8f94c4u] = 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t Call(Op operation,
                       std::initializer_list<std::uint32_t> arguments,
                       std::string_view text) override {
        events.push_back({operation, std::vector<std::uint32_t>(arguments),
                          std::string(text)});
        switch (operation) {
        case Op::GetRenderState:
            if (*arguments.begin() == 137u) return render_state;
            return *arguments.begin() == 26u ? 0x26u : 0x9u;
        case Op::GetMaterialEffects: return effects;
        case Op::GetDualTexture: return dual;
        case Op::VmPassCount: return passes;
        default: return 0u;
        }
    }
};

std::size_t Count(const Runtime& runtime, Op operation) {
    return static_cast<std::size_t>(std::count_if(
        runtime.events.begin(), runtime.events.end(),
        [operation](const Event& event) { return event.op == operation; }));
}

const Event& First(const Runtime& runtime, Op operation) {
    const auto found = std::find_if(
        runtime.events.begin(), runtime.events.end(),
        [operation](const Event& event) { return event.op == operation; });
    Check(found != runtime.events.end(), "expected runtime call missing");
    return *found;
}

void DirectRenderAndEarlySetup() {
    Runtime runtime;
    runtime.render_state = 0u;
    runtime.words[kHeader + 4u] = 0u;
    pal4::MatFxWorldSectorRenderer::Render(
              kRenderData, 0x99u, 0x123u, 0u, runtime);
    Check(First(runtime, Op::EnableClipping).arguments ==
              (std::vector<std::uint32_t>{0x99u, 0x23u}) &&
              First(runtime, Op::SetStreams).arguments ==
                  (std::vector<std::uint32_t>{kHeader + 16u, 0x444u}) &&
              First(runtime, Op::SetVertexDeclaration).arguments ==
                  (std::vector<std::uint32_t>{0x555u}) &&
              Count(runtime, Op::GetVertexAlphaPreference) == 0u,
          "setup truncates clipping byte and direct branch skips MatFx path");
}

void OrdinaryDynamicMeshes() {
    Runtime runtime;
    pal4::MatFxWorldSectorRenderer::Render(
              kRenderData, 0u, 1u, 8u, runtime);
    Check(First(runtime, Op::SetTextureStage).arguments ==
                  (std::vector<std::uint32_t>{0x777u, 0u}) &&
              First(runtime, Op::DrawPrimitive).arguments ==
                  (std::vector<std::uint32_t>{4u, 2u, 5u}) &&
              Count(runtime, Op::DrawIndexed) == 0u &&
              Count(runtime, Op::VmSetVector) == 0u,
          "non-dual mesh draws once without VM");
    runtime.events.clear();
    runtime.words[kHeader + 8u] = 0x888u;
    pal4::MatFxWorldSectorRenderer::Render(
              kRenderData, 0u, 1u, 8u, runtime);
    Check(First(runtime, Op::SetIndices).arguments ==
                  (std::vector<std::uint32_t>{0x888u}) &&
              First(runtime, Op::DrawIndexed).arguments ==
                  (std::vector<std::uint32_t>{4u, 2u, 0u,
                                               3u, 4u, 5u}),
          "indexed mesh passes exact draw arguments");
}

void DualTextureWithoutVm() {
    Runtime runtime;
    runtime.effects = 4u;
    runtime.dual = 0x9000u;
    pal4::MatFxWorldSectorRenderer::Render(
              kRenderData, 0u, 0u, 8u, runtime);
    Check(Count(runtime, Op::DrawPrimitive) == 0u &&
              Count(runtime, Op::VmPassCount) == 0u &&
              Count(runtime, Op::SetTextureStage) == 3u,
          "dual texture without VM skips draw but restores stage one");
    Check(runtime.events.back().op == Op::SetTextureStageState &&
              runtime.events.back().arguments ==
                  (std::vector<std::uint32_t>{1u, 4u, 1u}),
          "stage cleanup follows empty VM path");
}

void DualTextureWithVmPasses() {
    Runtime runtime;
    runtime.effects = 4u;
    runtime.dual = 0x9000u;
    runtime.passes = 2u;
    runtime.words[0x8f94c4u] = 0xa000u;
    pal4::MatFxWorldSectorRenderer::Render(
              kRenderData, 0u, 0u, 8u, runtime);
    Check(Count(runtime, Op::DrawPrimitive) == 2u &&
              Count(runtime, Op::VmSetPass) == 2u &&
              Count(runtime, Op::VmEndPass) == 2u &&
              Count(runtime, Op::VmEndTechnique) == 1u,
          "MatFx VM draws one mesh for each pass");
    Check(First(runtime, Op::VmSetVector).arguments ==
              (std::vector<std::uint32_t>{0xa000u, 0x222u,
                                           0x333u, 0x444u, 3u}) &&
              First(runtime, Op::VmSetVector).text == "Saturation" &&
              First(runtime, Op::VmSetFloat).arguments ==
                  (std::vector<std::uint32_t>{0xa000u, 0x111u}) &&
              First(runtime, Op::VmSetFloat).text == "Brightness" &&
              First(runtime, Op::VmFindTechnique).text == "RtkWorldLtMap",
          "VM uses fixed MatFx globals and technique name");
}

}  // namespace

int main() {
    DirectRenderAndEarlySetup();
    OrdinaryDynamicMeshes();
    DualTextureWithoutVm();
    DualTextureWithVmPasses();
}
