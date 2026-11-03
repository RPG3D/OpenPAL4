#include "pal4/world_manager_world_loader.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kManager = 0x1000u;
constexpr std::uint32_t kWorld = kManager + 116u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IWorldManagerWorldLoadRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::string> texts;
    std::vector<std::string> calls;
    std::uint32_t camera_calls = 0u;
    std::uint32_t scene_loads = 0u;

    void TouchSkillSystem() override { calls.emplace_back("skill"); }
    void CompactAtomicList(std::uint32_t list) override {
        calls.emplace_back("compact:" + std::to_string(list - kManager));
    }
    std::string_view StoredText(std::uint32_t descriptor) override {
        calls.emplace_back("read-text:" +
                           std::to_string(descriptor - kManager));
        return texts[descriptor];
    }
    void AssignText(std::uint32_t descriptor,
                    std::string_view text) override {
        calls.emplace_back("assign:" +
                           std::to_string(descriptor - kManager));
        texts[descriptor] = text;
    }
    std::uint32_t CurrentCameraObject() override {
        calls.emplace_back("camera:" + std::to_string(camera_calls));
        return 0xa000u + 0x100u * camera_calls++;
    }
    std::uint32_t SetWorldCamera(std::uint32_t world,
                                  std::uint32_t camera) override {
        Check(world == kWorld && camera == 0xcafeu,
              "second camera object's first word attached to world");
        calls.emplace_back("set-camera");
        return 7u;
    }
    std::uint32_t GameStateAddress() override { return 0x5000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-word:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write-word:" + std::to_string(address));
    }
    std::string_view ReadCString(std::uint32_t) override { return {}; }
    std::uint32_t LoadPackage(std::string_view, std::uint32_t) override {
        throw std::runtime_error("package load not expected");
    }
    void ProcessScript(std::uint32_t) override {}
    void CleanupAllScripts() override {}
    void DestroyScript(std::uint32_t) override {}
    std::string ConcatenateWorldBase(std::uint32_t, std::uint32_t) override {
        return "world-base";
    }
    std::uint32_t FinishWorldBase(std::string_view) override { return 0u; }
    std::uint32_t GameInstance() override { return 0x8000u; }
    void InitializeWorld(std::uint32_t world,
                         std::string_view base, std::string_view region,
                         std::string_view scene,
                         std::uint32_t context) override {
        Check(world == kWorld && base == "world-base" &&
                  region == "Qionghua" && scene == "Hall" &&
                  context == 0x8000u + 0x1acu,
              "scene initializer receives both names and world member");
        calls.emplace_back("scene-load");
        ++scene_loads;
    }
    void AddLight(std::uint32_t,
                  std::array<float, 4>) override {}
    std::uint32_t OpenFile(std::string_view, std::string_view) override {
        return 0u;
    }
    void ReadFile(std::uint32_t, std::uint32_t,
                  std::uint32_t, std::uint32_t) override {}
    void CloseFile(std::uint32_t) override {}
    pal4::IMatFxWorldSectorPipelineRuntime& PipelineRuntime() override {
        throw std::runtime_error("pipeline runtime not expected");
    }
    void BindWorldSectors(std::uint32_t, std::uint32_t,
                           std::uint32_t) override {}
    void AddClump(std::uint32_t, std::uint32_t) override {}
    void ProcessPluginAtomics(std::uint32_t,
                              std::uint32_t) override {}
    void ProcessClumpAtomics(std::uint32_t) override {}
};

std::size_t Position(const Runtime& runtime, std::string_view call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void UnchangedWorldSkipsSceneReload() {
    Runtime runtime;
    runtime.texts[kManager + 348u] = "Qionghua";
    runtime.texts[kManager + 364u] = "Hall";
    runtime.words[0xa100u] = 0xcafeu;
    Check(pal4::WorldManagerWorldLoader::Load(
              kManager, "Qionghua", "Hall", runtime) == 7u &&
              runtime.scene_loads == 0u && runtime.camera_calls == 2u,
          "unchanged world skips reload but updates camera");
    Check(runtime.words[kManager + 56u] == kWorld &&
              runtime.words[kManager + 60u] == 0xa000u,
          "manager stores world and first camera object");
    Check(Position(runtime, "skill") < Position(runtime, "compact:312") &&
              Position(runtime, "compact:312") <
                  Position(runtime, "compact:328") &&
              Position(runtime, "compact:328") <
                  Position(runtime, "read-text:348") &&
              Position(runtime, "camera:0") <
                  Position(runtime, "camera:1") &&
              Position(runtime, "camera:1") <
                  Position(runtime, "set-camera"),
          "compaction, comparison and camera order");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(), "skill") == 2,
          "skill singleton touched before load and before camera attach");
}

void ChangedWorldReloadsAndUpdatesText() {
    Runtime runtime;
    runtime.texts[kManager + 348u] = "Old";
    runtime.texts[kManager + 364u] = "Hall";
    runtime.words[0xa100u] = 0xcafeu;
    Check(pal4::WorldManagerWorldLoader::Load(
              kManager, "Qionghua", "Hall", runtime) == 7u &&
              runtime.scene_loads == 1u &&
              runtime.texts[kManager + 348u] == "Qionghua" &&
              runtime.texts[kManager + 364u] == "Hall",
          "changed region reloads then saves both names");
    Check(Position(runtime, "scene-load") <
              Position(runtime, "assign:348") &&
              Position(runtime, "assign:348") <
              Position(runtime, "assign:364") &&
              Position(runtime, "assign:364") <
              Position(runtime, "camera:0"),
          "scene load precedes both text assignments and camera");

    Runtime changed_scene;
    changed_scene.texts[kManager + 348u] = "Qionghua";
    changed_scene.texts[kManager + 364u] = "Old";
    changed_scene.words[0xa100u] = 0xcafeu;
    (void)pal4::WorldManagerWorldLoader::Load(
        kManager, "Qionghua", "Hall", changed_scene);
    Check(changed_scene.scene_loads == 1u &&
              Position(changed_scene, "read-text:364") <
                  Position(changed_scene, "scene-load"),
          "changed scene alone triggers reload");
}

}  // namespace

int main() {
    UnchangedWorldSkipsSceneReload();
    ChangedWorldReloadsAndUpdatesText();
}
