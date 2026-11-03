#include "pal4/combat_scene_coordinator.h"
#include "pal4/combat_scene_callback.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatSceneRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0xb000u;
    std::uint32_t substitute_callback = 0u;
    std::uint8_t start_result = 0u;
    bool throw_show = false;
    float dialog_x = 0.0f;
    float dialog_y = 0.0f;

    Runtime() {
        words[0x2000u + 1412u] = 9u;
        words[0x2000u + 1364u] = 8u;
        words[0x4000u] = 0u;
        words[0x5100u + 4u] = 0x9000u;
        words[0x8c1cf0u] = 0x8000u;
        words[0x8c1cf8u] = 0x8100u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("byte " + std::to_string(address));
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("ui-manager");
        return 0x1000u;
    }
    void SetUIFlag(std::uint32_t manager, std::uint32_t flag) override {
        calls.push_back("ui-flag " + std::to_string(manager) + " " +
                        std::to_string(flag));
    }
    std::uint32_t InitializeConsole() override {
        calls.emplace_back("console");
        return 0x1100u;
    }
    std::uint32_t CreateScenarioDialog(std::uint32_t manager) override {
        calls.push_back("scenario " + std::to_string(manager));
        return 0x1200u;
    }
    void HideScenarioDialog(std::uint32_t dialog) override {
        calls.push_back("hide " + std::to_string(dialog));
    }
    std::uint32_t CreateHintWindow(std::uint32_t manager) override {
        calls.push_back("hint " + std::to_string(manager));
        return 0x1300u;
    }
    void CloseHintWindow(std::uint32_t window) override {
        calls.push_back("close " + std::to_string(window));
    }
    std::uint32_t GameState() override {
        calls.emplace_back("game-state");
        return 0x2000u;
    }
    void AssignXmlNode(std::uint32_t destination,
                       std::uint32_t source) override {
        calls.push_back("xml-assign " + std::to_string(destination) + " " +
                        std::to_string(source));
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("combat-system");
        return 0x3000u;
    }
    void SetupCombatCamera(std::uint32_t system) override {
        calls.push_back("camera-defaults " + std::to_string(system));
    }
    std::uint32_t GlobalDataSubsystem() override {
        calls.emplace_back("global-data");
        return 0x4000u;
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("game-db");
        return 0x5000u;
    }
    std::uint32_t FindGameString(std::uint32_t table,
                                 std::uint32_t id) override {
        calls.push_back("find-string " + std::to_string(table) + " " +
                        std::to_string(id));
        return 0x5100u;
    }
    std::uint32_t CreateCommonDialog(std::uint32_t manager) override {
        calls.push_back("common-dialog " + std::to_string(manager));
        return 0x6000u;
    }
    void ShowCommonDialog(std::uint32_t dialog, std::uint32_t message,
                          float x, float y, std::uint32_t mode,
                          std::uint32_t& callback) override {
        calls.push_back("show " + std::to_string(dialog) + " " +
                        std::to_string(message) + " " +
                        std::to_string(mode) + " " +
                        std::to_string(callback));
        dialog_x = x;
        dialog_y = y;
        if (substitute_callback != 0u) callback = substitute_callback;
        if (throw_show) throw std::runtime_error("show failed");
    }
    void DestroyCallback(std::uint32_t callback,
                         std::uint32_t flags) override {
        calls.push_back("callback-destroy " + std::to_string(callback) + " " +
                        std::to_string(flags));
    }
    std::uint32_t CameraManager() override {
        calls.emplace_back("camera-manager");
        return 0xa000u;
    }
    void ActivateCamera(std::uint32_t manager,
                        std::uint32_t name) override {
        calls.push_back("activate " + std::to_string(manager) + " " +
                        std::to_string(name));
    }
    std::uint8_t BeginCombat(std::uint32_t system, std::uint32_t xml_node,
                             std::uint32_t camera_mode) override {
        calls.push_back("begin " + std::to_string(system) + " " +
                        std::to_string(xml_node) + " " +
                        std::to_string(camera_mode));
        return start_result;
    }
    void CloseCombatUI(std::uint32_t system) override {
        calls.push_back("close-combat-ui " + std::to_string(system));
    }
};

std::size_t Position(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    return found == runtime.calls.end()
               ? runtime.calls.size()
               : static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckCommonState(const Runtime& runtime) {
    Check(runtime.words.at(0x7000u + 4u) == 9u &&
          runtime.words.at(0x7000u + 8u) == 8u &&
          runtime.words.at(0x2000u + 1412u) == 0u &&
          runtime.words.at(0x2000u + 1364u) == 1u &&
          Position(runtime, "xml-assign 28684 36864") <
              Position(runtime, "camera-defaults 12288") &&
          std::count(runtime.calls.begin(), runtime.calls.end(),
                     "console") == 2,
          "entry saves and resets game flags before camera setup");
}

void DirectAndGlobalRoutes() {
    Runtime direct;
    direct.bytes[0x3000u + 501u] = 1u;
    Check(pal4::CombatSceneCoordinator::Enter(
              0x7000u, 33u, 0x9000u, direct) == 1u,
          "direct route returns one even when native start returns zero");
    CheckCommonState(direct);
    Check(Position(direct, "activate 40960 33024") <
              Position(direct, "begin 12288 28684 0") &&
          Position(direct, "global-data") == direct.calls.size(),
          "combat camera route bypasses global data and dialog");

    Runtime normal;
    normal.words[0x4000u] = 3u;
    normal.start_result = 0xffu;
    Check(pal4::CombatSceneCoordinator::Enter(
              0x7000u, 0u, 0x9000u, normal) == 1u &&
          Position(normal, "begin 12288 28684 1") < normal.calls.size() &&
          std::count(normal.calls.begin(), normal.calls.end(),
                     "global-data") == 2,
          "nonzero global state uses normal camera and mode one");
    Runtime zero;
    Check(pal4::CombatSceneCoordinator::Enter(
              0x7000u, 0u, 0x9000u, zero) == 1u &&
          Position(zero, "begin 12288 28684 0") < zero.calls.size(),
          "zero global state uses combat camera and mode zero");
}

void DialogRoute() {
    Runtime runtime;
    runtime.words[0x4000u] = 2u;
    Check(pal4::CombatSceneCoordinator::Enter(
              0x7000u, 0u, 0x9000u, runtime) == 1u,
          "dialog branch returns one");
    CheckCommonState(runtime);
    Check(runtime.words.at(0x3000u + 508u) == 0u &&
          runtime.words.at(0xb000u) == 0x8485e0u &&
          runtime.words.at(0xb000u + 4u) == 0x58b270u &&
          runtime.words.at(0xb000u + 8u) == 0x7000u &&
          Position(runtime, "find-string 20532 110") <
              Position(runtime, "show 24576 36864 3 45056") &&
          runtime.calls.back() == "callback-destroy 45056 1" &&
          runtime.dialog_x == 0.4f && runtime.dialog_y == 0.4f,
          "dialog route installs callback tuple, finds text and releases owner");

    Runtime failed;
    failed.words[0x4000u] = 2u;
    failed.words[0x5100u + 4u] = 0u;
    failed.allocation = 0u;
    Check(pal4::CombatSceneCoordinator::Enter(
              0x7000u, 0u, 0x9000u, failed) == 1u &&
          Position(failed, "show 24576 8657252 3 0") < failed.calls.size() &&
          Position(failed, "callback-destroy 0 1") == failed.calls.size(),
          "allocation failure still shows dialog with empty text fallback");

    Runtime substituted;
    substituted.words[0x4000u] = 2u;
    substituted.substitute_callback = 0x7200u;
    static_cast<void>(pal4::CombatSceneCoordinator::Enter(
        0x7000u, 0u, 0x9000u, substituted));
    Check(substituted.calls.back() == "callback-destroy 29184 1",
          "dialog can replace the owned callback pointer");

    Runtime throwing;
    throwing.words[0x4000u] = 2u;
    throwing.throw_show = true;
    bool threw = false;
    try { static_cast<void>(pal4::CombatSceneCoordinator::Enter(
        0x7000u, 0u, 0x9000u, throwing)); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw && throwing.calls.back() == "callback-destroy 45056 1",
          "dialog exception releases the callback owner");
}

void StartCombat() {
    Runtime runtime;
    runtime.start_result = 7u;
    Check(pal4::CombatSceneCoordinator::StartCombat(
              0x7000u, 1u, runtime) == 7u &&
          runtime.calls == std::vector<std::string>{
              "camera-manager", "read 9182448", "activate 40960 32768",
              "combat-system", "begin 12288 28684 1"},
          "mode one selects normal camera and forwards byte return");
    runtime.calls.clear();
    Check(pal4::CombatSceneCoordinator::StartCombat(
              0x7000u, 2u, runtime) == 7u &&
          runtime.calls == std::vector<std::string>{
              "camera-manager", "read 9182456", "activate 40960 33024",
              "combat-system", "begin 12288 28684 0"},
          "other modes select combat camera and zero start flag");
}

void LeaveCombat() {
    Runtime runtime;
    runtime.words[0x7000u + 4u] = 12u;
    runtime.words[0x7000u + 8u] = 13u;
    Check(pal4::CombatSceneCoordinator::Leave(
              0x7000u, 99u, runtime) == 1u &&
          runtime.words[0x2000u + 1412u] == 12u &&
          runtime.words[0x2000u + 1364u] == 13u &&
          Position(runtime, "write 9604 12") <
              Position(runtime, "write 9556 13") &&
          Position(runtime, "write 9556 13") <
              Position(runtime, "close-combat-ui 12288") &&
          Position(runtime, "close-combat-ui 12288") <
              Position(runtime, "close 4864"),
          "leave wrapper restores saved flags before closing combat and hint UI");
    runtime.calls.clear();
    Check(pal4::CombatSceneCoordinator::FinishLeave(0x7000u, runtime) == 1u &&
          runtime.calls.back() == "close 4864",
          "direct leave body returns one after hint-window close");
}

void CallbackClone() {
    Runtime runtime;
    runtime.allocation = 0xb000u;
    runtime.words[0x7000u + 4u] = 0x1234u;
    runtime.words[0x7000u + 8u] = 0x5678u;
    Check(pal4::CombatSceneCallback::Clone(0x7000u, runtime) == 0xb000u &&
          runtime.words[0xb000u] == 0x8485e0u &&
          runtime.words[0xb000u + 4u] == 0x1234u &&
          runtime.words[0xb000u + 8u] == 0x5678u &&
          runtime.calls == std::vector<std::string>{
              "allocate 12", "write 45056 8664528", "read 28676",
              "write 45060 4660", "read 28680", "write 45064 22136",
              "write 45056 8685024"},
          "callback clone writes base vtable, copied fields and final vtable");
    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(pal4::CombatSceneCallback::Clone(0x7000u, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"allocate 12"},
          "callback allocation failure skips all source reads and writes");
}

}  // namespace

int main() {
    try {
        DirectAndGlobalRoutes();
        DialogRoute();
        StartCombat();
        LeaveCombat();
        CallbackClone();
        std::cout << "combat scene coordinator tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
