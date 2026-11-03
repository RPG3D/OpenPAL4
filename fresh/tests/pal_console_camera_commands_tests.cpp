#include "pal4/pal_console_camera_commands.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalConsoleCameraRuntime {
    static constexpr std::uint32_t kCameraManager = 0x3000u;
    static constexpr std::uint32_t kPlayerManager = 0x1000u;
    static constexpr std::uint32_t kPlayer = kPlayerManager + 2u * 2836u;

    std::vector<std::string> calls;
    std::string output;
    std::uint32_t player_manager = kPlayerManager;
    std::uint32_t player_index = 2u;
    std::uint32_t camera_mode_state = 0u;
    std::uint32_t written_address = 0u;
    std::uint32_t written_value = 99u;
    std::uint32_t optimization_value = 99u;

    std::uint32_t CameraManager() override {
        calls.emplace_back("camera manager");
        return kCameraManager;
    }
    std::uint32_t PlayerManager() override {
        calls.emplace_back("player manager");
        return player_manager;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (address == kCameraManager + 0xc0u) {
            calls.emplace_back("camera state");
            return camera_mode_state;
        }
        Check(address == player_manager + 0x2c50u,
              "player manager index offset");
        calls.emplace_back("player index");
        return player_index;
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("autoseek write");
        written_address = address;
        written_value = value;
    }
    void SetModeByIndex(std::uint32_t manager,
                        std::uint32_t mode) override {
        Check(manager == kCameraManager && mode <= 5u,
              "camera mode operands");
        calls.emplace_back("set mode " + std::to_string(mode));
    }
    std::uint32_t ActiveCameraObject(std::uint32_t manager) override {
        Check(manager == kCameraManager, "camera object manager");
        calls.emplace_back("camera object");
        return 0x4000u;
    }
    void SetLookAtFromObjectData(std::uint32_t camera,
                                 std::uint32_t enabled) override {
        Check(camera == 0x4000u && enabled == 1u, "look-at operands");
        calls.emplace_back("look-at");
    }
    std::uint32_t ActorControl() override {
        calls.emplace_back("actor control");
        return 0x5000u;
    }
    void FlushMainCameraTailYaw(std::uint32_t control) override {
        Check(control == 0x5000u, "actor control operand");
        calls.emplace_back("flush yaw");
    }
    std::uint32_t ActiveCameraInternalId(std::uint32_t manager) override {
        Check(manager == kCameraManager, "internal camera manager");
        calls.emplace_back("camera id");
        return 0x6000u;
    }
    void SetPositionAndUpdateMatrix(std::uint32_t camera,
                                    std::uint32_t x, std::uint32_t y,
                                    std::uint32_t z,
                                    std::uint32_t update) override {
        Check(camera == 0x6000u && x == 0u && y == 0u &&
                  z == 0u && update == 1u,
              "camera position operands");
        calls.emplace_back("position");
    }
    std::int32_t SetDistanceOptimization(std::uint32_t enabled) override {
        calls.emplace_back("distance optimization");
        optimization_value = enabled;
        return 43;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x7000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view format) override {
        Check(console == 0x7000u, "console operand");
        calls.emplace_back("output");
        output = format;
        return -7;
    }
};

void Autoseek() {
    for (const auto [input, expected] :
         {std::pair{"0", 0u}, std::pair{"1", 1u},
          std::pair{"", 0u},
          std::pair{"nonnumeric", 0u}}) {
        Runtime runtime;
        Check(pal4::PalConsoleCameraCommands::SetAutoseek(
                  input, runtime) == static_cast<std::int32_t>(Runtime::kPlayer) &&
                  runtime.written_address == Runtime::kPlayer + 0xb0cu &&
                  runtime.written_value == expected &&
                  runtime.calls == std::vector<std::string>{
                      "player manager", "player index", "autoseek write"},
              "autoseek player slot and result");
    }
    for (const char* input :
         std::array<const char*, 3>{nullptr, "-1", "2"}) {
        Runtime runtime;
        Check(pal4::PalConsoleCameraCommands::SetAutoseek(input, runtime) ==
                  -7 &&
                  runtime.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
                  runtime.calls == std::vector<std::string>{"console", "output"},
              "autoseek missing or signed out-of-range argument");
    }
    Runtime missing_player;
    missing_player.player_manager = 0u;
    missing_player.player_index = 0u;
    Check(pal4::PalConsoleCameraCommands::SetAutoseek(
              "1", missing_player) == -7 &&
              missing_player.output == "\xce\xde\xd0\xa7\xbd\xc7\xc9\xab" &&
              missing_player.calls == std::vector<std::string>{
                  "player manager", "player index", "console", "output"},
          "null computed player reports original GBK error");
}

void CameraMode() {
    Runtime simple;
    Check(pal4::PalConsoleCameraCommands::SetCameraMode("5 0", simple) ==
              static_cast<std::int32_t>(Runtime::kPlayer) &&
              simple.written_value == 0u &&
              simple.calls == std::vector<std::string>{
                  "camera manager", "set mode 5", "camera manager",
                  "camera state", "camera manager", "camera object",
                  "look-at", "actor control", "flush yaw", "player manager",
                  "player index", "autoseek write"},
          "zero camera state uses look-at and yaw path");

    Runtime internal;
    internal.camera_mode_state = 1u;
    Check(pal4::PalConsoleCameraCommands::SetCameraMode("0 0", internal) ==
              static_cast<std::int32_t>(Runtime::kPlayer) &&
              internal.calls == std::vector<std::string>{
                  "camera manager", "set mode 0", "camera manager",
                  "camera state", "camera manager", "camera id", "position",
                  "player manager", "player index", "autoseek write"},
          "internal camera without follow skips look-at");

    Runtime following;
    following.camera_mode_state = 1u;
    Check(pal4::PalConsoleCameraCommands::SetCameraMode("3 1", following) ==
              static_cast<std::int32_t>(Runtime::kPlayer) &&
              following.written_value == 1u &&
              following.calls == std::vector<std::string>{
                  "camera manager", "set mode 3", "camera manager",
                  "camera state", "camera manager", "camera id", "position",
                  "camera manager", "camera object", "look-at",
                  "player manager", "player index", "autoseek write"},
          "internal camera follow restores look-at");
}

void CameraErrors() {
    for (const char* input :
         std::array<const char*, 7>{nullptr, "", "0", "-1 0", "6 0",
                                    "0 -1", "0 2"}) {
        Runtime runtime;
        Check(pal4::PalConsoleCameraCommands::SetCameraMode(input, runtime) ==
                  -7 &&
                  runtime.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
                  runtime.calls == std::vector<std::string>{"console", "output"},
              "camera parameters rejected before side effects");
    }
    Runtime missing_player;
    missing_player.player_manager = 0u;
    missing_player.player_index = 0u;
    Check(pal4::PalConsoleCameraCommands::SetCameraMode(
              "2 1", missing_player) == -7 &&
              missing_player.output == "\xce\xde\xd0\xa7\xbd\xc7\xc9\xab" &&
              missing_player.calls.back() == "output" &&
              missing_player.calls.front() == "camera manager" &&
              missing_player.calls[2] == "camera manager",
          "camera updates precede missing-player error");
}

void DistanceOptimization() {
    for (const auto [input, expected] :
         {std::pair{"1", 1u}, std::pair{"0", 0u},
          std::pair{"-1", 0u}, std::pair{"2", 0u},
          std::pair{"text", 0u}, std::pair{"", 0u}}) {
        Runtime runtime;
        Check(pal4::PalConsoleCameraCommands::SetDistanceOptionEnabled(
                  input, runtime) == 43 &&
                  runtime.optimization_value == expected &&
                  runtime.calls ==
                      std::vector<std::string>{"distance optimization"},
              "distance optimization enables only atoi value one");
    }
    Runtime missing;
    Check(pal4::PalConsoleCameraCommands::SetDistanceOptionEnabled(
              nullptr, missing) == -7 &&
              missing.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
              missing.calls == std::vector<std::string>{"console", "output"},
          "distance optimization missing argument");
}

}  // namespace

int main() {
    try {
        Autoseek();
        CameraMode();
        CameraErrors();
        DistanceOptimization();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
