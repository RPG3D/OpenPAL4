#include "pal4/scene_sky_dual_clumps.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(const float lhs, const float rhs) {
    return std::fabs(lhs - rhs) < 0.0001f;
}

struct Runtime final : pal4::ISceneSkyDualRuntime {
    std::vector<std::string> events;
    std::vector<pal4::SkyClumpPipeline> pipelines;
    std::vector<std::uint32_t> loaded_clumps{0x5001u, 0x5002u};
    std::vector<std::uint32_t> textures{0x6001u, 0x6002u};
    std::size_t load_index = 0;
    std::size_t texture_index = 0;
    std::uint32_t next_matrix = 0x7001u;

    void SetCleanupPipeline(const std::uint32_t clump) override {
        events.emplace_back("cleanup-pipeline:" + std::to_string(clump));
    }
    void DestroyClump(const std::uint32_t clump) override {
        events.emplace_back("destroy-clump:" + std::to_string(clump));
    }
    [[nodiscard]] std::uint32_t DestroyMatrix(
        const std::uint32_t matrix) override {
        events.emplace_back("destroy-matrix:" + std::to_string(matrix));
        return matrix + 0x10u;
    }
    [[nodiscard]] std::uint32_t LoadClump(
        const std::string_view filename,
        const std::string_view source) override {
        Check(source == "sky/", "loads from caller's asset source");
        events.emplace_back("load:" + std::string(filename));
        return loaded_clumps[load_index++];
    }
    void PrepareImage(const std::string_view source) override {
        Check(source == "sky/", "prepares same image source");
        events.emplace_back("prepare-image");
    }
    [[nodiscard]] std::uint32_t ReadTexture(
        const std::string_view filename) override {
        events.emplace_back("texture:" + std::string(filename));
        return textures[texture_index++];
    }
    void SetTextureFlags(const std::uint32_t texture,
                         const std::uint8_t filter,
                         const std::uint8_t addressing) override {
        Check(texture != 0u && filter == 2u && addressing == 17u,
              "sets native texture bytes at +80/+81");
        events.emplace_back("texture-flags");
    }
    [[nodiscard]] std::uint32_t CreateMatrix() override {
        events.emplace_back("create-matrix");
        return next_matrix++;
    }
    void TranslateMatrix(const std::uint32_t matrix,
                         const std::array<float, 3>& translation,
                         const std::uint32_t mode) override {
        events.emplace_back("translate:" + std::to_string(matrix) + ":" +
                            std::to_string(mode));
        Check(std::isfinite(translation[0]), "passes finite translation");
    }
    [[nodiscard]] std::uint32_t UpdateMatrix(
        const std::uint32_t matrix) override {
        events.emplace_back("update:" + std::to_string(matrix));
        return matrix + 0x10u;
    }
    void SetClumpPipeline(
        const std::uint32_t clump,
        const pal4::SkyClumpPipeline& pipeline) override {
        Check(clump == 0x5001u || clump == 0x5002u,
              "configures loaded clump");
        events.emplace_back("pipeline:" + std::to_string(clump));
        pipelines.push_back(pipeline);
    }
    void DestroyTexture(const std::uint32_t texture) override {
        events.emplace_back("destroy-texture:" + std::to_string(texture));
    }
    void ClearVertexColor(const std::uint32_t clump,
                          const std::uint32_t mask) override {
        Check(mask == 32u, "clears native vertex flag 32");
        events.emplace_back("clear-color:" + std::to_string(clump));
    }
    void SetVertexColor(const std::uint32_t clump,
                        const std::uint32_t color) override {
        Check(color == 0xffffffffu, "sets white vertex color");
        events.emplace_back("set-color:" + std::to_string(clump));
    }
};

}  // namespace

int main() {
    using Sky = pal4::SceneSkyDualClumps;
    pal4::SceneSkyDualState state;
    state.first.first_translation[1] = 12.0f;
    state.second.second_translation[2] = 34.0f;
    state.first.clump = 99u;
    state.second.active = 1u;
    Check(&Sky::Initialize(state) == &state &&
              state.vtable == Sky::kVtable && state.first.clump == 0u &&
              state.second.active == 0u &&
              state.first.first_translation[1] == 12.0f &&
              state.second.second_translation[2] == 34.0f,
          "constructor clears only native pointers and active words");

    Runtime fail_first;
    fail_first.loaded_clumps = {0u};
    Check(!Sky::Load(state, "sky/", "sky", "cloud", "skydual",
                     "clouddual", 8u, 9u, fail_first) &&
              fail_first.events == std::vector<std::string>({
                  "load:sky/sky.dff"}) && state.first.clump == 0u,
          "first missing DFF aborts before texture setup");
    Runtime fail_second;
    fail_second.loaded_clumps = {0x5001u, 0u};
    Check(!Sky::Load(state, "sky/", "sky", "cloud", "skydual",
                     "clouddual", 8u, 9u, fail_second) &&
              state.first.clump == 0x5001u && state.first.active == 1u &&
              state.second.clump == 0u &&
              fail_second.events.back() == "load:sky/skydual.dff",
          "second missing DFF retains the already configured first layer");

    Runtime runtime;
    Check(Sky::Load(state, "sky/", "sky", "cloud", "skydual",
                    "clouddual", 8u, 9u, runtime) &&
              runtime.events[0] == "cleanup-pipeline:20481" &&
              runtime.events[1] == "destroy-clump:20481" &&
              state.first.clump == 0x5001u &&
              state.second.clump == 0x5002u &&
              state.first.active == 1u && state.second.active == 1u &&
              state.first.first_matrix == 0x7001u &&
              state.first.second_matrix == 0x7002u &&
              state.second.first_matrix == 0x7003u &&
              state.second.second_matrix == 0x7004u &&
              Near(state.first.second_translation[0], 29.9f) &&
              Near(state.second.first_translation[0], 49.9f) &&
              Near(state.second.second_translation[0], 96.9f),
          "successful load cleans old layer and configures both new layers");
    Check(runtime.pipelines.size() == 2u &&
              runtime.pipelines[0].mode == 2u &&
              runtime.pipelines[0].texture == 0x6001u &&
              runtime.pipelines[0].option_one == 8u &&
              runtime.pipelines[0].option_two == 9u &&
              runtime.pipelines[0].first_matrix == 0x7001u &&
              runtime.pipelines[0].second_matrix == 0x7002u &&
              runtime.pipelines[1].mode == 2u &&
              runtime.pipelines[1].texture == 0x6002u &&
              runtime.pipelines[1].first_matrix == 0x7003u &&
              runtime.pipelines[1].second_matrix == 0x7003u,
          "second pipeline repeats first matrix in both native slots");
    Check(runtime.events.back() == "set-color:20482",
          "vertex color setup follows second pipeline and texture release");

    runtime.events.clear();
    Check(Sky::UpdateMatrices(state, 1000.0f, runtime) == 0x7014u &&
              Near(state.first.second_translation[0], 1.9f) &&
              Near(state.second.first_translation[0], 3.9f) &&
              Near(state.second.second_translation[0], 6.9f) &&
              runtime.events == std::vector<std::string>({
                  "translate:28674:2", "update:28674",
                  "translate:28675:2", "update:28675",
                  "translate:28676:2", "update:28676"}),
          "active layers update three matrices using native multipliers");
    state.second.active = 0u;
    runtime.events.clear();
    Check(Sky::UpdateMatrices(state, 2000.0f, runtime) == 0u &&
              runtime.events.size() == 2u &&
              Near(state.first.second_translation[0], 3.8f),
          "inactive second layer skips both of its matrix updates");
    state.first.active = 0u;
    runtime.events.clear();
    Check(Sky::UpdateMatrices(state, 3000.0f, runtime) == 0u &&
              runtime.events.empty(),
          "inactive layers perform no RenderWare calls");

    runtime.events.clear();
    Check(Sky::Cleanup(state, runtime) == 0x7014u &&
              state.first.clump == 0u && state.second.clump == 0u &&
              state.first.first_matrix == 0u &&
              state.second.second_matrix == 0u &&
              runtime.events.front() == "cleanup-pipeline:20481" &&
              runtime.events.back() == "destroy-matrix:28676",
          "cleanup releases both clumps and matrices in layer order");
    state.vtable = 0u;
    Check(Sky::Destroy(state, runtime) == 0u &&
              state.vtable == Sky::kVtable,
          "destructor restores vtable before cleanup");
    Runtime no_texture;
    no_texture.textures = {0u, 0u};
    Check(Sky::Load(state, "sky/", "sky", "cloud", "skydual",
                    "clouddual", 8u, 9u, no_texture) &&
              no_texture.pipelines.size() == 2u &&
              no_texture.pipelines[0].mode == 1u &&
              no_texture.pipelines[1].mode == 2u &&
              no_texture.pipelines[0].texture == 0u &&
              no_texture.pipelines[1].texture == 0u,
          "missing textures select first-layer mode one but leave second at two");
    Check(std::find(no_texture.events.begin(), no_texture.events.end(),
                    "texture-flags") == no_texture.events.end(),
          "missing textures skip native flag writes");
    (void)Sky::Cleanup(state, no_texture);
    pal4::SceneSkyDualRuntimeStub stub;
    Check(!Sky::Load(state, "sky/", "sky", "cloud", "skydual",
                     "clouddual", 0u, 0u, stub),
          "RenderWare and asset boundaries have linkable stubs");
    return EXIT_SUCCESS;
}
