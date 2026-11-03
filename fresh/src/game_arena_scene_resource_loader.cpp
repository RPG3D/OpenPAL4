#include "pal4/game_arena_scene_resource_loader.h"
#include "pal4/game_arena_render_clump_bindings.h"

#include <initializer_list>
#include <string>

namespace pal4 {
namespace {

std::uint32_t Invoke(IGameArenaSceneResourceRuntime& runtime,
                     const ArenaSceneResourceCall operation,
                     const std::uint32_t first = 0u,
                     const std::uint32_t second = 0u,
                     const std::string_view text = {}) {
    return runtime.Call(operation, first, second, text);
}

class RenderClumps final : public IGameArenaRenderClumpRuntime {
    IGameArenaSceneResourceRuntime& runtime;
public:
    explicit RenderClumps(IGameArenaSceneResourceRuntime& r):runtime(r) {}
    std::uint32_t ReadClump(std::uint32_t address) override {return runtime.ReadWord(address);}
    void EnableDistanceOptimization() override {(void)Invoke(runtime,ArenaSceneResourceCall::InitializePluginData,1u);}
    void AddClump(std::uint32_t arena,std::uint32_t clump) override {(void)Invoke(runtime,ArenaSceneResourceCall::AddClump,arena,clump);}
    void BindAlphaQueue(std::uint32_t clump,std::uint32_t queue) override {(void)Invoke(runtime,ArenaSceneResourceCall::ProcessClumpAtomics,clump,queue);}
    void BindDistanceCallbacks(std::uint32_t clump) override {(void)Invoke(runtime,ArenaSceneResourceCall::ProcessClump,clump);}
};

}  // namespace

bool GameArenaSceneResourceLoader::Load(
    const std::uint32_t object, const std::string_view scene,
    const std::string_view section, const std::uint32_t reuse_flag,
    IGameArenaSceneResourceRuntime& runtime) {
    const auto temperature = Invoke(runtime,
                                    ArenaSceneResourceCall::InitializeTemperature);
    static_cast<void>(Invoke(runtime, ArenaSceneResourceCall::SetTemperatureState,
                             temperature, 0u));
    if (reuse_flag == 0u && runtime.ReadWord(object + 48u) != 0u) {
        const auto cache_name = runtime.ReadText(object + 224u);
        const auto world_name = runtime.ReadText(object + 8u);
        static_cast<void>(Invoke(runtime,
                                 ArenaSceneResourceCall::InitializeArenaCache));
        runtime.RemoveArenaCache(world_name, cache_name);
    }
    runtime.UpdateProgress(0.0f);
    static_cast<void>(Invoke(runtime, ArenaSceneResourceCall::InitializeArenaModule,
                             object));
    const auto camera = Invoke(runtime, ArenaSceneResourceCall::InitializeCamera);
    static_cast<void>(Invoke(runtime, ArenaSceneResourceCall::ConfigureCamera,
                             camera));
    runtime.UpdateProgress(0.1f);

    if (runtime.ArchiveModeEnabled()) {
        std::string archive("gamedata\\PALWorld\\");
        archive.append(scene);
        archive += ".cpk";
        const auto script = Invoke(runtime, ArenaSceneResourceCall::LoadArchive,
                                   0u, 0u, archive);
        runtime.WriteWord(object + 316u, script);
        if (script != 0u)
            static_cast<void>(Invoke(runtime,
                                     ArenaSceneResourceCall::ProcessArchive,
                                     script));
    }

    std::string world_root(runtime.PathRoot(ArenaScenePathRoot::WorldPrefix));
    world_root += runtime.PathRoot(ArenaScenePathRoot::WorldSuffix);
    const auto context = runtime.GameContext();
    if (!runtime.InitializeWorld(object, world_root, scene, section, context)) {
        const auto script = runtime.ReadWord(object + 316u);
        if (script != 0u) {
            static_cast<void>(Invoke(runtime,
                                     ArenaSceneResourceCall::CleanupScripts));
            static_cast<void>(Invoke(runtime,
                                     ArenaSceneResourceCall::DestroyArchive,
                                     script));
            runtime.WriteWord(object + 316u, 0u);
        }
        return false;
    }

    const auto active_camera = Invoke(runtime,
                                      ArenaSceneResourceCall::GetActiveCamera,
                                      Invoke(runtime,
                                             ArenaSceneResourceCall::InitializeCamera));
    static_cast<void>(Invoke(runtime, ArenaSceneResourceCall::SetWorldCamera,
                             object, active_camera));
    runtime.AddAmbientLight(object, 1.0f, 1.0f, 1.0f, 1.0f);
    runtime.UpdateProgress(0.2f);
    runtime.WriteWord(object + 440u, 0x3e99999au);
    for (const auto offset : {428u, 432u, 436u})
        runtime.WriteWord(object + offset, 0x3f800000u);

    if (Invoke(runtime, ArenaSceneResourceCall::LoadLightmapConfig, object) != 0u) {
        static_cast<void>(Invoke(runtime,
                                 ArenaSceneResourceCall::DestroySkinPipeline,
                                 runtime.ReadWord(object + 424u)));
        const auto pipeline = Invoke(runtime,
                                     ArenaSceneResourceCall::CreateSkinPipeline);
        runtime.WriteWord(object + 424u, pipeline);
        if (pipeline != 0u)
            static_cast<void>(Invoke(runtime,
                                     ArenaSceneResourceCall::ApplySkinPipeline,
                                     runtime.ReadWord(object + 16u), pipeline));
    }

    std::string fog_path(runtime.ReadText(object + 4u));
    fog_path += runtime.ReadText(object + 12u);
    fog_path += "_fog";
    const auto old_fog = runtime.ReadWord(object + 504u);
    if (old_fog != 0u) {
        static_cast<void>(Invoke(runtime,
                                 ArenaSceneResourceCall::DestroyFogDefaults,
                                 old_fog));
        static_cast<void>(Invoke(runtime,
                                 ArenaSceneResourceCall::FreeMemory, old_fog));
        runtime.WriteWord(object + 504u, 0u);
    }
    const auto fog_memory = Invoke(runtime,
                                   ArenaSceneResourceCall::AllocateFogDefaults,
                                   0x18u);
    const auto fog = fog_memory == 0u ? 0u :
        Invoke(runtime, ArenaSceneResourceCall::InitializeFogDefaults,
               fog_memory);
    runtime.WriteWord(object + 504u, fog);
    if (Invoke(runtime, ArenaSceneResourceCall::LoadFogConfig,
               fog, 0u, fog_path) == 0u) {
        if (fog != 0u) {
            static_cast<void>(Invoke(runtime,
                                     ArenaSceneResourceCall::DestroyFogDefaults,
                                     fog));
            static_cast<void>(Invoke(runtime,
                                     ArenaSceneResourceCall::FreeMemory, fog));
        }
        runtime.WriteWord(object + 504u, 0u);
    }

    RenderClumps clumps(runtime);
    BindGameArenaRenderClumps(object,clumps);

    std::string scene_path(scene);
    scene_path += '\\';
    scene_path.append(section);
    scene_path += '\\';
    scene_path.append(section);
    std::string intri_path(world_root);
    intri_path += scene_path;
    intri_path += "_intri.dff";
    static_cast<void>(Invoke(runtime, ArenaSceneResourceCall::LoadIntriClump,
                             Invoke(runtime,
                                    ArenaSceneResourceCall::GetMovementAdapter),
                             0u, intri_path));

    std::string sky_path(world_root);
    sky_path += runtime.ReadText(object + 8u);
    sky_path += '\\';
    sky_path += runtime.ReadText(object + 12u);
    sky_path += '\\';
    runtime.LoadSkyAssets(object + 320u, sky_path, "sky", "cloud",
                          "skyDual", "clouddual", 5u, 6u);
    std::string sequence_path(runtime.ReadText(object + 4u));
    sequence_path += "sequence.xml";
    static_cast<void>(Invoke(runtime, ArenaSceneResourceCall::LoadSequenceXml,
                             0u, 0u, sequence_path));
    static_cast<void>(Invoke(runtime,
                             ArenaSceneResourceCall::ApplySequenceEntries,
                             object + 448u, runtime.GameContext()));
    if (runtime.ReadWord(object + 316u) != 0u)
        static_cast<void>(Invoke(runtime, ArenaSceneResourceCall::CleanupScripts));
    runtime.UpdateProgress(0.30000001f);
    static_cast<void>(Invoke(runtime, ArenaSceneResourceCall::ProcessScriptStage));

    std::string movement_path(runtime.PathRoot(ArenaScenePathRoot::MovementPrefix));
    movement_path += scene_path;
    std::string floor_path(movement_path);
    floor_path += "_floor.dff";
    static_cast<void>(Invoke(runtime, ArenaSceneResourceCall::LoadFloorClump,
                             Invoke(runtime,
                                    ArenaSceneResourceCall::GetMovementAdapter),
                             0u, floor_path));
    runtime.UpdateProgress(0.34999999f);
    std::string wall_path(movement_path);
    wall_path += "_wall.dff";
    static_cast<void>(Invoke(runtime, ArenaSceneResourceCall::LoadWallClump,
                             Invoke(runtime,
                                    ArenaSceneResourceCall::GetMovementAdapter),
                             0u, wall_path));
    static_cast<void>(Invoke(runtime, ArenaSceneResourceCall::CleanupScripts));
    const auto effect_context = runtime.GameContext();
    const auto scene_effect = runtime.ReadWord(object + 44u);
    const auto effect_singleton = Invoke(
        runtime, ArenaSceneResourceCall::RegisterEffectSingleton);
    runtime.InitializeEffects(effect_singleton, scene_effect, effect_context);
    runtime.UpdateProgress(0.40000001f);
    return true;
}

}  // namespace pal4
