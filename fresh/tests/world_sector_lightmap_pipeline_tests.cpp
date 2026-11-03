#include "pal4/world_sector_lightmap_pipeline.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using Op = pal4::WorldSectorRenderCall;
using Pipeline = pal4::WorldSectorLightmapPipeline;

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Invocation {
    Op op;
    std::vector<std::uint32_t> args;
    std::string text;
    bool operator==(const Invocation&) const = default;
};

struct Runtime final : pal4::IWorldSectorRenderRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> states;
    std::map<std::uint32_t, std::uint32_t> effects;
    std::map<std::uint32_t, std::uint32_t> duals;
    std::map<std::uint32_t, std::uint32_t> alpha;
    std::vector<Invocation> calls;
    std::uint32_t pipeline = 0x1000u;
    std::uint32_t locked = 0x3000u;
    std::uint32_t fragment = 1u;
    std::uint32_t unlocked = 1u;
    std::uint32_t vm = 0u;
    std::uint32_t passes = 0u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t Call(const Op op,
                       const std::initializer_list<std::uint32_t> args,
                       const std::string_view text) override {
        calls.push_back({op, args, std::string(text)});
        switch (op) {
        case Op::CreatePipeline: return pipeline;
        case Op::GetNodeDefinition: return 0x2000u;
        case Op::LockPipeline: return locked;
        case Op::AddFragment: return fragment;
        case Op::UnlockPipeline: return unlocked;
        case Op::FindNodeByName: return 0x4000u;
        case Op::GetRenderState: return states[args.begin()[0]];
        case Op::GetMaterialEffects: return effects[args.begin()[0]];
        case Op::GetDualTexture: return duals[args.begin()[0]];
        case Op::TextureHasAlpha: return alpha[args.begin()[0]];
        case Op::GetEffectVm: return vm;
        case Op::GetArenaManager: return 0x5000u;
        case Op::VmPassCount: return passes;
        default: return 0u;
        }
    }
    std::size_t Count(const Op op) const {
        return static_cast<std::size_t>(std::count_if(
            calls.begin(), calls.end(),
            [op](const Invocation& call) { return call.op == op; }));
    }
    const Invocation& First(const Op op) const {
        const auto it = std::find_if(
            calls.begin(), calls.end(),
            [op](const Invocation& call) { return call.op == op; });
        Check(it != calls.end(), "expected pipeline operation");
        return *it;
    }
};

void SetupRenderData(Runtime& runtime, const bool indexed,
                     const std::uint32_t mesh_count) {
    constexpr std::uint32_t header = 0x6018u;
    runtime.words[header + 4u] = mesh_count;
    runtime.words[header + 8u] = indexed ? 0x99u : 0u;
    runtime.words[header + 12u] = 3u;
    runtime.words[header + 48u] = 0x44u;
    runtime.words[header + 52u] = 0x55u;
    for (std::uint32_t index = 0u; index < mesh_count; ++index) {
        const auto mesh = header + 72u + index * 36u;
        const auto material = 0x7000u + index * 0x100u;
        runtime.words[mesh] = material;
        runtime.words[material] = 0x8000u + index * 0x100u;
        runtime.words[mesh + 8u] = 0xa0u + index;
        runtime.words[mesh + 12u] = 10u + index;
        runtime.words[mesh + 16u] = 20u + index;
        runtime.words[mesh + 20u] = 30u + index;
        runtime.words[mesh + 24u] = 40u + index;
    }
}

void CreateBranches() {
    Runtime runtime;
    runtime.words[0x2000u] = 0x2100u;
    Check(Pipeline::Create(runtime) == 0x1000u,
          "pipeline creation succeeds");
    Check(runtime.calls == std::vector<Invocation>{
              {Op::CreatePipeline, {}, ""},
              {Op::GetNodeDefinition, {}, ""},
              {Op::SetMipmapping, {0u}, ""},
              {Op::LockPipeline, {0x1000u}, ""},
              {Op::AddFragment, {0x3000u, 0u, 0x2000u, 0u}, ""},
              {Op::UnlockPipeline, {0x3000u}, ""},
              {Op::FindNodeByName, {0x1000u, 0x2100u, 0u, 0u}, ""},
              {Op::SetInstanceCallback, {0x4000u}, ""},
              {Op::SetRenderCallback, {0x4000u}, ""}},
          "pipeline setup preserves IDA call and argument order");

    runtime = Runtime{};
    runtime.pipeline = 0u;
    Check(Pipeline::Create(runtime) == 0u &&
              runtime.calls.size() == 3u &&
              runtime.Count(Op::DestroyPipeline) == 0u,
          "null pipeline still loads definition and disables mipmapping");
    runtime = Runtime{};
    runtime.locked = 0u;
    Check(Pipeline::Create(runtime) == 0u &&
              runtime.Count(Op::AddFragment) == 0u &&
              runtime.Count(Op::DestroyPipeline) == 1u,
          "lock failure destroys pipeline");
    runtime = Runtime{};
    runtime.fragment = 0u;
    Check(Pipeline::Create(runtime) == 0u &&
              runtime.Count(Op::UnlockPipeline) == 0u &&
              runtime.Count(Op::DestroyPipeline) == 1u,
          "fragment failure skips unlock and destroys pipeline");
    runtime = Runtime{};
    runtime.unlocked = 0u;
    Check(Pipeline::Create(runtime) == 0u &&
              runtime.Count(Op::FindNodeByName) == 0u &&
              runtime.Count(Op::DestroyPipeline) == 1u,
          "unlock failure skips callback registration");
}

void OrdinaryRenderAndAlpha() {
    Runtime runtime;
    SetupRenderData(runtime, true, 2u);
    runtime.states[26u] = 0x12u;
    runtime.states[9u] = 0x34u;
    runtime.alpha[0x8000u] = 1u;
    Pipeline::RenderCallback(0x6000u, 0x77u, 0x1234u,
                                      0x80u, runtime);
    Check(runtime.calls[0] == Invocation{Op::SetPixelShader, {0u}, ""} &&
              runtime.calls[1] ==
                  Invocation{Op::EnableClipping, {0x77u, 0x34u}, ""} &&
              runtime.First(Op::SetIndices).args ==
                  std::vector<std::uint32_t>{0x99u} &&
              runtime.First(Op::SetStreams).args ==
                  (std::vector<std::uint32_t>{0x6028u, 0x44u}) &&
              runtime.First(Op::SetVertexDeclaration).args ==
                  std::vector<std::uint32_t>{0x55u},
          "lighting entry binds indices, streams and declaration");
    Check(runtime.Count(Op::GetVertexAlphaPreference) == 0u &&
              runtime.Count(Op::DrawIndexed) == 2u &&
              runtime.Count(Op::DrawPrimitive) == 0u &&
              runtime.Count(Op::TextureHasAlpha) == 2u,
          "render flags 0x80 select alpha-aware ordinary path");
    Check(runtime.First(Op::DrawIndexed).args ==
              (std::vector<std::uint32_t>{3u, 10u, 0u, 20u, 30u, 40u}),
          "indexed primitive receives exact mesh words");
    Check(runtime.Count(Op::SetTextureStage) == 3u,
          "alpha texture binds once and is cleared on next non-alpha mesh");
    const auto last = runtime.calls.back();
    Check(last == Invocation{Op::SetRenderState, {9u, 0x34u}, ""},
          "render state 9 is restored last");
    const auto before_last = runtime.calls[runtime.calls.size() - 2u];
    Check(before_last == Invocation{Op::SetRenderState, {26u, 0x12u}, ""},
          "render state 26 is restored before state 9");
}

void DirectLightingBranches() {
    Runtime runtime;
    SetupRenderData(runtime, false, 1u);
    runtime.states[137u] = 1u;
    Pipeline::RenderCallback(0x6000u, 5u, 0u, 0u, runtime);
    Check(runtime.Count(Op::GetVertexAlphaPreference) == 1u &&
              runtime.Count(Op::SetIndices) == 0u &&
              runtime.Count(Op::DrawPrimitive) == 1u &&
              runtime.Count(Op::GetRenderState) == 1u,
          "render state 137 selects direct lighting and nonindexed draw");
    Check(runtime.First(Op::DrawPrimitive).args ==
              (std::vector<std::uint32_t>{3u, 10u, 40u}),
          "nonindexed primitive receives exact mesh words");
    runtime = Runtime{};
    SetupRenderData(runtime, false, 0u);
    Pipeline::RenderCallback(0x6000u, 0u, 0u, 8u, runtime);
    Check(runtime.Count(Op::GetVertexAlphaPreference) == 1u &&
              runtime.Count(Op::DrawPrimitive) == 0u,
          "render flag bit 8 forces direct path even without meshes");
}

void DualTextureAndVm() {
    Runtime runtime;
    SetupRenderData(runtime, true, 1u);
    runtime.states[137u] = 1u;
    runtime.effects[0x7000u] = 4u;
    runtime.duals[0x7000u] = 0x9000u;
    Pipeline::RenderCallback(0x6000u, 0u, 0u, 0u, runtime);
    Check(runtime.Count(Op::DrawIndexed) == 0u &&
              runtime.Count(Op::SetTextureStage) == 3u,
          "dual texture without VM binds and clears stages but does not draw");
    runtime.calls.clear();
    runtime.vm = 0xa000u;
    runtime.passes = 2u;
    runtime.words[0x5000u + 428u] = 0x11u;
    runtime.words[0x5000u + 432u] = 0x22u;
    runtime.words[0x5000u + 436u] = 0x33u;
    runtime.words[0x5000u + 440u] = 0x3f800000u;
    Pipeline::RenderCallback(0x6000u, 0u, 0u, 0u, runtime);
    Check(runtime.Count(Op::DrawIndexed) == 2u &&
              runtime.Count(Op::VmSetPass) == 2u &&
              runtime.Count(Op::VmEndPass) == 2u &&
              runtime.Count(Op::VmEndTechnique) == 1u,
          "dual texture draws once per VM pass and closes technique");
    Check(runtime.First(Op::VmSetVector) ==
              Invocation{Op::VmSetVector,
                         {0xa000u, 0x11u, 0x22u, 0x33u, 3u},
                         "Saturation"} &&
              runtime.First(Op::VmSetFloat) ==
                  Invocation{Op::VmSetFloat,
                             {0xa000u, 0x3f800000u}, "Brightness"} &&
              runtime.First(Op::VmFindTechnique) ==
                  Invocation{Op::VmFindTechnique, {0xa000u},
                             "RtkWorldLtMap"} &&
              runtime.Count(Op::GetArenaManager) == 4u,
          "VM receives exact arena settings and technique name");
    runtime.calls.clear();
    runtime.passes = 0u;
    Pipeline::RenderCallback(0x6000u, 0u, 0u, 0u, runtime);
    Check(runtime.Count(Op::DrawIndexed) == 0u &&
              runtime.Count(Op::VmEndTechnique) == 1u,
          "zero-pass technique still closes without drawing");
}

}  // namespace

int main() {
    CreateBranches();
    OrdinaryRenderAndAlpha();
    DirectLightingBranches();
    DualTextureAndVm();
}
