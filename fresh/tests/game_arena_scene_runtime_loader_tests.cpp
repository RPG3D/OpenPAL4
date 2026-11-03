#include "pal4/game_arena_scene_runtime_loader.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using Op = pal4::GameArenaRuntimeOperation;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct CallRecord {
    Op operation;
    std::uint32_t first;
    std::uint32_t second;
    std::string first_text;
    std::string second_text;
    bool operator==(const CallRecord&) const = default;
};

struct Runtime final : pal4::IGameArenaSceneRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> texts;
    std::vector<pal4::GameArenaSceneRecord> records;
    std::vector<CallRecord> calls;
    std::vector<std::string> order;
    std::vector<float> progress;
    std::uint32_t nav_clump = 0u;
    std::uint32_t has_player_data = 0u;
    int nav_reads = 0;
    std::string missing_message;
    std::string missing_title;
    std::string effect_root;
    std::string effect_world;
    std::string effect_scene;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        order.push_back("write");
    }
    std::string_view ReadText(std::uint32_t address) override {
        return texts[address];
    }
    void SetSceneName(std::uint32_t address, std::string_view text) override {
        texts[address] = text;
        order.push_back("scene");
    }
    std::uint32_t NavClump() override {
        ++nav_reads;
        return nav_clump;
    }
    void BuildCollisionTree(std::uint32_t world,
                            std::uint32_t clump) override {
        Check(world == 0x5000u && clump == nav_clump,
              "collision tree receives world and nav clump");
        order.push_back("collision");
    }
    void ShowMissingNavmesh(std::string_view message,
                            std::string_view title) override {
        missing_message = message;
        missing_title = title;
        order.push_back("nav-error");
    }
    std::string_view EffectRoot() override { return "effect-root"; }
    void LoadEffectRecords(std::string_view root, std::string_view world,
                           std::string_view scene) override {
        effect_root = root;
        effect_world = world;
        effect_scene = scene;
        order.push_back("effects");
    }
    std::span<const pal4::GameArenaSceneRecord> SceneRecords() override {
        return records;
    }
    void UpdateProgress(float value) override {
        progress.push_back(value);
        order.push_back("progress");
    }
    std::uint32_t Call(Op operation, std::uint32_t first,
                       std::uint32_t second, std::string_view first_text,
                       std::string_view second_text) override {
        calls.push_back({operation, first, second,
                         std::string(first_text), std::string(second_text)});
        order.push_back("call:" + std::to_string(static_cast<int>(operation)));
        return operation == Op::PlayerZeroHasData ? has_player_data : 0u;
    }
    std::vector<CallRecord> Calls(Op operation) const {
        std::vector<CallRecord> selected;
        for (const auto& call : calls)
            if (call.operation == operation) selected.push_back(call);
        return selected;
    }
    std::size_t Position(Op operation) const {
        const auto it = std::find_if(calls.begin(), calls.end(),
                                     [operation](const CallRecord& call) {
                                         return call.operation == operation;
                                     });
        return static_cast<std::size_t>(it - calls.begin());
    }
    std::size_t EventPosition(std::string_view event) const {
        const auto it = std::find(order.begin(), order.end(), event);
        return static_cast<std::size_t>(it - order.begin());
    }
};

void SetDefaults(Runtime& runtime) {
    runtime.texts[0x5008u] = "world";
    runtime.texts[0x500cu] = "default";
    runtime.texts[0x50f0u] = "WORLD";
}

void CheckProgressAndCallbacks(const Runtime& runtime) {
    Check(runtime.progress == std::vector<float>{
              0.5f, 0.55000001f, 0.60000002f, 0.64999998f,
              0.69999999f, 0.75f, 0.80000001f, 0.81999999f,
              0.83999997f, 0.86000001f, 0.88f, 0.89999998f,
              0.94999999f, 0.98000002f, 1.0f},
          "all fifteen loading progress updates retain IDA order");
    const auto callback = [&runtime](Op operation, std::uint32_t address) {
        Check(runtime.Calls(operation) == std::vector<CallRecord>{
                  {operation, 0x5000u, address, "", ""}},
              "callback receives arena and original function address");
    };
    callback(Op::ForEachPlayer,
             pal4::GameArenaSceneRuntimeLoader::kPlayerCallback);
    callback(Op::ForEachNpc,
             pal4::GameArenaSceneRuntimeLoader::kNpcCallback);
    callback(Op::ForEachAnimation,
             pal4::GameArenaSceneRuntimeLoader::kAnimationCallback);
    callback(Op::ForEachMonster,
             pal4::GameArenaSceneRuntimeLoader::kMonsterCallback);
    callback(Op::ForEachEffect,
             pal4::GameArenaSceneRuntimeLoader::kEffectCallback);
    Check(runtime.Position(Op::ProcessScriptStage) <
              runtime.Position(Op::ResetPlayerData) &&
              runtime.Position(Op::ResetPlayerData) <
              runtime.Position(Op::InitializeNpc) &&
              runtime.Position(Op::InitializeNpc) <
              runtime.Position(Op::LoadGameObjects) &&
              runtime.Position(Op::LoadGameObjects) <
              runtime.Position(Op::InitializeMonster) &&
              runtime.Position(Op::LoadPathway) <
              runtime.Position(Op::CleanupScripts) &&
              runtime.Position(Op::CleanupScripts) <
              runtime.Position(Op::DispatchUiEvent) &&
              runtime.Position(Op::DispatchUiEvent) <
              runtime.Position(Op::UpdateArenaCache) &&
              runtime.Position(Op::ForEachPlayer) <
              runtime.Position(Op::UpdatePlayerData) &&
              runtime.Position(Op::UpdatePlayerData) <
              runtime.Position(Op::ForEachNpc) &&
              runtime.Position(Op::ForEachNpc) <
              runtime.Position(Op::ForEachAnimation) &&
              runtime.Position(Op::ForEachAnimation) <
              runtime.Position(Op::ForEachMonster) &&
              runtime.Position(Op::ForEachMonster) <
              runtime.Position(Op::ForEachEffect) &&
              runtime.Position(Op::ExecuteSceneScript) <
              runtime.Position(Op::SetBackgroundMusic),
          "subsystems and five callbacks follow IDA sequence");
    Check(runtime.Calls(Op::DispatchUiEvent) == std::vector<CallRecord>{
              {Op::DispatchUiEvent,
               pal4::GameArenaSceneRuntimeLoader::kUiEvent, 0u, "", ""}},
          "UI event address is preserved");
    Check(runtime.order.front() == "scene" &&
              runtime.EventPosition("effects") <
                  runtime.EventPosition("call:" +
                      std::to_string(static_cast<int>(Op::LoadPathway))) &&
              runtime.order.back() ==
                  "call:" +
                      std::to_string(static_cast<int>(Op::SetBackgroundMusic)) &&
              runtime.order[runtime.order.size() - 2u] == "progress",
          "scene selection, effects and final progress surround runtime calls");
}

void ExplicitSceneAndFirstRecord() {
    Runtime runtime;
    SetDefaults(runtime);
    runtime.nav_clump = 0xabcdu;
    runtime.has_player_data = 1u;
    runtime.words[0x5084u] = 1u;
    runtime.words[0x50a0u] = std::bit_cast<std::uint32_t>(750.0f);
    runtime.texts[0x50f0u] = "elsewhere";
    runtime.records = {
        {0x111u, "WORLD", "OTHER", 1u},
        {0x222u, "WORLD", "HALL", 1u},
        {0x333u, "WORLD", "HALL", 2u},
    };
    Check(pal4::GameArenaSceneRuntimeLoader::Load(0x5000u, "Hall", runtime),
          "scene runtime completes");
    Check(runtime.texts[0x50e0u] == "Hall" && runtime.nav_reads == 2 &&
              runtime.missing_message.empty() &&
              runtime.words[0x5108u] == 0x222u,
          "explicit scene, nav tree and first matching record are selected");
    Check(runtime.EventPosition("collision") <
              runtime.EventPosition("progress"),
          "collision tree precedes loading progress");
    Check(runtime.Calls(Op::SetActorWord28) == std::vector<CallRecord>{
              {Op::SetActorWord28, 0u, 0u, "", ""}} &&
              runtime.Calls(Op::PlayerZeroHasData).size() == 1u &&
              runtime.Calls(Op::UpdatePlayerStates) == std::vector<CallRecord>{
                  {Op::UpdatePlayerStates, 0x5000u, 0u, "", ""}},
          "type one clears actor flag and updates available player data");
    Check(runtime.Calls(Op::SetCameraFarClip) == std::vector<CallRecord>{
              {Op::SetCameraFarClip,
               std::bit_cast<std::uint32_t>(1500.0f), 0u, "", ""}} &&
              runtime.Calls(Op::ExecuteSceneScript) ==
                  std::vector<CallRecord>{{Op::ExecuteSceneScript,
                                           0x5000u, 1u, "", ""}},
          "camera distance doubles and changed world name sets script flag");
    Check(runtime.Calls(Op::SetBackgroundMusic) ==
              std::vector<CallRecord>{{Op::SetBackgroundMusic,
                                       0u, 0u, "world", "Hall"}} &&
              runtime.effect_root == "effect-root" &&
              runtime.effect_world == "world" &&
              runtime.effect_scene == "Hall",
          "effect and BGM receive world then scene text");
    CheckProgressAndCallbacks(runtime);
}

void DefaultSceneAndMissingNavmesh() {
    Runtime runtime;
    SetDefaults(runtime);
    runtime.records = {{0x444u, "WORLD", "DEFAULT", 2u}};
    Check(pal4::GameArenaSceneRuntimeLoader::Load(0x5000u, "", runtime),
          "default scene completes without navmesh");
    Check(runtime.texts[0x50e0u] == "default" &&
              runtime.nav_reads == 1 &&
              runtime.missing_message == "Not Exist navMesh" &&
              runtime.missing_title == "navMesh error" &&
              runtime.words[0x5108u] == 0x444u,
          "empty scene falls back and missing navmesh is reported");
    Check(runtime.EventPosition("nav-error") <
              runtime.EventPosition("progress"),
          "missing navmesh warning precedes loading progress");
    Check(runtime.Calls(Op::SetActorWord28) == std::vector<CallRecord>{
              {Op::SetActorWord28, 1u, 0u, "", ""}} &&
              runtime.Calls(Op::PlayerZeroHasData).empty() &&
              runtime.Calls(Op::UpdatePlayerStates).empty() &&
              runtime.Calls(Op::SetCameraFarClip).empty() &&
              runtime.Calls(Op::ExecuteSceneScript) ==
                  std::vector<CallRecord>{{Op::ExecuteSceneScript,
                                           0x5000u, 0u, "", ""}},
          "type two skips player update and equal world name clears script flag");
    CheckProgressAndCallbacks(runtime);
}

void NoRecordAndOtherActorType() {
    Runtime missing;
    SetDefaults(missing);
    missing.words[0x5108u] = 0x999u;
    Check(pal4::GameArenaSceneRuntimeLoader::Load(0x5000u, "Hall", missing) &&
              missing.words[0x5108u] == 0u &&
              missing.Calls(Op::SetActorWord28).empty() &&
              missing.Calls(Op::PlayerZeroHasData).empty(),
          "missing record clears stale actor without actor-side operations");

    Runtime other;
    SetDefaults(other);
    other.records = {{0x555u, "WORLD", "HALL", 7u}};
    Check(pal4::GameArenaSceneRuntimeLoader::Load(0x5000u, "Hall", other) &&
              other.Calls(Op::SetActorWord28) ==
                  std::vector<CallRecord>{{Op::SetActorWord28,
                                           1u, 0u, "", ""}} &&
              other.Calls(Op::PlayerZeroHasData).size() == 1u &&
              other.Calls(Op::UpdatePlayerStates).empty(),
          "other actor type sets flag but empty player data gates update");
}

void CStringInputsAndStub() {
    Runtime runtime;
    SetDefaults(runtime);
    runtime.texts[0x5008u] = std::string("world\0ignored", 13);
    runtime.records = {{0x666u, "WORLD", "HALL", 1u}};
    Check(pal4::GameArenaSceneRuntimeLoader::Load(
              0x5000u, std::string_view("Hall\0ignored", 12), runtime) &&
              runtime.texts[0x50e0u] == "Hall" &&
              runtime.words[0x5108u] == 0x666u &&
              runtime.Calls(Op::SetBackgroundMusic).front().first_text ==
                  "world",
          "legacy C strings stop at first NUL");
    pal4::GameArenaSceneRuntimeStub stub;
    Check(pal4::GameArenaSceneRuntimeLoader::Load(0x5000u, "", stub),
          "native service stub remains linkable and completes no-op load");
}

}  // namespace

int main() {
    try {
        ExplicitSceneAndFirstRecord();
        DefaultSceneAndMissingNavmesh();
        NoRecordAndOtherActorType();
        CStringInputsAndStub();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
