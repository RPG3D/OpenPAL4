#include "pal4/script_builtin_handlers.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct ImmediateRuntime final : pal4::IScriptImmediateModeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t ScriptInstance() override {
        calls.emplace_back("instance");
        return 0x1000;
    }
    std::uint32_t SetDebuggerMode(std::uint32_t debugger,
                                  std::uint32_t mode) override {
        Check(debugger == 0x1054 && mode == 1,
              "IMM begin debugger pointer and mode");
        calls.emplace_back("set mode");
        return 0xabcd;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read " + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write " + std::to_string(address));
        words[address] = value;
    }
};

struct NewGameRuntime final : pal4::IScriptNewGameRuntime {
    std::vector<std::string> calls;
    std::uint32_t player_calls = 0;
    std::uint32_t actor_calls = 0;
    std::uint32_t InitializePlayerManager() override {
        calls.emplace_back("player instance");
        return 0x2000 + ++player_calls;
    }
    void ResetAllPlayerStates(std::uint32_t manager) override {
        Check(manager == 0x2001, "reset receives first manager result");
        calls.emplace_back("reset players");
    }
    void LoadAndProcessPlayerData(std::uint32_t manager) override {
        Check(manager == 0x2002, "load receives second manager result");
        calls.emplace_back("load players");
    }
    std::uint32_t ActorControlInstance() override {
        calls.emplace_back("actor instance");
        return 0x3000 + ++actor_calls;
    }
    void ShutdownActorControl(std::uint32_t control) override {
        Check(control == 0x3001, "shutdown receives first actor result");
        calls.emplace_back("shutdown actor");
    }
    int InitializeActorControl(std::uint32_t control) override {
        Check(control == 0x3002, "initialize receives second actor result");
        calls.emplace_back("initialize actor");
        return -37;
    }
};

struct CameraRuntime final : pal4::IScriptCameraRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, float> values;
    std::array<float, 4> deltas{};
    std::uint8_t option = 0;
    std::uint32_t InitializeCameraSubsystem() override {
        calls.emplace_back("initialize subsystem");
        return 0x1000;
    }
    std::uint32_t ActiveCameraObject(std::uint32_t manager) override {
        Check(manager == 0x1000, "active object manager");
        calls.emplace_back("active object");
        return 0x2000;
    }
    std::uint32_t ActiveCameraInternalId(std::uint32_t manager) override {
        Check(manager == 0x1000, "internal camera manager");
        calls.emplace_back("internal id");
        return 0x3000;
    }
    int AdjustAngles(std::uint32_t camera, float yaw, float pitch,
                     float roll, std::uint8_t flags) override {
        Check(camera == 0x2000, "angle adjustment camera");
        calls.emplace_back("angles");
        deltas = {yaw, pitch, roll, 0};
        option = flags;
        return -11;
    }
    int AdjustDistance(std::uint32_t camera, float distance,
                       std::uint8_t flags) override {
        Check(camera == 0x2000, "distance adjustment camera");
        calls.emplace_back("distance");
        deltas = {0, 0, 0, distance};
        option = flags;
        return -12;
    }
    int AdjustAnglesAndDistance(std::uint32_t camera, float yaw,
                                float pitch, float roll, float distance,
                                std::uint8_t flags) override {
        Check(camera == 0x2000, "combined adjustment camera");
        calls.emplace_back("combined");
        deltas = {yaw, pitch, roll, distance};
        option = flags;
        return -13;
    }
    float CameraFloat(std::uint32_t address) override {
        calls.emplace_back("read " + std::to_string(address));
        return values.at(address);
    }
};

void TestImmediateMode() {
    ImmediateRuntime runtime;
    Check(pal4::ScriptImmediateMode::Begin(runtime) == 0xabcd &&
              runtime.calls ==
                  std::vector<std::string>{"instance", "set mode"},
          "IMM begin forwards debugger pointer and result");
    runtime.calls.clear();
    runtime.words[0x1060] = 0;
    Check(pal4::ScriptImmediateMode::End(runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                                   "instance", "read 4192"},
          "IMM end with no context makes no writes");
    runtime.calls.clear();
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x2110] = 5;
    Check(pal4::ScriptImmediateMode::End(runtime) == 0x2000 &&
              runtime.words[0x210c] == 5 &&
              runtime.words[0x2110] == 0 &&
              runtime.calls == std::vector<std::string>{
                                   "instance", "read 4192", "read 8464",
                                   "write 8460", "read 4192", "write 8464"},
          "IMM end restores previous mode and clears it");
}

void TestNewGame() {
    NewGameRuntime runtime;
    Check(pal4::ScriptNewGameHandler::Run(runtime) == -37 &&
              runtime.player_calls == 2 && runtime.actor_calls == 2 &&
              runtime.calls == std::vector<std::string>{
                                   "player instance", "reset players",
                                   "player instance", "load players",
                                   "actor instance", "shutdown actor",
                                   "actor instance", "initialize actor"},
          "new game resets, reloads, shuts down and initializes in order");
}

void TestCamera() {
    CameraRuntime runtime;
    Check(pal4::ScriptCameraHandler::AdjustYpr(
              1.25f, -2.5f, 3.75f, 0x123, runtime) == -11 &&
              runtime.deltas == std::array<float, 4>{1.25f, -2.5f, 3.75f, 0} &&
              runtime.option == 0x23 &&
              runtime.calls == std::vector<std::string>{
                                   "initialize subsystem", "active object",
                                   "angles"},
          "YPR adjustment uses deltas and low option byte");
    runtime.calls.clear();
    Check(pal4::ScriptCameraHandler::AdjustScalar(
              -4.5f, -1, runtime) == -12 &&
              runtime.deltas == std::array<float, 4>{0, 0, 0, -4.5f} &&
              runtime.option == 0xff &&
              runtime.calls == std::vector<std::string>{
                                   "initialize subsystem", "active object",
                                   "distance"},
          "scalar adjustment preserves signed delta and masks option");
    runtime.calls.clear();
    Check(pal4::ScriptCameraHandler::AdjustYprd(
              1, 2, 3, 4, 0x100, runtime) == -13 &&
              runtime.deltas == std::array<float, 4>{1, 2, 3, 4} &&
              runtime.option == 0 &&
              runtime.calls == std::vector<std::string>{
                                   "initialize subsystem", "active object",
                                   "combined"},
          "combined adjustment forwards four deltas");

    runtime.values = {{0x303c, 0.1f}, {0x3040, -2.25f},
                      {0x3044, 3.5f}, {0x3048, 9.75f}};
    using pal4::ScriptCameraHandler;
    const std::array<double, 4> observed{
        ScriptCameraHandler::Yaw(runtime),
        ScriptCameraHandler::Pitch(runtime),
        ScriptCameraHandler::Roll(runtime),
        ScriptCameraHandler::Distance(runtime)};
    Check(observed == std::array<double, 4>{
                          static_cast<double>(0.1f), -2.25, 3.5, 9.75} &&
              runtime.calls.size() == 15,
          "camera getters read four offsets and promote float to double");
    Check(runtime.calls[12] == "initialize subsystem" &&
              runtime.calls[13] == "internal id" &&
              runtime.calls[14] == "read 12360",
          "distance getter uses internal ID and offset 72");
}

}  // namespace

int main() {
    try {
        TestImmediateMode();
        TestNewGame();
        TestCamera();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
