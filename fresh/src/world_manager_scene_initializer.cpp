#include "pal4/world_manager_scene_initializer.h"
#include "pal4/pal_game_iv.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t WorldManagerSceneInitializer::Initialize(
    const std::uint32_t world, const std::string_view region,
    const std::string_view scene,
    IWorldManagerSceneInitializationRuntime& runtime) {
    (void)ClearPipeline(world, runtime);
    std::uint32_t script = 0u;
    if (runtime.ReadWord(runtime.GameStateAddress() + 1420u) != 0u) {
        std::string path = "gamedata\\PALWorld\\";
        path += region;
        path += '\\';
        path += scene;
        path += ".cpk";
        script = runtime.LoadPackage(path, kPackageCallback);
        if (script != 0u) runtime.ProcessScript(script);
    }

    const auto base = runtime.ConcatenateWorldBase(
        0x94d560u, 0x94d554u);
    runtime.InitializeWorld(world, base, region, scene,
                            runtime.GameInstance() +
                                PalGameIV::kTextureDictionaryOffset);
    runtime.AddLight(world, {1.0f, 1.0f, 1.0f, 1.0f});
    runtime.WriteWord(0x8f94b4u, 0x3e99999au);
    runtime.WriteWord(0x8f94b8u, 0x3f800000u);
    runtime.WriteWord(0x8f94bcu, 0x3f800000u);
    runtime.WriteWord(0x8f94c0u, 0x3f800000u);

    if (LoadLightmapConfig(world, runtime)) {
        auto& pipeline_runtime = runtime.PipelineRuntime();
        (void)pipeline_runtime.DestroyPipeline(runtime.ReadWord(world + 228u));
        const auto pipeline = MatFxWorldSectorPipeline::Create(pipeline_runtime);
        runtime.WriteWord(world + 228u, pipeline);
        if (pipeline != 0u)
            runtime.BindWorldSectors(runtime.ReadWord(world + 16u),
                                     kSectorCallback, pipeline);
    }

    for (const auto offset : {52u, 72u, 92u}) {
        const auto clump = runtime.ReadWord(world + offset);
        if (clump == 0u) continue;
        runtime.AddClump(world, clump);
        runtime.ProcessPluginAtomics(clump, world + 196u);
        if (offset != 92u) runtime.ProcessClumpAtomics(clump);
    }
    if (script != 0u) {
        runtime.CleanupAllScripts();
        runtime.DestroyScript(script);
    }
    return runtime.FinishWorldBase(base);
}

bool WorldManagerSceneInitializer::LoadLightmapConfig(
    const std::uint32_t world,
    IWorldManagerLightmapRuntime& runtime) {
    std::string path(runtime.ReadCString(runtime.ReadWord(world + 4u)));
    path += runtime.ReadCString(runtime.ReadWord(world + 12u));
    path += "_ltMap.cfg";
    const auto file = runtime.OpenFile(path, "rb");
    if (file == 0u) return false;
    runtime.ReadFile(0x8f94b8u, 12u, 1u, file);
    runtime.ReadFile(0x8f94b4u, 4u, 1u, file);
    runtime.CloseFile(file);
    return true;
}

std::uint32_t WorldManagerSceneInitializer::ClearPipeline(
    const std::uint32_t world,
    IWorldManagerSceneInitializationRuntime& runtime) {
    const auto pipeline = runtime.ReadWord(world + 228u);
    if (pipeline == 0u) return 0u;
    const auto result = runtime.PipelineRuntime().DestroyPipeline(pipeline);
    runtime.WriteWord(world + 228u, 0u);
    return result;
}

}  // namespace pal4
