#include "pal4/rw_clump_material_pipeline.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

using C = pal4::RwClumpMaterialCallback;

struct Event final {
    char kind;
    C callback;
    std::uint32_t data;
    bool operator==(const Event&) const = default;
};

class Runtime final : public pal4::IRwClumpMaterialPipelineRuntime {
public:
    std::vector<Event> calls;
    std::uint32_t traversal_result = 0;

    std::uint32_t ForAllAtomics(std::uint32_t clump,
                                C callback,
                                std::uint32_t data) override {
        Check(clump == 0x1000u, "passes the original clump to traversal");
        calls.push_back({'F', callback, data});
        return traversal_result;
    }
    std::uint32_t NormalPipeline() override {
        calls.push_back({'N', C::SetPipeline, 0});
        return 0x2000u;
    }
    std::uint32_t SkinPipeline(std::uint32_t type) override {
        calls.push_back({'S', C::SetPipeline, type});
        return 0x3000u;
    }
    std::uint32_t MaterialEffectPipeline(
        std::uint32_t type) override {
        calls.push_back({'M', C::SetPipeline, type});
        return 0x4000u;
    }
    std::uint32_t DefaultPipeline() override {
        calls.push_back({'D', C::SetPipeline, 0});
        return 0x5000u;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(!pal4::RwClumpMaterialPipeline::SetNormalMap(
              0u, 0x22u, runtime) &&
              !pal4::RwClumpMaterialPipeline::SetNormalMap(
                  0x1000u, 0u, runtime) &&
              !pal4::RwClumpMaterialPipeline::SetNormalMapTextures(
                  0u, 0x22u, runtime) &&
              !pal4::RwClumpMaterialPipeline::SetNormalMapTextures(
                  0x1000u, 0u, runtime) &&
              !pal4::RwClumpMaterialPipeline::Select(
                  0u, true, true, runtime) &&
              !pal4::RwClumpMaterialPipeline::SetEnvironmentMap(
                  0u, 0x22u, runtime) &&
              !pal4::RwClumpMaterialPipeline::SetEnvironmentMap(
                  0x1000u, 0u, runtime) &&
              !pal4::RwClumpMaterialPipeline::SetMaterialEffect(
                  0u, 0x22u, runtime) &&
              !pal4::RwClumpMaterialPipeline::SetMaterialEffect(
                  0x1000u, 0u, runtime) &&
              pal4::RwClumpMaterialPipeline::Reset(0u, runtime) == 0u &&
              runtime.calls.empty(),
          "null inputs stop all six entrypoints before RenderWare calls");

    Check(pal4::RwClumpMaterialPipeline::SetNormalMap(
              0x1000u, 0x22u, runtime) &&
              runtime.calls == std::vector<Event>{
                  {'F', C::SetNormalMap, 0x22u},
                  {'N', C::SetPipeline, 0},
                  {'F', C::SetPipeline, 0x2000u}},
          "normal-map setup traverses materials before installing pipeline");
    runtime.calls.clear();
    Check(pal4::RwClumpMaterialPipeline::SetNormalMapTextures(
              0x1000u, 0x23u, runtime) &&
              runtime.calls == std::vector<Event>{
                  {'F', C::SetNormalMapTextures, 0x23u}},
          "texture-only normal-map path uses one callback");

    runtime.calls.clear();
    Check(pal4::RwClumpMaterialPipeline::Select(
              0x1000u, false, true, runtime) &&
              runtime.calls == std::vector<Event>{
                  {'N', C::SetPipeline, 0},
                  {'F', C::SetPipeline, 0x2000u}},
          "normal-map selection uses global pipeline");
    runtime.calls.clear();
    Check(pal4::RwClumpMaterialPipeline::Select(
              0x1000u, true, true, runtime) &&
              runtime.calls == std::vector<Event>{
                  {'N', C::SetPipeline, 0},
                  {'F', C::SetPipeline, 0x2000u}},
          "normal-map selection wins over skin branch");
    runtime.calls.clear();
    Check(pal4::RwClumpMaterialPipeline::Select(
              0x1000u, true, false, runtime) &&
              runtime.calls == std::vector<Event>{
                  {'S', C::SetPipeline, 1u},
                  {'F', C::SetPipeline, 0x3000u}},
          "skin selection requests type-one D3D9 pipeline");
    runtime.calls.clear();
    Check(pal4::RwClumpMaterialPipeline::Select(
              0x1000u, false, false, runtime) &&
              runtime.calls == std::vector<Event>{
                  {'D', C::SetPipeline, 0},
                  {'F', C::SetPipeline, 0x5000u}},
          "ordinary selection uses default pipeline");

    runtime.calls.clear();
    Check(pal4::RwClumpMaterialPipeline::SetEnvironmentMap(
              0x1000u, 0x24u, runtime) &&
              runtime.calls == std::vector<Event>{
                  {'F', C::SetEnvironmentMap, 0x24u},
                  {'M', C::SetPipeline, 1u},
                  {'F', C::SetPipeline, 0x4000u}},
          "environment mapping sets texture before MatFX pipeline");
    runtime.calls.clear();
    Check(pal4::RwClumpMaterialPipeline::SetMaterialEffect(
              0x1000u, 0x25u, runtime) &&
              runtime.calls == std::vector<Event>{
                  {'F', C::SetMaterialEffect, 0x25u},
                  {'M', C::SetPipeline, 1u},
                  {'F', C::SetPipeline, 0x4000u}},
          "material effect sets material before MatFX pipeline");

    runtime.calls.clear();
    runtime.traversal_result = 0x9876u;
    Check(pal4::RwClumpMaterialPipeline::Reset(
              0x1000u, runtime) == 0x9876u &&
              runtime.calls == std::vector<Event>{
                  {'F', C::ResetMaterialEffect, 0},
                  {'D', C::SetPipeline, 0},
                  {'F', C::SetPipeline, 0x5000u}},
          "reset forwards second traversal result after default pipeline");

    std::cout << "rw_clump_material_pipeline_tests passed\n";
}
