#include "pal4/game_arena_scene_resource_loader.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

using Op = pal4::ArenaSceneResourceCall;

struct OperationRecord {
    Op operation;
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
    std::string text;
    bool operator==(const OperationRecord&) const = default;
};

struct Runtime final : pal4::IGameArenaSceneResourceRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> texts;
    std::map<Op, std::uint32_t> results;
    std::vector<OperationRecord> calls;
    std::vector<float> progress;
    bool archive_mode = false;
    bool world_success = true;
    std::string world_root;
    std::string world_scene;
    std::string world_section;
    std::uint32_t world_context = 0u;
    std::string cache_world;
    std::string cache_name;
    std::string sky_path;
    std::uint32_t sky_config = 0u;
    std::uint32_t effect_singleton = 0u;
    std::uint32_t effect_scene = 0u;
    std::uint32_t effect_context = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::string_view ReadText(std::uint32_t address) override {
        return texts[address];
    }
    std::string_view PathRoot(pal4::ArenaScenePathRoot root) override {
        switch (root) {
        case pal4::ArenaScenePathRoot::WorldPrefix: return "WORLD/";
        case pal4::ArenaScenePathRoot::WorldSuffix: return "sub/";
        case pal4::ArenaScenePathRoot::MovementPrefix: return "MOVE/";
        }
        throw std::runtime_error("unexpected path root");
    }
    bool ArchiveModeEnabled() override { return archive_mode; }
    std::uint32_t GameContext() override { return 0x8888u; }
    void RemoveArenaCache(std::string_view world,
                          std::string_view cache) override {
        cache_world = world;
        cache_name = cache;
    }
    void UpdateProgress(float value) override { progress.push_back(value); }
    bool InitializeWorld(std::uint32_t object,
                         std::string_view root,
                         std::string_view scene,
                         std::string_view section,
                         std::uint32_t context) override {
        Check(object == 0x5000u, "world receives arena object");
        world_root = root;
        world_scene = scene;
        world_section = section;
        world_context = context;
        return world_success;
    }
    void AddAmbientLight(std::uint32_t object, float red, float green,
                         float blue, float alpha) override {
        Check(object == 0x5000u && red == 1.0f && green == 1.0f &&
                  blue == 1.0f && alpha == 1.0f,
              "ambient light uses four unit channels");
    }
    void LoadSkyAssets(std::uint32_t config, std::string_view path,
                       std::string_view sky, std::string_view cloud,
                       std::string_view sky_dual,
                       std::string_view cloud_dual,
                       std::uint32_t first_index,
                       std::uint32_t second_index) override {
        Check(sky == "sky" && cloud == "cloud" &&
                  sky_dual == "skyDual" && cloud_dual == "clouddual" &&
                  first_index == 5u && second_index == 6u,
              "sky assets and clump indices match IDA");
        sky_config = config;
        sky_path = path;
    }
    void InitializeEffects(std::uint32_t singleton,
                           std::uint32_t scene_effect,
                           std::uint32_t context) override {
        effect_singleton = singleton;
        effect_scene = scene_effect;
        effect_context = context;
    }
    std::uint32_t Call(Op operation, std::uint32_t first,
                       std::uint32_t second,
                       std::string_view text) override {
        calls.push_back({operation, first, second, std::string(text)});
        return results[operation];
    }

    std::size_t Position(Op operation) const {
        const auto it = std::find_if(calls.begin(), calls.end(),
                                     [operation](const OperationRecord& value) {
                                         return value.operation == operation;
                                     });
        Check(it != calls.end(), "expected scene-resource operation");
        return static_cast<std::size_t>(it - calls.begin());
    }
    std::vector<OperationRecord> Calls(Op operation) const {
        std::vector<OperationRecord> found;
        for (const auto& call : calls)
            if (call.operation == operation) found.push_back(call);
        return found;
    }
};

void SetDefaults(Runtime& runtime) {
    constexpr std::uint32_t object = 0x5000u;
    runtime.texts[object + 4u] = "SCENEDIR\\";
    runtime.texts[object + 8u] = "WORLDNAME";
    runtime.texts[object + 12u] = "ROOM";
    runtime.texts[object + 224u] = "OLD_CACHE";
    runtime.words[object + 16u] = 0x920u;
    runtime.words[object + 44u] = 0xb00u;
    runtime.words[object + 424u] = 0x910u;
    runtime.results[Op::InitializeTemperature] = 0x111u;
    runtime.results[Op::InitializeCamera] = 0x222u;
    runtime.results[Op::LoadArchive] = 0x333u;
    runtime.results[Op::GetActiveCamera] = 0x444u;
    runtime.results[Op::LoadLightmapConfig] = 1u;
    runtime.results[Op::CreateSkinPipeline] = 0x555u;
    runtime.results[Op::AllocateFogDefaults] = 0x666u;
    runtime.results[Op::InitializeFogDefaults] = 0x667u;
    runtime.results[Op::LoadFogConfig] = 1u;
    runtime.results[Op::GetMovementAdapter] = 0x777u;
    runtime.results[Op::RegisterEffectSingleton] = 0x999u;
}

void CompleteLoad() {
    Runtime runtime;
    SetDefaults(runtime);
    runtime.archive_mode = true;
    runtime.words[0x5000u + 48u] = 1u;
    runtime.words[0x5000u + 504u] = 0x900u;
    runtime.words[0x5000u + 52u] = 0xa00u;
    runtime.words[0x5000u + 72u] = 0xa20u;
    runtime.words[0x5000u + 92u] = 0xa40u;
    Check(pal4::GameArenaSceneResourceLoader::Load(
              0x5000u, "Q01", "Hall", 0u, runtime),
          "complete arena resource load succeeds");
    Check(runtime.progress == std::vector<float>{
              0.0f, 0.1f, 0.2f, 0.30000001f,
              0.34999999f, 0.40000001f} &&
              runtime.world_root == "WORLD/sub/" &&
              runtime.world_scene == "Q01" &&
              runtime.world_section == "Hall" &&
              runtime.world_context == 0x8888u &&
              runtime.cache_world == "WORLDNAME" &&
              runtime.cache_name == "OLD_CACHE",
          "world initialization, cache texts and progress follow IDA");
    Check(runtime.Calls(Op::LoadArchive) == std::vector<OperationRecord>{
              {Op::LoadArchive, 0u, 0u,
               "gamedata\\PALWorld\\Q01.cpk"}} &&
              runtime.words[0x5000u + 316u] == 0x333u &&
              runtime.Position(Op::ProcessArchive) <
                  runtime.Position(Op::GetActiveCamera),
          "optional CPK is processed before world resources");
    Check(runtime.words[0x5000u + 440u] == 0x3e99999au &&
              runtime.words[0x5000u + 428u] == 0x3f800000u &&
              runtime.words[0x5000u + 432u] == 0x3f800000u &&
              runtime.words[0x5000u + 436u] == 0x3f800000u &&
              runtime.words[0x5000u + 424u] == 0x555u &&
              runtime.Calls(Op::ApplySkinPipeline) == std::vector<OperationRecord>{
                  {Op::ApplySkinPipeline, 0x920u, 0x555u}},
          "light and skin-pipeline state matches resource load");
    Check(runtime.Calls(Op::DestroyFogDefaults) == std::vector<OperationRecord>{
              {Op::DestroyFogDefaults, 0x900u}} &&
              runtime.Calls(Op::LoadFogConfig) == std::vector<OperationRecord>{
                  {Op::LoadFogConfig, 0x667u, 0u, "SCENEDIR\\ROOM_fog"}} &&
              runtime.words[0x5000u + 504u] == 0x667u,
          "old fog is released and scene fog path loaded");
    Check(runtime.Calls(Op::AddClump).size() == 3u &&
              runtime.Calls(Op::ProcessClumpAtomics).size() == 3u &&
              runtime.Calls(Op::ProcessClump).size() == 2u &&
              runtime.Calls(Op::ProcessClumpAtomics)[0].second == 0x511cu,
          "all clumps register; only first two use extra atomic pass");
    Check(runtime.Calls(Op::LoadIntriClump) == std::vector<OperationRecord>{
              {Op::LoadIntriClump, 0x777u, 0u,
               "WORLD/sub/Q01\\Hall\\Hall_intri.dff"}} &&
              runtime.sky_config == 0x5140u &&
              runtime.sky_path == "WORLD/sub/WORLDNAME\\ROOM\\" &&
              runtime.Calls(Op::LoadSequenceXml) == std::vector<OperationRecord>{
                  {Op::LoadSequenceXml, 0u, 0u,
                   "SCENEDIR\\sequence.xml"}},
          "intri, sky and sequence paths match IDA concatenation");
    Check(runtime.Calls(Op::LoadFloorClump) == std::vector<OperationRecord>{
              {Op::LoadFloorClump, 0x777u, 0u,
               "MOVE/Q01\\Hall\\Hall_floor.dff"}} &&
              runtime.Calls(Op::LoadWallClump) == std::vector<OperationRecord>{
                  {Op::LoadWallClump, 0x777u, 0u,
                   "MOVE/Q01\\Hall\\Hall_wall.dff"}} &&
              runtime.effect_singleton == 0x999u &&
              runtime.effect_scene == 0xb00u &&
              runtime.effect_context == 0x8888u &&
              runtime.Calls(Op::CleanupScripts).size() == 2u,
          "movement and effects stages finish in order");
}

void WorldInitializationFailure() {
    Runtime runtime;
    SetDefaults(runtime);
    runtime.archive_mode = true;
    runtime.world_success = false;
    Check(!pal4::GameArenaSceneResourceLoader::Load(
              0x5000u, "Q01", "Hall", 1u, runtime) &&
              runtime.progress == std::vector<float>{0.0f, 0.1f} &&
              runtime.words[0x5000u + 316u] == 0u &&
              runtime.Calls(Op::CleanupScripts).size() == 1u &&
              runtime.Calls(Op::DestroyArchive) == std::vector<OperationRecord>{
                  {Op::DestroyArchive, 0x333u}} &&
              runtime.Calls(Op::LoadLightmapConfig).empty(),
          "world failure destroys a loaded archive and exits early");

    Runtime no_archive;
    SetDefaults(no_archive);
    no_archive.world_success = false;
    Check(!pal4::GameArenaSceneResourceLoader::Load(
              0x5000u, "Q01", "Hall", 1u, no_archive) &&
              no_archive.Calls(Op::LoadArchive).empty() &&
              no_archive.Calls(Op::CleanupScripts).empty() &&
              no_archive.Calls(Op::DestroyArchive).empty(),
          "world failure without archive has no script cleanup");
}

void OptionalResources() {
    Runtime runtime;
    SetDefaults(runtime);
    runtime.results[Op::LoadLightmapConfig] = 0u;
    runtime.results[Op::LoadFogConfig] = 0u;
    Check(pal4::GameArenaSceneResourceLoader::Load(
              0x5000u, "Q01", "Hall", 1u, runtime) &&
              runtime.Calls(Op::LoadArchive).empty() &&
              runtime.Calls(Op::InitializeArenaCache).empty() &&
              runtime.Calls(Op::CreateSkinPipeline).empty() &&
              runtime.Calls(Op::AddClump).empty() &&
              runtime.words[0x5000u + 504u] == 0u &&
              runtime.Calls(Op::DestroyFogDefaults) == std::vector<OperationRecord>{
                  {Op::DestroyFogDefaults, 0x667u}} &&
              runtime.Calls(Op::FreeMemory) == std::vector<OperationRecord>{
                  {Op::FreeMemory, 0x667u}} &&
              runtime.Calls(Op::CleanupScripts).size() == 1u,
          "optional resources skip and failed fog load cleans new object");
}

void NativeServiceStub() {
    pal4::GameArenaSceneResourceRuntimeStub runtime;
    Check(!pal4::GameArenaSceneResourceLoader::Load(
              0x5000u, "Q01", "Hall", 1u, runtime),
          "native-service stub links and exits before unavailable world setup");
}

}  // namespace

int main() {
    try {
        CompleteLoad();
        WorldInitializationFailure();
        OptionalResources();
        NativeServiceStub();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
