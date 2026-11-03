#include "pal4/script_camera_callbacks.h"
#include "pal4/script_builtin_handlers.h"

#include "pal4/player_manager_object.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptCameraCallbackRuntime {
    void AdjustYpr(float, float, float, std::uint8_t) override {
        throw std::logic_error("unexpected camera angle control");
    }
    bool main_camera = true;
    bool script_camera = true;
    bool active_camera = true;
    float pitch = -17.25f;
    std::uint32_t manager_calls = 0u;
    std::uint32_t audio_calls = 0u;
    pal4::PlayerManagerObjectState player_state;
    std::vector<std::string> calls;

    std::uint32_t CameraSubsystem() override {
        calls.emplace_back("camera-manager");
        return 0x1000u + manager_calls++ * 0x1000u;
    }
    std::uint32_t CameraByName(std::uint32_t manager,
                               std::string_view name) override {
        Check((manager == 0x1000u && name == "MainCamera") ||
              (manager == 0x2000u && name == "ScriptCamera"),
              "two camera singleton reads and names");
        calls.emplace_back("by-name:" + std::string(name));
        if (name == "MainCamera") return main_camera ? 0x3000u : 0u;
        return script_camera ? 0x4000u : 0u;
    }
    std::uint32_t CameraInternalId(std::uint32_t camera) override {
        Check(camera == 0x3000u || camera == 0x4000u,
              "named camera internal ID operand");
        calls.emplace_back("internal:" + std::to_string(camera));
        return camera + 0x100u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check((address == 0x3000u + 0x100u + 0x1a8u ||
               address == 0x4000u + 0x100u + 0x1a8u) && value <= 1u,
              "camera collision field +0x1a8");
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    std::uint32_t ActiveCameraObject(std::uint32_t manager) override {
        Check(manager == 0x1000u, "active camera manager operand");
        calls.emplace_back("active");
        return active_camera ? 0x9000u : 0u;
    }
    void SetLookAt(std::uint32_t object,
                   std::uint32_t option) override {
        Check(object == 0x9000u && option == 1u,
              "look-at refresh before listener sync");
        calls.emplace_back("look-at");
    }
    std::uint32_t ActiveCameraInternalId(
        std::uint32_t manager) override {
        Check(manager == 0x1000u || manager == 0x2000u || manager == 0x3000u,
              "reacquired camera manager for each vector");
        calls.emplace_back("active-id");
        return 0xa000u;
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("float:" + std::to_string(address));
        switch (address) {
        case 0xa040u: return pitch;
        case 0xa074u: return 1.25f;
        case 0xa078u: return -2.5f;
        case 0xa07cu: return 3.5f;
        case 0xa050u: return -4.0f;
        case 0xa054u: return 5.0f;
        case 0xa058u: return 0.0f;
        default: throw std::runtime_error("unexpected camera float address");
        }
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio");
        return 0xb000u + audio_calls++ * 0x1000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0xb01cu || address == 0xc01cu,
              "listener handle at audio manager +0x1c");
        calls.emplace_back("audio-handle");
        return address == 0xb01cu ? 0xd000u : 0xe000u;
    }
    void SetListenerPosition(std::uint32_t handle, float x,
                             float y, float z) override {
        Check(handle == 0xd000u && x == 1.25f && y == -2.5f &&
              z == -3.5f, "listener position flips camera Z");
        calls.emplace_back("position");
    }
    std::uint32_t SetListenerOrientation(
        std::uint32_t handle, float at_x, float at_y, float at_z,
        float up_x, float up_y, float up_z) override {
        Check(handle == 0xe000u && at_x == -4.0f && at_y == 5.0f &&
              std::bit_cast<std::uint32_t>(at_z) == 0x80000000u &&
              std::bit_cast<std::uint32_t>(up_x) == 0u &&
              up_y == 1.0f &&
              std::bit_cast<std::uint32_t>(up_z) == 0x80000000u,
              "listener orientation flips Z and preserves signed zero");
        calls.emplace_back("orientation");
        return 0xbeefu;
    }
    pal4::PlayerManagerObjectState& PlayerManagerState() override {
        calls.emplace_back("player-manager");
        return player_state;
    }
};

void CollisionBranches() {
    for (const std::uint8_t input : {0u, 1u, 2u, 255u}) {
        Runtime runtime;
        pal4::ScriptCameraCallbacks::SetCollision(input, runtime);
        const auto enabled = input == 1u ? "1" : "0";
        Check(runtime.calls == std::vector<std::string>{
                  "camera-manager", "by-name:MainCamera",
                  "internal:12288", "write:" +
                      std::to_string(0x3000u + 0x100u + 0x1a8u) + ":" +
                      std::string(enabled),
                  "camera-manager", "by-name:ScriptCamera",
                  "internal:16384", "write:" +
                      std::to_string(0x4000u + 0x100u + 0x1a8u) + ":" +
                      std::string(enabled)},
              "collision accepts exactly byte one and visits both cameras");
    }
    Runtime missing;
    missing.main_camera = false;
    missing.script_camera = false;
    pal4::ScriptCameraCallbacks::SetCollision(1u, missing);
    Check(missing.calls == std::vector<std::string>{
              "camera-manager", "by-name:MainCamera", "camera-manager",
              "by-name:ScriptCamera"},
          "missing cameras skip internal IDs and writes");
}

void PitchQuery() {
    for (const auto pitch : {-17.25f, -0.0f, 0.0f, 359.5f}) {
        Runtime runtime;
        runtime.pitch = pitch;
        Check(std::bit_cast<std::uint32_t>(static_cast<float>(pal4::ScriptCameraHandler::Pitch(runtime))) ==
                  std::bit_cast<std::uint32_t>(pitch) &&
                  runtime.calls == std::vector<std::string>{"camera-manager", "active-id", "float:41024"},
              "pitch reads current camera +64 once without normalization or signed-zero loss");
    }
}

void ListenerSync() {
    Runtime absent;
    absent.active_camera = false;
    Check(pal4::ScriptCameraCallbacks::SeekToPlayer(absent) == 0u &&
          absent.calls == std::vector<std::string>{
              "camera-manager", "active"},
          "missing active camera is the only early gate");

    Runtime runtime;
    Check(pal4::ScriptCameraCallbacks::SeekToPlayer(runtime) == 0xbeefu &&
          runtime.manager_calls == 3u && runtime.audio_calls == 2u,
          "listener orientation result and singleton counts");
    Check(runtime.calls == std::vector<std::string>{
              "camera-manager", "active", "look-at",
              "camera-manager", "active-id", "float:41076",
              "float:41080", "float:41084", "audio", "audio-handle",
              "position", "camera-manager", "active-id", "float:41040",
              "float:41044", "float:41048", "audio", "audio-handle",
              "orientation"},
          "camera position and direction sampled separately in IDA order");
}

void AutoSeekByte() {
    for (const auto input : {0u, 1u, 0x1234u, 0xffffffffu}) {
        Runtime runtime;
        runtime.player_state.Word(0x2c50u / 4u) = 2u;
        for (std::uint32_t i = 0u; i < 4u; ++i)
            runtime.player_state.Word((i * 0xb14u + 0xb0cu) / 4u) = 0xdeadu;
        Check(pal4::ScriptCameraCallbacks::AutoSeek(input, runtime) ==
                  2u * (0xb14u / 4u) &&
              runtime.calls == std::vector<std::string>{"player-manager"},
              "script auto-seek forwards masked byte to player-state setter");
        for (std::uint32_t i = 0u; i < 4u; ++i)
            Check(runtime.player_state.Word((i * 0xb14u + 0xb0cu) / 4u) ==
                      (i == 2u ? (input & 0xffu) : 0u),
                  "state setter clears four slots and selects active slot");
    }
}

}  // namespace

int main() {
    try {
        CollisionBranches();
        PitchQuery();
        ListenerSync();
        AutoSeekByte();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
