#include "pal4/effect_geometry_uv_transform.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectGeometryUvRuntime {
    std::vector<std::array<std::uint32_t, 3>> calls;
    void ApplyToMaterials(std::uint32_t atomic, std::uint32_t first,
                          std::uint32_t second) override {
        calls.push_back({atomic, first, second});
    }
};

struct PipelineRuntime final : pal4::IEffectGeometryUvPipelineRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> materials{0x501u, 0x502u};
    std::uint32_t count = 2u;
    void SetupAtomic(std::uint32_t atomic) override {
        Check(atomic == 0x100u, "pipeline atomic");
        calls.push_back("setup");
    }
    std::uint32_t AtomicGeometry(std::uint32_t atomic) override {
        Check(atomic == 0x100u, "geometry owner");
        calls.push_back("geometry");
        return 0x200u;
    }
    std::uint32_t MaterialCount(std::uint32_t geometry) override {
        Check(geometry == 0x200u, "material count geometry");
        calls.push_back("count");
        return count;
    }
    std::uint32_t ForAllMaterials(
        std::uint32_t geometry,
        const std::function<void(std::uint32_t)>& callback) override {
        Check(geometry == 0x200u, "material iteration geometry");
        calls.push_back("visit");
        for (auto material : materials) callback(material);
        return 0x777u;
    }
    void SetEffects(std::uint32_t material,
                    std::uint32_t effect) override {
        calls.push_back("effect:" + std::to_string(material) + ":" +
                        std::to_string(effect));
    }
    void SetUvMatrices(std::uint32_t material, std::uint32_t first,
                       std::uint32_t second) override {
        calls.push_back("matrix:" + std::to_string(material) + ":" +
                        std::to_string(first) + ":" +
                        std::to_string(second));
    }
};

}  // namespace

int main() {
    PipelineRuntime pipeline;
    Check(pal4::EffectGeometryUvTransform::ConfigureMaterial(
              0x500u, 5u, 0x300u, 0x400u, pipeline) == 0x500u &&
              pipeline.calls == std::vector<std::string>{
                  "effect:1280:5", "matrix:1280:768:1024"},
          "single material callback sets effect before both matrices");
    pipeline.calls.clear();
    Check(pal4::EffectGeometryUvTransform::ApplyToMaterials(
              0x100u, 0x300u, 0x400u, pipeline) == 0x777u &&
              pipeline.calls == std::vector<std::string>{
                  "setup", "geometry", "count", "visit",
                  "effect:1281:6", "matrix:1281:768:1024",
                  "effect:1282:6", "matrix:1282:768:1024"},
          "multi-material path chooses effect six and forwards iterator result");
    pipeline.calls.clear();
    pipeline.count = 1u;
    pipeline.materials = {0x501u};
    Check(pal4::EffectGeometryUvTransform::ApplyToMaterials(
              0x100u, 0u, 0x400u, pipeline) == 0x777u &&
              pipeline.calls == std::vector<std::string>{
                  "setup", "geometry", "count", "visit",
                  "effect:1281:5", "matrix:1281:0:1024"},
          "single-material path chooses effect five and preserves null matrix");
    Runtime runtime;
    Check(pal4::EffectGeometryUvTransform::ApplyFirst(
              0x1000u, 0u, runtime) == 0u &&
              pal4::EffectGeometryUvTransform::ApplySecond(
                  0x1000u, 0u, runtime) == 0u &&
              runtime.calls.empty(),
          "zero matrix returns zero without material dispatch");
    Check(pal4::EffectGeometryUvTransform::ApplyFirst(
              0x1000u, 0x2000u, runtime) == 0x1000u &&
              pal4::EffectGeometryUvTransform::ApplySecond(
                  0x3000u, 0x4000u, runtime) == 0x3000u &&
              runtime.calls ==
                  std::vector<std::array<std::uint32_t, 3>>{
                      {0x1000u, 0x2000u, 0u},
                      {0x3000u, 0u, 0x4000u}},
          "UV wrappers select the exact material matrix slot");
}
