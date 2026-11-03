#include "pal4/world_manager_scene_initializer.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kWorld = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IWorldManagerSceneInitializationRuntime,
                       pal4::IMatFxWorldSectorPipelineRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::string> strings;
    std::vector<std::string> calls;
    std::string opened_path;
    std::string package_path;
    std::uint32_t file = 0u;
    std::uint32_t script = 0u;
    std::uint32_t pipeline = 0x9000u;

    Runtime() {
        words[kWorld + 4u] = 0x6000u;
        words[kWorld + 12u] = 0x6100u;
        words[kWorld + 16u] = 0x6200u;
        words[0x8d5c78u] = 0x8000u;
        strings[0x6000u] = "assets/";
        strings[0x6100u] = "town";
    }
    std::uint32_t GameStateAddress() override { return 0x5000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::string_view ReadCString(std::uint32_t address) override {
        return strings[address];
    }
    std::uint32_t LoadPackage(std::string_view path,
                              std::uint32_t callback) override {
        Check(callback ==
                  pal4::WorldManagerSceneInitializer::kPackageCallback,
              "package loader callback");
        package_path = path;
        calls.emplace_back("package");
        return script;
    }
    void ProcessScript(std::uint32_t value) override {
        Check(value == script, "processed script");
        calls.emplace_back("process-script");
    }
    void CleanupAllScripts() override {
        calls.emplace_back("cleanup-scripts");
    }
    void DestroyScript(std::uint32_t value) override {
        Check(value == script, "destroyed script");
        calls.emplace_back("destroy-script");
    }
    std::string ConcatenateWorldBase(std::uint32_t first,
                                     std::uint32_t second) override {
        Check(first == 0x94d560u && second == 0x94d554u,
              "world base source addresses");
        calls.emplace_back("base");
        return "base/path";
    }
    std::uint32_t FinishWorldBase(std::string_view value) override {
        Check(value == "base/path", "finished world base");
        calls.emplace_back("finish-base");
        return 0x55u;
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game-instance");
        return 0x7000u;
    }
    void InitializeWorld(std::uint32_t world,
                         std::string_view base,
                         std::string_view region,
                         std::string_view scene,
                         std::uint32_t context) override {
        Check(world == kWorld && base == "base/path" &&
                  region == "region" && scene == "scene" &&
                  context == 0x7000u + 0x1acu,
              "GameWorld initialization arguments");
        calls.emplace_back("world-init");
    }
    void AddLight(std::uint32_t world,
                  std::array<float, 4> color) override {
        Check(world == kWorld &&
                  color == (std::array<float, 4>{1.0f, 1.0f,
                                                 1.0f, 1.0f}),
              "initial light is white RGBA");
        calls.emplace_back("light");
    }
    std::uint32_t OpenFile(std::string_view path,
                           std::string_view mode) override {
        Check(mode == "rb", "lightmap config mode");
        opened_path = path;
        calls.emplace_back("open");
        return file;
    }
    void ReadFile(std::uint32_t destination, std::uint32_t bytes,
                  std::uint32_t count, std::uint32_t opened) override {
        Check(opened == file && count == 1u, "config file read handle");
        if (destination == 0x8f94b8u)
            Check(bytes == 12u, "saturation read size");
        else
            Check(destination == 0x8f94b4u && bytes == 4u,
                  "brightness read size");
        calls.emplace_back(destination == 0x8f94b8u ?
                               "read-saturation" : "read-brightness");
    }
    void CloseFile(std::uint32_t opened) override {
        Check(opened == file, "config file close");
        calls.emplace_back("close");
    }
    pal4::IMatFxWorldSectorPipelineRuntime& PipelineRuntime() override {
        return *this;
    }
    void BindWorldSectors(std::uint32_t world,
                          std::uint32_t callback,
                          std::uint32_t value) override {
        Check(world == 0x6200u &&
                  callback ==
                      pal4::WorldManagerSceneInitializer::kSectorCallback &&
                  value == pipeline, "world sector pipeline binding");
        calls.emplace_back("bind-sectors");
    }
    void AddClump(std::uint32_t world,
                  std::uint32_t clump) override {
        Check(world == kWorld, "clump world");
        calls.emplace_back("add:" + std::to_string(clump));
    }
    void ProcessPluginAtomics(std::uint32_t clump,
                              std::uint32_t options) override {
        Check(options == kWorld + 196u, "plugin options subobject");
        calls.emplace_back("plugin:" + std::to_string(clump));
    }
    void ProcessClumpAtomics(std::uint32_t clump) override {
        calls.emplace_back("atomics:" + std::to_string(clump));
    }
    std::uint32_t CreatePipeline() override {
        calls.emplace_back("create-pipeline");
        return pipeline;
    }
    std::uint32_t NodeDefinitionAddress() override {
        calls.emplace_back("node-definition");
        return 0x8d5c78u;
    }
    void SetMipmapping(std::uint32_t enabled) override {
        Check(enabled == 0u, "mipmapping off");
        calls.emplace_back("mipmapping");
    }
    std::uint32_t LockPipeline(std::uint32_t value) override {
        Check(value == pipeline, "pipeline lock");
        calls.emplace_back("lock-pipeline");
        return 0x9100u;
    }
    std::uint32_t AddFragment(std::uint32_t locked,
                              std::uint32_t first,
                              std::uint32_t definition,
                              std::uint32_t last) override {
        Check(locked == 0x9100u && first == 0u &&
                  definition == 0x8d5c78u && last == 0u,
              "pipeline fragment arguments");
        calls.emplace_back("fragment");
        return 1u;
    }
    std::uint32_t UnlockPipeline(std::uint32_t locked) override {
        Check(locked == 0x9100u, "pipeline unlock");
        calls.emplace_back("unlock-pipeline");
        return 1u;
    }
    std::uint32_t FindNode(std::uint32_t value,
                           std::uint32_t name,
                           std::uint32_t first,
                           std::uint32_t last) override {
        Check(value == pipeline && name == 0x8000u &&
                  first == 0u && last == 0u,
              "pipeline node lookup");
        calls.emplace_back("find-node");
        return 0x9300u;
    }
    void SetInstanceCallback(std::uint32_t node,
                             std::uint32_t callback) override {
        Check(node == 0x9300u &&
                  callback == pal4::MatFxWorldSectorPipeline::kInstanceCallback,
              "pipeline instance callback");
        calls.emplace_back("instance-callback");
    }
    void SetRenderCallback(std::uint32_t node,
                             std::uint32_t callback) override {
        Check(node == 0x9300u &&
                  callback == pal4::MatFxWorldSectorPipeline::kRenderCallback,
              "pipeline lighting callback");
        calls.emplace_back("lighting-callback");
    }
    std::uint32_t DestroyPipeline(std::uint32_t value) override {
        calls.emplace_back("destroy-pipeline:" + std::to_string(value));
        return 0x44u;
    }
};

std::size_t Position(const Runtime& runtime, std::string_view name) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 name);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void ConfigAndPipelineEntries() {
    Runtime runtime;
    Check(!pal4::WorldManagerSceneInitializer::LoadLightmapConfig(
              kWorld, runtime) &&
              runtime.opened_path == "assets/town_ltMap.cfg" &&
              runtime.calls == std::vector<std::string>{"open"},
          "missing lightmap config stops before reads");
    runtime.calls.clear();
    runtime.file = 0x88u;
    Check(pal4::WorldManagerSceneInitializer::LoadLightmapConfig(
              kWorld, runtime) &&
              runtime.calls ==
                  (std::vector<std::string>{"open", "read-saturation",
                                            "read-brightness", "close"}),
          "config reads twelve and four bytes before close");
    runtime.calls.clear();
    Check(pal4::WorldManagerSceneInitializer::ClearPipeline(
              kWorld, runtime) == 0u && runtime.calls.empty(),
          "empty pipeline cleanup returns original zero");
    runtime.words[kWorld + 228u] = 0x7777u;
    Check(pal4::WorldManagerSceneInitializer::ClearPipeline(
              kWorld, runtime) == 0x44u &&
              runtime.words[kWorld + 228u] == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{
                      "destroy-pipeline:30583", "write:4324"}),
          "nonempty cleanup forwards destroy result and clears pointer");
}

void InitializeWithoutOptionalResources() {
    Runtime runtime;
    Check(pal4::WorldManagerSceneInitializer::Initialize(
              kWorld, "region", "scene", runtime) == 0x55u &&
              runtime.package_path.empty() &&
              runtime.opened_path == "assets/town_ltMap.cfg" &&
              runtime.words[0x8f94b4u] == 0x3e99999au &&
              runtime.words[0x8f94b8u] == 0x3f800000u &&
              runtime.words[0x8f94bcu] == 0x3f800000u &&
              runtime.words[0x8f94c0u] == 0x3f800000u,
          "base world and MatFx defaults initialize without package/config");
    Check(Position(runtime, "base") <
                  Position(runtime, "game-instance") &&
              Position(runtime, "game-instance") <
                  Position(runtime, "world-init") &&
              Position(runtime, "world-init") < Position(runtime, "light") &&
              Position(runtime, "open") <
                  Position(runtime, "finish-base") &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "create-pipeline") == runtime.calls.end(),
          "initialization order and missing-config pipeline gate");
}

void InitializeWithPackagePipelineAndClumps() {
    Runtime runtime;
    runtime.words[0x5000u + 1420u] = 1u;
    runtime.words[kWorld + 228u] = 0xaaaa;
    runtime.words[kWorld + 52u] = 0xc1u;
    runtime.words[kWorld + 72u] = 0xc2u;
    runtime.words[kWorld + 92u] = 0xc3u;
    runtime.script = 0x55u;
    runtime.file = 0x88u;
    Check(pal4::WorldManagerSceneInitializer::Initialize(
              kWorld, "region", "scene", runtime) == 0x55u &&
              runtime.package_path ==
                  "gamedata\\PALWorld\\region\\scene.cpk" &&
              runtime.words[kWorld + 228u] == runtime.pipeline,
          "package path and replacement pipeline");
    Check(Position(runtime, "destroy-pipeline:43690") <
                  Position(runtime, "package") &&
              Position(runtime, "package") <
                  Position(runtime, "process-script") &&
              Position(runtime, "read-brightness") <
                  Position(runtime, "destroy-pipeline:0") &&
              Position(runtime, "destroy-pipeline:0") <
                  Position(runtime, "create-pipeline") &&
              Position(runtime, "lighting-callback") <
                  Position(runtime, "bind-sectors"),
          "old pipeline release, package and new pipeline order");
    Check(Position(runtime, "add:193") <
                  Position(runtime, "plugin:193") &&
              Position(runtime, "plugin:193") <
                  Position(runtime, "atomics:193") &&
              Position(runtime, "atomics:194") <
                  Position(runtime, "add:195") &&
              Position(runtime, "plugin:195") <
                  Position(runtime, "cleanup-scripts") &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "atomics:195") == runtime.calls.end() &&
              Position(runtime, "cleanup-scripts") <
                  Position(runtime, "destroy-script") &&
              Position(runtime, "destroy-script") <
                  Position(runtime, "finish-base"),
          "three clump branches and package cleanup order");
}

void FailedNewPipeline() {
    Runtime runtime;
    runtime.file = 0x88u;
    runtime.pipeline = 0u;
    Check(pal4::WorldManagerSceneInitializer::Initialize(
              kWorld, "region", "scene", runtime) == 0x55u &&
              runtime.words[kWorld + 228u] == 0u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "bind-sectors") == runtime.calls.end(),
          "failed pipeline creation skips world-sector binding");
}

}  // namespace

int main() {
    ConfigAndPipelineEntries();
    InitializeWithoutOptionalResources();
    InitializeWithPackagePipelineAndClumps();
    FailedNewPipeline();
}
