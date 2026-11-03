#include "pal4/game_arena_release.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using Op = pal4::GameArenaReleaseCall;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Invocation {
    Op operation;
    std::uint32_t first;
    std::uint32_t second;
    std::uint32_t third;
};

struct Runtime final : pal4::IGameArenaReleaseRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Invocation> calls;
    std::vector<std::uint32_t> reads;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::pair<std::uint32_t, std::string>> stored_texts;
    std::vector<std::uint32_t> text_reads;
    bool fail_pipeline = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.emplace_back(address, value);
        words[address] = value;
    }
    std::string_view ReadCString(std::uint32_t address) override {
        text_reads.push_back(address);
        if (address == 0x5008u)
            return std::string_view{"world\0tail", 10};
        Check(address == 0x500cu, "area text field");
        return "hall";
    }
    void StoreText(std::uint32_t address,
                   std::string_view value) override {
        stored_texts.emplace_back(address, value);
    }
    std::uint32_t Call(Op operation, std::uint32_t first,
                       std::uint32_t second,
                       std::uint32_t third) override {
        calls.push_back({operation, first, second, third});
        if (operation == Op::DestroyPipeline) {
            Check(words[0x5108u] == 0u && words[0x51a8u] == first,
                  "Scene record clears before pipeline destruction; pipeline owner remains live");
            if (fail_pipeline) throw std::runtime_error("pipeline release failure");
        }
        if (operation == Op::ClearArenaSkillObjects) {
            Check(words[0x51f8u] == 0u,
                  "fog ownership retires before skill cleanup");
            for (const auto offset : {616u, 632u, 648u, 664u, 680u})
                Check(words[0x5000u + offset + 8u] ==
                          words[0x5000u + offset + 4u],
                      "all five borrowed ranges retire before skill objects");
        }
        if (operation == Op::ResetDefaults) words[0x51f8u] = 0x8010u;
        if (operation == Op::RemoveClump) {
            if (second == 0x8100u) words[0x5034u] = 0x8110u;
            if (second == 0x8200u) words[0x5048u] = 0x8210u;
        }
        if (operation == Op::CleanupAuxObjectFirst)
            words[0x5194u] = 0xc010u;
        if (operation == Op::GetMovementAdapter) return 0xa000u;
        return 0x9000u + static_cast<std::uint32_t>(operation);
    }
};

std::vector<Op> Operations(const Runtime& runtime) {
    std::vector<Op> result;
    for (const auto& call : runtime.calls) result.push_back(call.operation);
    return result;
}

void EarlyGate() {
    Runtime runtime;
    runtime.words[0x513cu] = 0x7000u;
    Check(!pal4::GameArenaRelease::Release(0x5000u, runtime),
          "missing world exits after script cleanup");
    Check(Operations(runtime) == std::vector<Op>{Op::DestroyScript} &&
              runtime.calls[0].first == 0x7000u &&
              runtime.reads == std::vector<std::uint32_t>{0x513cu, 0x5030u} &&
              runtime.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x513cu, 0u}}),
          "script is destroyed and cleared before world gate");

    runtime = Runtime{};
    Check(!pal4::GameArenaRelease::Release(0x5000u, runtime) &&
              runtime.calls.empty() && runtime.writes.empty(),
          "null script and world have no side effects");
}

Runtime FullRuntime() {
    Runtime runtime;
    runtime.words = {{0x513cu, 0x7000u}, {0x5030u, 1u},
                     {0x51f8u, 0x8000u}, {0x5034u, 0x8100u},
                     {0x5048u, 0x8200u}, {0x505cu, 0x8300u},
                     {0xa000u, 0xb000u}, {0xa004u, 0xb004u},
                     {0x5194u, 0xc000u}, {0x51a8u, 0xd000u},
                     {0x5108u, 0xe000u}};
    for (const auto offset : {616u, 632u, 648u, 664u, 680u}) {
        runtime.words[0x5000u + offset + 4u] = 0x1000u + offset;
        runtime.words[0x5000u + offset + 8u] = 0x2000u + offset;
    }
    return runtime;
}

void FullPath() {
    auto runtime = FullRuntime();
    Check(pal4::GameArenaRelease::Release(0x5000u, runtime),
          "active world releases successfully");
    const std::vector<Op> expected{
        Op::DestroyScript, Op::ResetDefaults, Op::DestroyDefaults,
        Op::FreeMemory, Op::UpdateWeatherEffects,
        Op::GetEffectSingleton, Op::ShutdownEffectSingleton,
        Op::GetEffectSystem, Op::CleanupEffects,
        Op::GetArenaSkillManager, Op::ClearArenaSkillObjects,
        Op::GetScriptExecutor, Op::CleanupLogHandlers,
        Op::GetScriptExecutor, Op::ResetScriptExecutor,
        Op::GetScriptCloudManager, Op::CleanupCameraTree,
        Op::GetCameraSubsystem, Op::CleanupCameraProperties,
        Op::RemoveClump, Op::ResetClumpMaterials,
        Op::RemoveClump, Op::ResetClumpMaterials, Op::RemoveClump,
        Op::CleanupGeometry, Op::CleanupGeometry, Op::CleanupGeometry,
        Op::GetMovementAdapter, Op::CleanupMovementComponent,
        Op::GetMovementAdapter, Op::CleanupMovementComponent,
        Op::CleanupCache, Op::CleanupConfig, Op::ResetObjectList,
        Op::CleanupAuxObjectFirst, Op::CleanupAuxObjectSecond, Op::FreeMemory,
        Op::GetActorControl, Op::ClearActor,
        Op::GetPlayerManager, Op::ForAllPlayers,
        Op::GetPlayerManager, Op::UpdatePlayers, Op::RemovePetScript,
        Op::GetPetManager, Op::CleanupPets,
        Op::GetNpcManager, Op::ForAllNpcs,
        Op::GetNpcManager, Op::CleanupNpcs,
        Op::GetMonsterManager, Op::ForAllMonsters,
        Op::GetMonsterManager, Op::CleanupMonsters,
        Op::GetObjectManager, Op::ForAllObjects,
        Op::GetObjectManager, Op::CleanupObjects,
        Op::GetSecondaryObjectManager, Op::ForAllSecondaryObjects,
        Op::GetSecondaryObjectManager, Op::CleanupSecondaryObjects,
        Op::GetPathPlanner, Op::CleanupPaths,
        Op::GetGameController, Op::CleanupWorld, Op::DestroyPipeline};
    Check(Operations(runtime) == expected,
          "complete release service order follows IDA");
    Check(runtime.stored_texts ==
              (std::vector<std::pair<std::uint32_t, std::string>>{
                  {0x50f0u, "world"}, {0x50fcu, "hall"}}) &&
              runtime.text_reads == std::vector<std::uint32_t>{
                  0x5008u, 0x500cu},
          "world and area names copied to arena text fields");

    unsigned movement_index = 0u;
    unsigned material_index = 0u;
    std::vector<std::uint32_t> freed;
    for (const auto& call : runtime.calls) {
        if (call.operation == Op::ClearArenaSkillObjects)
            Check(call.first ==
                      0x9000u + static_cast<std::uint32_t>(
                                    Op::GetArenaSkillManager),
                  "skill records use the skill singleton, not the arena image");
        if (call.operation == Op::CleanupWorld)
            Check(call.first == 0x5000u &&
                      call.second ==
                          0x9000u + static_cast<std::uint32_t>(
                                        Op::GetGameController) + 0x1acu,
                  "world cleanup gets PALGameIV object base +0x1ac (IDA 51B573)");
        if (call.operation == Op::DestroyDefaults)
            Check(call.first == 0x8010u,
                  "default-object cleanup uses reread pointer");
        if (call.operation == Op::FreeMemory) freed.push_back(call.first);
        if (call.operation == Op::ResetClumpMaterials)
            Check(call.first == (material_index++ == 0u ? 0x8110u
                                                        : 0x8210u),
                  "material cleanup rereads clump after removal");
        if (call.operation == Op::CleanupMovementComponent)
            Check(call.first == (movement_index++ == 0u ? 0xb000u
                                                        : 0xb004u),
                  "each movement adapter component is read separately");
        if (call.operation == Op::CleanupAuxObjectSecond)
            Check(call.first == 0xc010u,
                  "auxiliary object reread after first cleanup");
    }
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> expected_writes{
        {0x513cu, 0u}, {0x51f8u, 0u},
        {0x5270u, 0x1268u}, {0x5280u, 0x1278u},
        {0x5290u, 0x1288u}, {0x52a0u, 0x1298u},
        {0x52b0u, 0x12a8u},
        {0x5194u, 0u}, {0x5108u, 0u}, {0x51a8u, 0u}};
    Check(movement_index == 2u &&
              material_index == 2u &&
              freed == std::vector<std::uint32_t>{0x8010u, 0xc010u} &&
              runtime.writes == expected_writes,
          "five record ranges collapse to begin and pointers clear");
}

void OptionalResources() {
    Runtime runtime;
    runtime.words[0x5030u] = 1u;
    Check(pal4::GameArenaRelease::Release(0x5000u, runtime),
          "world without optional resources still releases");
    const auto operations = Operations(runtime);
    for (const auto operation : {
             Op::DestroyScript, Op::ResetDefaults, Op::DestroyDefaults,
             Op::RemoveClump, Op::ResetClumpMaterials,
             Op::CleanupAuxObjectFirst, Op::CleanupAuxObjectSecond,
             Op::DestroyPipeline}) {
        Check(std::find(operations.begin(), operations.end(), operation) ==
                  operations.end(),
              "null optional resource skips its cleanup");
    }
    Check(runtime.writes ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x5270u, 0u}, {0x5280u, 0u}, {0x5290u, 0u},
                  {0x52a0u, 0u}, {0x52b0u, 0u}, {0x5108u, 0u}}),
          "world slot +264 clears even without an optional pipeline");

    pal4::GameArenaReleaseRuntimeStub stub;
    Check(!pal4::GameArenaRelease::Release(0x5000u, stub),
          "native runtime stub links and exits before services");
}

void WorldTailFailure() {
    Runtime runtime;
    runtime.words[0x5108u] = 0xe000u;
    runtime.words[0x51a8u] = 0xd000u;
    runtime.fail_pipeline = true;
    bool failed = false;
    try {
        pal4::GameArenaRelease::ReleaseWorldAndPipeline(0x5000u, runtime);
    } catch (const std::runtime_error& error) {
        failed = std::string_view(error.what()) == "pipeline release failure";
    }
    Check(failed && runtime.words[0x5108u] == 0u &&
              runtime.words[0x51a8u] == 0xd000u &&
              Operations(runtime) == std::vector<Op>{
                  Op::GetGameController, Op::CleanupWorld, Op::DestroyPipeline},
          "world tail propagates destruction failure without losing pipeline ownership");
    runtime.fail_pipeline = false;
    runtime.calls.clear();
    pal4::GameArenaRelease::ReleaseWorldAndPipeline(0x5000u, runtime);
    Check(runtime.words[0x51a8u] == 0u,
          "successful world tail clears optional pipeline ownership");
}

}  // namespace

int main() {
    try {
        EarlyGate();
        FullPath();
        OptionalResources();
        WorldTailFailure();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
