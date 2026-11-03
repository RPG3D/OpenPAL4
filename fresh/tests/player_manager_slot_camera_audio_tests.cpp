#include "pal4/player_manager_slot_camera_audio.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PlayerManagerSlotCameraAudioRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::vector<std::uint32_t> cameras{0x2000u, 0x3000u, 0x4000u};
    std::size_t camera_calls{};
    std::uint32_t result{0x9876u};

    void SetBaseProperty(std::uint32_t object,
                         std::uint32_t property) override {
        Check(object == 0x5000u && property == 0x77u,
              "base property arguments");
        events.emplace_back("base-property");
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t CameraManager() override {
        events.emplace_back("manager");
        return 0x1000u;
    }
    std::uint32_t ActiveCameraObject(std::uint32_t manager) override {
        Check(manager == 0x1000u, "active camera object manager");
        events.emplace_back("active-object");
        return 0x1100u;
    }
    void SetLookAt(std::uint32_t camera,
                   std::uint32_t mode) override {
        Check(camera == 0x1100u && mode == 1u,
              "look-at camera and mode");
        events.emplace_back("look-at");
    }
    std::uint32_t ActiveCameraInternal(
        std::uint32_t manager) override {
        Check(manager == 0x1000u, "active internal camera manager");
        events.emplace_back("internal");
        const auto index = camera_calls++;
        return cameras[index];
    }
    std::uint32_t AudioManager() override {
        events.emplace_back("audio-manager");
        return 0x7000u;
    }
    std::uint32_t SetListenerPosition(
        std::uint32_t listener, float x, float y, float z) override {
        Check(listener == 0x7100u && x == 3.0f && y == -4.0f &&
                  z == -5.0f,
              "listener position flips camera Z");
        events.emplace_back("position");
        return 0x1111u;
    }
    std::uint32_t SetListenerOrientation(
        std::uint32_t listener, float x, float y, float z,
        float up_x, float up_y, float up_z) override {
        Check(listener == 0x7100u && x == 1.0f && y == 2.0f &&
                  z == -3.0f && up_x == 0.0f && up_y == 1.0f &&
                  std::bit_cast<std::uint32_t>(up_z) == 0x80000000u,
              "listener orientation flips Z and preserves negative zero");
        events.emplace_back("orientation");
        return result;
    }
};

void Disabled() {
    Runtime runtime;
    Check(pal4::PlayerManagerSlotCameraAudio::Update(
              0x5000u, 0x77u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "base-property", "read:23308"},
          "disabled slot only sets base property and reads +0xb0c");
}

void MissingCamera() {
    Runtime runtime;
    runtime.words[0x5b0cu] = 1u;
    runtime.cameras = {0u};
    Check(pal4::PlayerManagerSlotCameraAudio::Update(
              0x5000u, 0x77u, runtime) == 0u &&
              runtime.camera_calls == 1u &&
              runtime.events == std::vector<std::string>{
                  "base-property", "read:23308", "manager",
                  "active-object", "look-at", "manager", "internal"},
          "missing internal camera exits after look-at");
}

void CompleteAudioUpdate() {
    Runtime runtime;
    runtime.words[0x5b0cu] = 1u;
    runtime.words[0x3074u] = std::bit_cast<std::uint32_t>(3.0f);
    runtime.words[0x3078u] = std::bit_cast<std::uint32_t>(-4.0f);
    runtime.words[0x307cu] = std::bit_cast<std::uint32_t>(5.0f);
    runtime.words[0x4050u] = std::bit_cast<std::uint32_t>(1.0f);
    runtime.words[0x4054u] = std::bit_cast<std::uint32_t>(2.0f);
    runtime.words[0x4058u] = std::bit_cast<std::uint32_t>(3.0f);
    runtime.words[0x701cu] = 0x7100u;
    Check(pal4::PlayerManagerSlotCameraAudio::Update(
              0x5000u, 0x77u, runtime) == 0x9876u,
          "orientation return value is forwarded");
    Check(runtime.camera_calls == 3u &&
              runtime.events == std::vector<std::string>{
                  "base-property", "read:23308", "manager",
                  "active-object", "look-at", "manager", "internal",
                  "manager", "internal", "read:12404", "read:12408",
                  "read:12412", "audio-manager", "read:28700",
                  "position", "manager", "internal", "read:16464",
                  "read:16468", "read:16472", "audio-manager",
                  "read:28700", "orientation"},
          "camera and audio calls follow IDA order with fresh lookups");
}

}  // namespace

int main() {
    Disabled();
    MissingCamera();
    CompleteAudioUpdate();
    std::cout << "3 player-manager camera/audio groups passed\n";
}
