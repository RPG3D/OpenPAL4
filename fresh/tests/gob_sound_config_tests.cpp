#include "pal4/gob_sound.h"
#include "pal4/gob_sound_config.h"

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

struct Runtime final : pal4::IGobSoundConfigRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::map<std::string, float> config_floats;
    std::string config_name = "Wind";
    std::string assigned_name;
    std::uint32_t scene_package = 0u;
    std::uint32_t model_result = 0x77u;
    std::uint32_t parameters = 0x8100u;
    std::uint32_t old_sound = 0x8200u;
    std::uint32_t play_result = 0x99u;
    bool playing = false;
    std::uint8_t stop_result = 2u;
    std::uint8_t pause_result = 3u;
    std::uint8_t active = 0u;

    Runtime() {
        words[object + 0x1c0u] = 0x8000u;
        words[object + 0x14u] = 0x7000u;
        words[object + 0x188u] = 0x55u;
    }
    void CleanupBeforeSceneLoad(std::uint32_t address) override {
        Check(address == object, "scene cleanup object");
        calls.emplace_back("preload cleanup");
    }
    bool UsesObjectPackage(std::string_view path) override {
        calls.emplace_back("check object");
        return path.find("palobject") != std::string_view::npos;
    }
    bool UsesWeaponPackage(std::string_view path) override {
        calls.emplace_back("check weapon");
        return path.find("palweapon") != std::string_view::npos;
    }
    void LoadObjectPackage() override { calls.emplace_back("load object"); }
    void LoadWeaponPackage() override { calls.emplace_back("load weapon"); }
    std::uint32_t CurrentScenePackage() override {
        calls.emplace_back("scene package");
        return scene_package;
    }
    void LoadScenePackage(std::uint32_t package) override {
        Check(package == 0x7000u, "scene package address");
        calls.emplace_back("load scene");
    }
    std::uint32_t InitializeBaseModel(
        std::uint32_t address, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) override {
        Check(address == object && first == 2u && fourth == 3u &&
              fifth == 4u && !path.empty(), "base model arguments");
        calls.emplace_back("initialize model");
        return model_result;
    }
    void CleanupScriptPackages() override {
        calls.emplace_back("cleanup packages");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write " + std::to_string(address - object));
    }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.emplace_back("float " + std::to_string(address - object));
    }
    std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) override {
        calls.emplace_back("find " + std::string(name));
        if (parent == 0x8000u && name == "PAL4-GameObject-Parameters")
            return parameters;
        if (parent == parameters && name == "PAL4-GameObject-sound")
            return old_sound;
        return 0u;
    }
    void RemoveXmlChild(std::uint32_t parent,
                        std::uint32_t child) override {
        Check(parent == parameters && child == old_sound,
              "remove old sound node");
        calls.emplace_back("remove sound");
    }
    std::uint32_t CreateXmlChild(std::uint32_t parent,
                                  std::string_view name) override {
        Check(parent == parameters && name == "PAL4-GameObject-sound",
              "create sound node");
        calls.emplace_back("create sound");
        return 0x8300u;
    }
    std::uint32_t MovementAdapter() override {
        calls.emplace_back("movement adapter");
        return 0x9000u;
    }
    std::uint32_t RenderProperty(std::uint32_t address) override {
        Check(address == object, "render property object");
        calls.emplace_back("render property");
        return 0x66u;
    }
    std::uint32_t SetClumpProperties(std::uint32_t adapter,
        std::uint32_t clump, std::uint32_t property,
        std::uint32_t first, std::uint32_t second,
        std::uint32_t render_property) override {
        Check(adapter == 0x9000u && clump == 0x7000u &&
              property == 0x55u && first == 1u && second == 0u &&
              render_property == 0x66u, "clump properties arguments");
        calls.emplace_back("set clump properties");
        return 0x11u;
    }
    std::uint32_t InstallRenderCallback(std::uint32_t adapter,
                                        std::uint32_t clump) override {
        Check(adapter == 0x9000u && clump == 0x7000u,
              "render callback arguments");
        calls.emplace_back("render callback");
        return 0x22u;
    }
    bool IsAudioPlaying(std::uint32_t handle) override {
        Check(handle == object + 0x1e0u, "audio handle address");
        calls.emplace_back("is playing");
        return playing;
    }
    std::uint8_t StopAudio(std::uint32_t handle) override {
        Check(handle == object + 0x1e0u, "stop handle address");
        calls.emplace_back("stop audio");
        return stop_result;
    }
    std::uint8_t PauseAudio(std::uint32_t handle) override {
        Check(handle == object + 0x1e0u, "pause handle address");
        calls.emplace_back("pause audio");
        return pause_result;
    }
    void ApplyBaseParameters(std::uint32_t address) override {
        Check(address == object, "base parameters object");
        calls.emplace_back("apply base parameters");
    }
    std::string ReadConfigString(std::uint32_t node,
        std::string_view key, std::string_view fallback) override {
        Check(node == old_sound && key == "PAL4-GameObject-sound-name" &&
              fallback.empty(), "config sound name key");
        calls.emplace_back("read name");
        return config_name;
    }
    float ReadConfigFloat(std::uint32_t node,
        std::string_view key, float fallback) override {
        Check(node == old_sound &&
              (key == "PAL4-GameObject-sound-mintime" ||
               key == "PAL4-GameObject-sound-maxtime"),
              "config sound float key");
        calls.emplace_back("read " + std::string(key));
        const auto it = config_floats.find(std::string(key));
        return it == config_floats.end() ? fallback : it->second;
    }
    void AssignNameRecord(std::uint32_t record,
                          std::string_view name) override {
        Check(record == object + 0x1e4u, "name record offset");
        assigned_name = name;
        calls.emplace_back("assign name");
    }
    std::uint8_t CheckAudioActive(std::uint32_t handle) override {
        Check(handle == object + 0x1e0u, "active handle address");
        calls.emplace_back("check active");
        return active;
    }
    std::uint32_t PlayAudioHandle(std::uint32_t handle) override {
        Check(handle == object + 0x1e0u, "play handle address");
        calls.emplace_back("play handle");
        return play_result;
    }
};

void CheckScene() {
    for (const auto& [path, route] :
         std::vector<std::pair<std::string, std::string>>{
             {"palobject/palweapon", "load object"},
             {"palweapon", "load weapon"},
             {"scene", "load scene"}}) {
        Runtime runtime;
        runtime.scene_package = 0x7000u;
        Check(pal4::GobSound::InitializeSceneAndConfig(
                  Runtime::object, 2, path, 3, 4, runtime) == 0x77u &&
              runtime.words[Runtime::object + 0x1dcu] == 0u &&
              std::find(runtime.calls.begin(), runtime.calls.end(), route) !=
                  runtime.calls.end() &&
              runtime.calls[runtime.calls.size() - 3] == "create sound" &&
              runtime.calls[runtime.calls.size() - 2] == "cleanup packages" &&
              runtime.calls.back() == "write 476",
              "scene package route and sound node creation");
    }
    Runtime failed;
    failed.model_result = 0u;
    Check(pal4::GobSound::InitializeSceneAndConfig(
              Runtime::object, 2, "scene", 3, 4, failed) == 0u &&
          failed.calls.back() == "cleanup packages" &&
          failed.calls.size() == 6u,
          "failed base model skips sound config");
    Runtime no_document;
    no_document.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobSound::InitializeSceneAndConfig(
              Runtime::object, 2, "scene", 3, 4, no_document) == 0u &&
          no_document.calls.back() == "cleanup packages",
          "missing document stops sound setup");
    Runtime no_parameters;
    no_parameters.parameters = 0u;
    Check(pal4::GobSound::InitializeSceneAndConfig(
              Runtime::object, 2, "scene", 3, 4, no_parameters) == 0x77u &&
          no_parameters.calls.back() == "write 476",
          "missing parameters still clear suppression");
}

void CheckAdapterAndPlayback() {
    Runtime runtime;
    Check(pal4::GobSound::ApplyClumpAdapterProperties(
              Runtime::object, runtime) == 0x11u &&
          runtime.calls == (std::vector<std::string>{
              "movement adapter", "render property", "set clump properties"}),
          "adapter properties arguments and order");
    runtime.calls.clear();
    Check(pal4::GobSound::SetClumpRenderCallback(
              Runtime::object, runtime) == 0x22u &&
          runtime.calls == (std::vector<std::string>{
              "movement adapter", "render callback"}),
          "render callback forwarding");
    runtime.calls.clear();
    Check(pal4::GobSound::StopPlaybackAndSuppress(
              Runtime::object, runtime) == 0u &&
          runtime.words[Runtime::object + 0x1dcu] == 1u &&
          runtime.calls == (std::vector<std::string>{
              "is playing", "write 476"}),
          "stop with idle sound only suppresses");
    runtime.playing = true;
    runtime.calls.clear();
    Check(pal4::GobSound::StopPlaybackAndSuppress(
              Runtime::object, runtime) == 2u &&
          runtime.calls == (std::vector<std::string>{
              "is playing", "stop audio", "write 476"}),
          "playing sound stops before suppression");
    runtime.calls.clear();
    Check(pal4::GobSound::ResumePlaybackAndUnsuppress(
              Runtime::object, runtime) == 1u &&
          runtime.words[Runtime::object + 0x1dcu] == 0u &&
          runtime.calls == (std::vector<std::string>{
              "is playing", "write 476"}),
          "playing sound skips pause on resume");
    runtime.playing = false;
    runtime.calls.clear();
    Check(pal4::GobSound::ResumePlaybackAndUnsuppress(
              Runtime::object, runtime) == 3u &&
          runtime.calls == (std::vector<std::string>{
              "is playing", "pause audio", "write 476"}),
          "idle sound invokes pause helper before unsuppressing");
}

void CheckConfiguration() {
    Runtime runtime;
    Check(pal4::GobSound::GetConfigNode(Runtime::object, runtime) ==
              0x8200u, "sound config node lookup");
    runtime.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobSound::GetConfigNode(Runtime::object, runtime) == 0u,
          "missing document config gate");
    runtime.words[Runtime::object + 0x1c0u] = 0x8000u;
    runtime.parameters = 0u;
    Check(pal4::GobSound::GetConfigNode(Runtime::object, runtime) == 0u,
          "missing parameters config gate");
    Runtime values;
    values.config_floats["PAL4-GameObject-sound-mintime"] = 2.0f;
    values.config_floats["PAL4-GameObject-sound-maxtime"] = 4.0f;
    values.playing = true;
    Check(pal4::GobSound::ReadConfigAndResetAudio(
              Runtime::object, values) == 0x99u &&
          values.assigned_name == "Wind" &&
          values.floats[Runtime::object + 0x1f4u] == 2.0f &&
          values.floats[Runtime::object + 0x1f8u] == 4.0f &&
          values.words[Runtime::object + 0x200u] == 0u &&
          values.words[Runtime::object + 0x204u] == 0u &&
          values.calls.front() == "apply base parameters" &&
          values.calls[values.calls.size() - 2] == "is playing" &&
          values.calls.back() == "play handle",
          "sound name, timer fields and playing handle refresh");
    Runtime defaults;
    Check(pal4::GobSound::ReadConfigAndResetAudio(
              Runtime::object, defaults) == 0u &&
          defaults.floats[Runtime::object + 0x1f4u] == 0.0f &&
          defaults.floats[Runtime::object + 0x1f8u] == 1.0f &&
          defaults.calls.back() == "check active",
          "missing timer attributes use zero/one defaults");
    Runtime active;
    active.active = 1u;
    Check(pal4::GobSound::ReadConfigAndResetAudio(
              Runtime::object, active) == 0x99u &&
          active.calls[active.calls.size() - 2] == "check active" &&
          active.calls.back() == "play handle",
          "active idle audio handle refreshes playback");
    Runtime missing;
    missing.old_sound = 0u;
    Check(pal4::GobSound::ReadConfigAndResetAudio(
              Runtime::object, missing) == 0u &&
          missing.calls == (std::vector<std::string>{
              "apply base parameters", "find PAL4-GameObject-Parameters",
              "find PAL4-GameObject-sound"}),
          "missing sound node preserves config fields");
}

}  // namespace

int main() {
    try {
        CheckScene();
        CheckAdapterAndPlayback();
        CheckConfiguration();
        std::cout << "gob_sound_config_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
