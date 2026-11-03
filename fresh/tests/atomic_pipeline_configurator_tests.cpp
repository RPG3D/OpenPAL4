#include "pal4/atomic_pipeline_configurator.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IAtomicPipelineRuntime {
    static constexpr std::uint32_t clump = 0x1000u;
    static constexpr std::uint32_t atomic = 0x2000u;
    static constexpr std::uint32_t geometry = 0x3000u;
    static constexpr std::uint32_t material = 0x4000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {atomic + 24u, geometry}, {0x950cd0u, 0x20u},
        {0x979ef8u, 0x5000u}, {0x5000u + 0x20u + 60u, 0x6000u}};
    std::vector<std::string> events;
    std::uint32_t material_pipeline = 0u;
    std::uint32_t material_effects = 9u;
    std::uint32_t second_pass_result = 0x7654u;

    [[nodiscard]] std::uint32_t ReadWord(const std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void SetupMaterialEffects(const std::uint32_t value) override {
        Check(value == atomic, "setup receives atomic");
        events.emplace_back("setup");
    }
    [[nodiscard]] std::uint32_t ForEachMaterial(
        const std::uint32_t value, const pal4::MaterialPipelinePass pass,
        const std::uint32_t data) override {
        Check(value == geometry, "material traversal receives geometry");
        if (pass == pal4::MaterialPipelinePass::set_pipeline) {
            events.emplace_back("materials:set");
            material_pipeline = data;
        } else {
            events.emplace_back("materials:clear");
            Check(data == 0u, "clear callback receives native zero data");
            (void)pal4::AtomicPipelineConfigurator::ClearMaterialEffects(
                material, *this);
        }
        return geometry;
    }
    [[nodiscard]] std::uint32_t ForEachAtomic(
        const std::uint32_t value, const pal4::AtomicPipelinePass pass,
        const std::uint32_t data) override {
        Check(value == clump, "atomic traversal receives clump");
        switch (pass) {
        case pal4::AtomicPipelinePass::set_material_pipeline:
            events.emplace_back("atomics:materials");
            (void)pal4::AtomicPipelineConfigurator::SetAtomicMaterialPipeline(
                atomic, data, *this);
            break;
        case pal4::AtomicPipelinePass::set_atomic_pipeline:
            events.emplace_back("atomics:pipeline");
            (void)pal4::AtomicPipelineConfigurator::SetAtomicPipeline(
                atomic, data, *this);
            return second_pass_result;
        case pal4::AtomicPipelinePass::clear_material_effects:
            events.emplace_back("atomics:clear");
            (void)pal4::AtomicPipelineConfigurator::ClearAtomicMaterials(
                atomic, data, *this);
            break;
        }
        return clump;
    }
    [[nodiscard]] std::uint32_t GetD3D9Pipeline(
        const std::uint32_t kind) override {
        Check(kind == 1u, "queries MatFX D3D9 pipeline kind one");
        events.emplace_back("get-d3d9");
        return 0x7000u;
    }
    void SetMaterialEffects(const std::uint32_t value,
                            const std::uint32_t effects) override {
        Check(value == material, "clear callback receives material");
        events.emplace_back("set-effects");
        material_effects = effects;
    }
};

}  // namespace

int main() {
    using Config = pal4::AtomicPipelineConfigurator;
    Runtime runtime;
    Check(Config::SetAtomicPipeline(Runtime::atomic, 0x1111u, runtime) ==
              Runtime::atomic &&
              runtime.words[Runtime::atomic + 108u] == 0x1111u,
          "atomic callback writes pipeline at +108 and returns atomic");
    runtime.events.clear();
    Check(Config::SetAtomicMaterialPipeline(Runtime::atomic, 0x2222u,
                                            runtime) == Runtime::atomic &&
              runtime.material_pipeline == 0x2222u &&
              runtime.events.front() == "setup" &&
              runtime.events.back() == "materials:set",
          "material setup precedes geometry traversal");
    runtime.events.clear();
    runtime.words[Runtime::atomic + 24u] = 0u;
    Check(Config::SetAtomicMaterialPipeline(Runtime::atomic, 3u, runtime) ==
              Runtime::atomic &&
              runtime.events.size() == 2u &&
              runtime.events.front() == "setup",
          "material setup still runs without geometry");
    runtime.events.clear();
    Check(Config::ClearAtomicMaterials(Runtime::atomic, 0u, runtime) ==
              Runtime::atomic && runtime.events.size() == 1u,
          "clear callback skips absent geometry");
    runtime.words[Runtime::atomic + 24u] = Runtime::geometry;
    runtime.events.clear();
    Check(Config::SetClumpPipeline(0u, 0x2222u, runtime) == 0u &&
              Config::SetClumpPipeline(Runtime::clump, 0u, runtime) == 0u &&
              runtime.events.empty(),
          "clump setup rejects either missing input");
    Check(Config::SetClumpPipeline(Runtime::clump, 0x2222u, runtime) == 1u &&
              runtime.material_pipeline == 0x2222u &&
              runtime.words[Runtime::atomic + 108u] == 0x7000u,
          "clump setup applies material and D3D9 atomic pipelines");
    Check(runtime.events.front() == "atomics:materials" &&
              runtime.events[1] == "setup" &&
              runtime.events[3] == "materials:set" &&
              runtime.events[4] == "get-d3d9" &&
              runtime.events[5] == "atomics:pipeline",
          "clump setup traversals and pipeline lookup retain native order");
    runtime.events.clear();
    Check(Config::ClearMaterialEffects(Runtime::material, runtime) ==
              Runtime::material && runtime.material_effects == 0u,
          "material callback clears effects and returns material");
    runtime.events.clear();
    runtime.material_effects = 9u;
    Check(Config::ClearClumpPipeline(0u, runtime) == 0u &&
              runtime.events.empty(), "null clump has no traversal");
    Check(Config::ClearClumpPipeline(Runtime::clump, runtime) ==
              runtime.second_pass_result &&
              runtime.material_effects == 0u &&
              runtime.words[Runtime::atomic + 108u] == 0x6000u,
          "clear clump effects and restore global atomic pipeline");
    Check(runtime.events.front() == "atomics:clear" &&
              runtime.events[2] == "materials:clear" &&
              runtime.events[3] == "set-effects" &&
              runtime.events[4] == "read:" + std::to_string(0x950cd0u) &&
              runtime.events[5] == "read:" + std::to_string(0x979ef8u) &&
              runtime.events[7] == "atomics:pipeline",
          "clear clump reads global pipeline after clearing materials");
    pal4::AtomicPipelineRuntimeStub stub;
    Check(Config::SetClumpPipeline(1u, 2u, stub) == 1u &&
              Config::ClearClumpPipeline(1u, stub) == 0u,
          "RenderWare boundary stub links");
    return EXIT_SUCCESS;
}
