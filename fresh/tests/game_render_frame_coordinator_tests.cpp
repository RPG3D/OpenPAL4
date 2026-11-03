#include "pal4/game_render_frame_coordinator.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kCameraManager = 0x1000u;
constexpr std::uint32_t kGame = 0x2000u;
constexpr std::uint32_t kState = 0x3000u;
constexpr std::uint32_t kFirstCamera = 0x4000u;
constexpr std::uint32_t kSecondCamera = 0x5000u;
constexpr std::uint32_t kFirstSlot = 0x6000u;
constexpr std::uint32_t kSecondSlot = 0x6004u;
constexpr std::uint32_t kObject = 0x7000u;
constexpr std::uint32_t kCombat = 0x8000u;
constexpr std::uint32_t kCombatCamera = 0x9000u;
constexpr std::uint32_t kAudio = 0xa000u;
constexpr std::uint32_t kRender = 0xb000u;
constexpr std::uint32_t kContainer = 0xc000u;
constexpr std::uint32_t kTexture = 0xd000u;

struct Runtime final : pal4::IGameFrameRuntime {
    std::vector<std::string> trace;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> options;
    std::uint32_t camera_calls = 0u;
    std::uint32_t game_calls = 0u;
    std::uint32_t audio_updates = 0u;
    std::uint32_t object_updates = 0u;
    std::uint32_t render_params_updates = 0u;
    std::uint32_t cleared_camera = 0u;
    int video_mode_result = 1;
    std::uint32_t first_camera = kFirstCamera;
    std::uint32_t second_camera = kSecondCamera;
    std::uint32_t second_slot = kSecondSlot;
    std::uint32_t game_object = kObject;
    std::uint32_t begin_result = 0xe000u;
    bool texture_overlay = false;
    bool text_overlay = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == kGame + pal4::GameRenderFrameCoordinator::kGameObjectOffset)
            return game_object;
        if (address == kFirstSlot) return 0x4444u;
        if (address == kSecondSlot) return 0x5555u;
        if (address == kState + pal4::GameRenderFrameCoordinator::kTextureOverlayOffset)
            return texture_overlay ? 1u : 0u;
        if (address == kState + pal4::GameRenderFrameCoordinator::kTextOverlayOffset)
            return text_overlay ? 1u : 0u;
        if (address == kGame + pal4::GameRenderFrameCoordinator::kTextureOffset)
            return kTexture;
        throw std::runtime_error("unexpected word read");
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == kState + pal4::GameRenderFrameCoordinator::kScaleOffset,
              "game time scale address");
        trace.push_back("read-scale");
        return 1.5f;
    }
    int HandleVideoModeChange() override {
        trace.push_back("video-mode");
        return video_mode_result;
    }
    std::uint32_t InitCameraSubsystem() override {
        trace.push_back("camera-manager");
        return kCameraManager;
    }
    std::uint32_t ActiveCamera(std::uint32_t manager) override {
        Check(manager == kCameraManager, "camera manager pointer");
        trace.push_back("active-camera");
        return camera_calls++ == 0u ? first_camera : second_camera;
    }
    std::uint32_t InternalIdSlot(std::uint32_t camera) override {
        trace.push_back("internal-id");
        if (camera == kFirstCamera) return kFirstSlot;
        Check(camera == kSecondCamera, "second camera pointer");
        return second_slot;
    }
    std::uint32_t GameInstance() override {
        trace.push_back("game-instance");
        ++game_calls;
        return kGame;
    }
    std::uint32_t GameState() override {
        trace.push_back("game-state");
        return kState;
    }
    std::uint32_t CombatSystem() override {
        trace.push_back("combat-system");
        return kCombat;
    }
    std::uint32_t CombatCamera(std::uint32_t combat) override {
        Check(combat == kCombat, "combat pointer");
        trace.push_back("combat-camera");
        return kCombatCamera;
    }
    std::uint32_t AudioManager() override {
        trace.push_back("audio-manager");
        return kAudio;
    }
    void UpdateAudio(std::uint32_t audio, float elapsed) override {
        Check(audio == kAudio && elapsed == 3.0f,
              "scaled audio elapsed time");
        trace.push_back("update-audio");
        ++audio_updates;
    }
    void UpdateGameObject(std::uint32_t object, float elapsed,
                          std::uint32_t camera_id,
                          std::uint32_t override_camera) override {
        Check(object == kObject && elapsed == 3.0f &&
                  camera_id == 0x4444u &&
                  (override_camera == 0u || override_camera == 0x7777u),
              "game object update arguments");
        trace.push_back("update-object");
        ++object_updates;
    }
    void UpdateRenderParams(std::uint32_t camera, double elapsed) override {
        Check(camera == kCombatCamera && elapsed == 2.0,
              "unscaled render elapsed time");
        trace.push_back("render-params");
        ++render_params_updates;
    }
    std::uint32_t CameraContainer(std::uint32_t camera) override {
        Check(camera == kSecondCamera, "second camera container owner");
        trace.push_back("camera-container");
        return kContainer;
    }
    void PrepareRenderFrame(std::uint32_t frame,
                            std::uint32_t container) override {
        Check(frame == kCombatCamera + 260u && container == kContainer,
              "render frame address and container");
        trace.push_back("prepare-frame");
    }
    void SetRenderOption(std::uint32_t option,
                         std::uint32_t value) override {
        trace.push_back("render-option");
        options.emplace_back(option, value);
    }
    void RenderGameObject(std::uint32_t object,
                          std::uint32_t camera_id_slot) override {
        Check(object == kObject && camera_id_slot == kSecondSlot,
              "game object render uses second id slot");
        trace.push_back("render-object");
    }
    void DrawTexturedRectangle(
        std::uint32_t camera_id_slot, std::uint32_t texture,
        std::uint32_t x, std::uint32_t y,
        std::uint32_t width, std::uint32_t height,
        const std::array<std::uint8_t, 4>& color) override {
        Check(camera_id_slot == kSecondSlot && texture == kTexture &&
                  x == 544u && y == 400u && width == 256u &&
                  height == 200u &&
                  color == std::array<std::uint8_t, 4>{255u, 255u, 255u, 255u},
              "texture overlay geometry and white color");
        trace.push_back("draw-texture");
    }
    std::uint32_t RenderSystem() override {
        trace.push_back("render-system");
        return kRender;
    }
    void DrawTextCollection(std::uint32_t render_system,
                            std::uint32_t camera_id_slot) override {
        Check(render_system == kRender && camera_id_slot == kSecondSlot,
              "text collection render arguments");
        trace.push_back("draw-text");
    }
    void EndRenderFrame(std::uint32_t frame) override {
        Check(frame == kCombatCamera + 260u, "end frame address");
        trace.push_back("end-frame");
    }
    void RenderScene(std::uint32_t frame) override {
        Check(frame == kCombatCamera + 260u, "scene frame address");
        trace.push_back("render-scene");
    }
    void CameraClear(std::uint32_t camera, std::uint32_t color,
                     std::uint32_t flags) override {
        Check(color == pal4::GameRenderFrameCoordinator::kClearColor && flags == 3u,
              "RenderWare clear color/flags");
        cleared_camera = camera;
        trace.push_back("camera-clear");
    }
    std::uint32_t CameraBeginUpdate(std::uint32_t camera) override {
        Check(camera == cleared_camera, "begin update follows clear target");
        trace.push_back("camera-begin");
        return begin_result;
    }
    void RenderPasses(std::uint32_t combat_camera,
                      std::uint32_t camera) override {
        Check(combat_camera == kCombatCamera && camera == begin_result,
              "effect render pass camera");
        trace.push_back("render-passes");
    }
    void CameraEndUpdate(std::uint32_t camera) override {
        Check(camera == begin_result, "RenderWare end camera");
        trace.push_back("camera-end");
    }
    void ShowGameRaster(std::uint32_t camera) override {
        Check(camera == begin_result, "shown game raster camera");
        trace.push_back("show-raster");
    }
    void ClearRenderCollection(std::uint32_t render_system) override {
        Check(render_system == kRender, "render collection system");
        trace.push_back("clear-collection");
    }
    std::uint32_t TexturePropertyGlobal() override {
        trace.push_back("texture-global");
        return 0xf000u;
    }
    void ApplyTextureProperties(std::uint32_t property,
                                float elapsed,
                                std::uint32_t global) override {
        Check(property == kGame + 376u && elapsed == 2.0f &&
                  global == 0xf000u,
              "texture postprocess arguments");
        trace.push_back("texture-properties");
    }
    int HandleDeviceLoss() override {
        trace.push_back("device-loss");
        return -27;
    }
};

bool Has(const Runtime& runtime, const std::string& name) {
    return std::find(runtime.trace.begin(), runtime.trace.end(), name) !=
           runtime.trace.end();
}

std::size_t Position(const Runtime& runtime, const std::string& name) {
    const auto found = std::find(runtime.trace.begin(), runtime.trace.end(), name);
    Check(found != runtime.trace.end(), "required frame stage missing");
    return static_cast<std::size_t>(found - runtime.trace.begin());
}

void EarlyGates() {
    Runtime mode;
    mode.video_mode_result = 0;
    Check(pal4::GameRenderFrameCoordinator::Update(2.0, 0u, mode) == 0 &&
              mode.trace == std::vector<std::string>{"video-mode"},
          "video mode gate stops frame before camera lookup");

    Runtime no_camera;
    no_camera.first_camera = 0u;
    Check(pal4::GameRenderFrameCoordinator::Update(2.0, 0u, no_camera) == 0 &&
              no_camera.game_calls == 0u &&
              no_camera.camera_calls == 1u &&
              !Has(no_camera, "device-loss"),
          "first missing camera exits before game lookup");

    Runtime no_object;
    no_object.game_object = 0u;
    Check(pal4::GameRenderFrameCoordinator::Update(2.0, 0u, no_object) ==
              static_cast<int>(kGame) &&
              no_object.audio_updates == 0u &&
              no_object.camera_calls == 1u &&
              !Has(no_object, "device-loss"),
          "missing game object returns game pointer before sound update");

    Runtime no_second_camera;
    no_second_camera.second_camera = 0u;
    Check(pal4::GameRenderFrameCoordinator::Update(2.0, 0u, no_second_camera) == 0 &&
              no_second_camera.audio_updates == 1u &&
              no_second_camera.object_updates == 1u &&
              no_second_camera.render_params_updates == 0u &&
              !Has(no_second_camera, "device-loss"),
          "second missing camera exits after simulation update");
}

void CompleteFrame() {
    Runtime runtime;
    runtime.texture_overlay = true;
    runtime.text_overlay = true;
    Check(pal4::GameRenderFrameCoordinator::Update(2.0, 0u, runtime) == -27 &&
              runtime.camera_calls == 2u && runtime.game_calls == 3u &&
              runtime.audio_updates == 1u && runtime.object_updates == 1u &&
              runtime.render_params_updates == 1u &&
              runtime.cleared_camera == 0x5555u &&
              runtime.options ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {20u, 2u}, {6u, 1u}, {8u, 1u}} &&
              Position(runtime, "update-audio") <
                  Position(runtime, "update-object") &&
              Position(runtime, "update-object") <
                  Position(runtime, "render-params") &&
              Position(runtime, "prepare-frame") <
                  Position(runtime, "render-object") &&
              Position(runtime, "render-object") <
                  Position(runtime, "draw-texture") &&
              Position(runtime, "draw-texture") <
                  Position(runtime, "draw-text") &&
              Position(runtime, "draw-text") <
                  Position(runtime, "end-frame") &&
              Position(runtime, "end-frame") <
                  Position(runtime, "render-scene") &&
              Position(runtime, "render-scene") <
                  Position(runtime, "camera-clear") &&
              Position(runtime, "camera-begin") <
                  Position(runtime, "render-passes") &&
              Position(runtime, "render-passes") <
                  Position(runtime, "camera-end") &&
              Position(runtime, "camera-end") <
                  Position(runtime, "show-raster") &&
              Position(runtime, "show-raster") <
                  Position(runtime, "clear-collection") &&
              Position(runtime, "clear-collection") <
                  Position(runtime, "texture-properties") &&
              runtime.trace.back() == "device-loss",
          "complete frame preserves simulation, render and postprocess order");
}

void OverrideAndRenderFailures() {
    Runtime override_frame;
    override_frame.texture_overlay = true;
    override_frame.text_overlay = true;
    Check(pal4::GameRenderFrameCoordinator::Update(2.0, 0x7777u,
                                         override_frame) == -27 &&
              override_frame.cleared_camera == 0x7777u &&
              !Has(override_frame, "draw-texture") &&
              !Has(override_frame, "draw-text") &&
              !Has(override_frame, "texture-properties"),
          "override camera suppresses both overlays and postprocess");

    Runtime no_slot;
    no_slot.second_slot = 0u;
    no_slot.texture_overlay = true;
    Check(pal4::GameRenderFrameCoordinator::Update(2.0, 0u, no_slot) == -27 &&
              no_slot.render_params_updates == 1u &&
              !Has(no_slot, "prepare-frame") &&
              !Has(no_slot, "camera-clear") &&
              Has(no_slot, "texture-properties") &&
              no_slot.trace.back() == "device-loss",
          "missing second ID slot skips scene render but keeps postprocess");

    Runtime begin_failure;
    begin_failure.begin_result = 0u;
    begin_failure.texture_overlay = true;
    Check(pal4::GameRenderFrameCoordinator::Update(2.0, 0u,
                                         begin_failure) == -27 &&
              Has(begin_failure, "camera-begin") &&
              !Has(begin_failure, "render-passes") &&
              !Has(begin_failure, "camera-end") &&
              !Has(begin_failure, "clear-collection") &&
              Has(begin_failure, "texture-properties") &&
              begin_failure.trace.back() == "device-loss",
          "failed RenderWare begin skips passes while completing frame");
}

}  // namespace

int main() {
    try {
        EarlyGates();
        CompleteFrame();
        OverrideAndRenderFailures();
        std::cout << "game_frame_updater_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
