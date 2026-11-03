#include "pal4/gob_obstacle.h"
#include "pal4/gob_obstacle_config.h"

#include <bit>
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

struct Runtime final : pal4::IGobObstacleConfigRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::map<std::uint32_t, std::string> strings;
    std::map<std::string, std::int32_t> int_values;
    std::map<std::string, float> float_values;
    std::map<std::string, std::string> string_values;
    std::vector<std::string> calls;
    std::uint32_t scene_package = 0u;
    std::uint32_t machine_result = 0x77u;
    std::uint32_t parameters = 0x8100u;
    std::uint32_t old_obstacle = 0x8200u;

    Runtime() { words[object + 0x1c0u] = 0x8000u; }
    void CleanupBeforeSceneLoad(std::uint32_t address) override {
        Check(address == object, "preload cleanup address");
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
        Check(package == 0x7000u, "scene package value");
        calls.emplace_back("load scene");
    }
    std::uint32_t InitializeMachineScene(std::uint32_t address,
        std::uint32_t first, std::string_view path,
        std::uint32_t fourth, std::uint32_t fifth) override {
        Check(address == object && first == 2 && fourth == 3 &&
              fifth == 4 && !path.empty(), "machine scene arguments");
        calls.emplace_back("initialize machine scene");
        return machine_result;
    }
    void CleanupScriptPackages() override {
        calls.emplace_back("cleanup packages");
    }
    void AttachObstacleHelper(std::uint32_t helper,
                              std::uint32_t address) override {
        Check(helper == object + 0x2ccu && address == object,
              "helper attachment");
        calls.emplace_back("attach helper");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word " + std::to_string(address - object));
    }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.emplace_back("write float " + std::to_string(address - object));
    }
    std::uint32_t FindXmlChild(std::uint32_t parent,
                                std::string_view name) override {
        calls.emplace_back("find " + std::string(name));
        if (parent == 0x8000u && name == "PAL4-GameObject-Parameters")
            return parameters;
        if (parent == parameters && name == "PAL4_GameObject-obstacle")
            return old_obstacle;
        return 0u;
    }
    void RemoveXmlChild(std::uint32_t parent,
                        std::uint32_t child) override {
        Check(parent == parameters && child == old_obstacle,
              "old obstacle removal");
        calls.emplace_back("remove obstacle");
    }
    std::uint32_t CreateXmlChild(std::uint32_t parent,
                                  std::string_view name) override {
        Check(parent == parameters && name == "PAL4_GameObject-obstacle",
              "obstacle child creation");
        calls.emplace_back("create obstacle");
        return 0x8300u;
    }
    void LoadMachineConfig(std::uint32_t address) override {
        Check(address == object, "machine config address");
        calls.emplace_back("load machine config");
    }
    std::int32_t ReadConfigInt(std::uint32_t node,
        std::string_view key, std::int32_t fallback) override {
        Check(node == old_obstacle, "integer config node");
        calls.emplace_back("int " + std::string(key));
        const auto it = int_values.find(std::string(key));
        return it == int_values.end() ? fallback : it->second;
    }
    float ReadConfigFloat(std::uint32_t node,
        std::string_view key, float fallback) override {
        Check(node == old_obstacle, "float config node");
        calls.emplace_back("float " + std::string(key));
        const auto it = float_values.find(std::string(key));
        return it == float_values.end() ? fallback : it->second;
    }
    std::string ReadConfigString(std::uint32_t node,
        std::string_view key, std::string_view fallback) override {
        Check(node == old_obstacle, "string config node");
        calls.emplace_back("string " + std::string(key));
        const auto it = string_values.find(std::string(key));
        return it == string_values.end() ? std::string(fallback) : it->second;
    }
    void AssignStringRecord(std::uint32_t record,
                            std::string_view value) override {
        Check(record == object + 0x308u || record == object + 0x320u,
              "string record offset");
        strings[record] = value;
        calls.emplace_back("assign string " +
                           std::to_string(record - object));
    }
    std::uint32_t TimeMilliseconds() override {
        calls.emplace_back("time");
        return 1234u;
    }
    float RandomDelay(float minimum, float maximum, float seed) override {
        Check(minimum == floats[object + 0x314u] &&
              maximum == floats[object + 0x318u] && seed == 1234.0f,
              "random delay arguments");
        calls.emplace_back("random delay");
        return 2.5f;
    }
};

void CheckScene() {
    const std::vector<std::pair<std::string, std::vector<std::string>>> routes{
        {"palobject/palweapon", {"preload cleanup", "check object",
            "load object", "initialize machine scene", "cleanup packages",
            "attach helper", "find PAL4-GameObject-Parameters",
            "find PAL4_GameObject-obstacle", "remove obstacle",
            "create obstacle"}},
        {"palweapon", {"preload cleanup", "check object", "check weapon",
            "load weapon", "initialize machine scene", "cleanup packages",
            "attach helper", "find PAL4-GameObject-Parameters",
            "find PAL4_GameObject-obstacle", "remove obstacle",
            "create obstacle"}},
        {"scene", {"preload cleanup", "check object", "check weapon",
            "scene package", "load scene", "initialize machine scene",
            "cleanup packages", "attach helper",
            "find PAL4-GameObject-Parameters",
            "find PAL4_GameObject-obstacle", "remove obstacle",
            "create obstacle"}}
    };
    for (const auto& [path, calls] : routes) {
        Runtime runtime;
        runtime.scene_package = 0x7000u;
        Check(pal4::GobObstacle::InitializeSceneAndConfig(
                  Runtime::object, 2, path, 3, 4, runtime) == 0x77u &&
              runtime.calls == calls,
              "scene package precedence and XML replacement");
    }
    Runtime failed;
    failed.machine_result = 0u;
    Check(pal4::GobObstacle::InitializeSceneAndConfig(
              Runtime::object, 2, "scene", 3, 4, failed) == 0u &&
          failed.calls == (std::vector<std::string>{
              "preload cleanup", "check object", "check weapon",
              "scene package", "initialize machine scene",
              "cleanup packages"}), "failed machine initialization stops");
    Runtime no_parameters;
    no_parameters.parameters = 0u;
    Check(pal4::GobObstacle::InitializeSceneAndConfig(
              Runtime::object, 2, "scene", 3, 4, no_parameters) == 0x77u &&
          no_parameters.calls.back() == "find PAL4-GameObject-Parameters",
          "missing parameters skips obstacle node");
    Runtime no_previous;
    no_previous.old_obstacle = 0u;
    Check(pal4::GobObstacle::InitializeSceneAndConfig(
              Runtime::object, 2, "scene", 3, 4, no_previous) == 0x77u &&
          no_previous.calls.back() == "create obstacle",
          "absent obstacle node creates new one");
}

void CheckConfigNode() {
    Runtime runtime;
    Check(pal4::GobObstacle::GetConfigNode(Runtime::object, runtime) ==
              0x8200u, "obstacle config node lookup");
    runtime.words[Runtime::object + 0x1c0u] = 0u;
    runtime.calls.clear();
    Check(pal4::GobObstacle::GetConfigNode(Runtime::object, runtime) == 0u &&
          runtime.calls.empty(), "missing document bypasses XML lookup");
    runtime.words[Runtime::object + 0x1c0u] = 0x8000u;
    runtime.parameters = 0u;
    Check(pal4::GobObstacle::GetConfigNode(Runtime::object, runtime) == 0u,
          "missing parameters bypass obstacle lookup");
}

void CheckConfiguration() {
    Runtime runtime;
    runtime.int_values["PAL4_GameObject-obstacle_selfvisible"] = 0;
    runtime.int_values["PAL4_GAMEOBJECT_obstacle_applyDirect"] = 1;
    runtime.int_values["PAL4_GameObject-obstacle_effType"] = 2;
    runtime.float_values["PAL4_GameObject-obstacle_suction"] = 1.25f;
    runtime.float_values["PAL4_GameObject-obstacle_thrust"] = 3.5f;
    runtime.float_values["PAL4_GameObject-obstacle_effTime"] = 4.0f;
    runtime.float_values["PAL4_GameObject-obstacle_effDuration"] = 5.0f;
    runtime.float_values["PAL4_GameObject-obstacle_effHurt"] = 6.0f;
    runtime.float_values["CSTR_GAMEOBJECT_obstacle_launch_tMin"] = 1.0f;
    runtime.float_values["CSTR_GAMEOBJECT_obstacle_launch_tMax"] = 3.0f;
    runtime.string_values["CSTR_GAMEOBJECT_obstacle_launch_name"] = "Door";
    runtime.string_values["CSTR_GAMEOBJECT_obstacle_launchEff"] = "Spark";
    Check(pal4::GobObstacle::ReadConfigAndResetLaunchRuntime(
              Runtime::object, runtime) == std::bit_cast<std::uint32_t>(3.5f) &&
          runtime.words[Runtime::object + 0x170u] == 0u &&
          runtime.floats[Runtime::object + 0x2d8u] == 1.25f &&
          runtime.floats[Runtime::object + 0x2dcu] == 3.5f &&
          runtime.words[Runtime::object + 0x2f4u] == 1u &&
          runtime.words[Runtime::object + 0x2f8u] == 2u &&
          runtime.floats[Runtime::object + 0x2fcu] == 4.0f &&
          runtime.floats[Runtime::object + 0x300u] == 5.0f &&
          runtime.floats[Runtime::object + 0x304u] == 6.0f &&
          runtime.strings[Runtime::object + 0x308u] == "Door" &&
          runtime.strings[Runtime::object + 0x320u] == "Spark" &&
          runtime.floats[Runtime::object + 0x314u] == 1.0f &&
          runtime.floats[Runtime::object + 0x318u] == 3.0f &&
          runtime.floats[Runtime::object + 0x31cu] == 2.5f &&
          runtime.words[Runtime::object + 0x2f0u] == 0u &&
          runtime.floats[Runtime::object + 0x2ecu] == 3.5f,
          "all twelve config fields and launch runtime reset");
    Check(runtime.calls.front() == "load machine config" &&
          runtime.calls[runtime.calls.size() - 2] == "write word 752" &&
          runtime.calls.back() == "write float 748",
          "machine load precedes obstacle config and runtime reset follows");
    Runtime defaults;
    Check(pal4::GobObstacle::ReadConfigAndResetLaunchRuntime(
              Runtime::object, defaults) == 0u &&
          defaults.words[Runtime::object + 0x170u] == 1u &&
          defaults.floats[Runtime::object + 0x318u] == 1.0f &&
          defaults.strings[Runtime::object + 0x308u].empty() &&
          defaults.strings[Runtime::object + 0x320u].empty(),
          "missing config values use IDA defaults");
    Runtime missing;
    missing.old_obstacle = 0u;
    Check(pal4::GobObstacle::ReadConfigAndResetLaunchRuntime(
              Runtime::object, missing) == 0u &&
          missing.calls == (std::vector<std::string>{
              "load machine config", "find PAL4-GameObject-Parameters",
              "find PAL4_GameObject-obstacle"}),
          "absent obstacle node preserves fields");
}

}  // namespace

int main() {
    try {
        CheckScene();
        CheckConfigNode();
        CheckConfiguration();
        std::cout << "gob_obstacle_config_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
