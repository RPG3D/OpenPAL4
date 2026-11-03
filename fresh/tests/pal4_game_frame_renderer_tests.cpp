#include "pal4/pal4_game_frame_renderer.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPal4GameFrameRuntime {
    std::vector<std::string> calls;
    std::uint32_t camera_queries = 0;
    std::uint32_t game_queries = 0;
    std::uint32_t camera_reads = 0;
    std::uint32_t effect_queries = 0;
    std::int32_t mode_result = 1;
    std::int32_t device_result = -37;
    std::uint32_t first_camera = 0x4000;
    std::uint32_t second_camera = 0x5000;
    std::uint32_t first_slot = 0x3000;
    std::uint32_t second_slot = 0x3010;
    std::uint32_t object = 0x2000;
    std::uint32_t updated_object = 0x2000;
    std::uint32_t begun_camera = 0x6000;
    bool texture_overlay = true;
    bool text_overlay = true;
    float scale = 0.5f;

    void Log(const char* call) { calls.emplace_back(call); }
    std::int32_t HandleVideoModeChange() override {
        Log("video mode");
        return mode_result;
    }
    std::uint32_t InitCameraSubsystem() override {
        Log("init cameras");
        return 0x100;
    }
    std::uint32_t ActiveCamera(std::uint32_t subsystem) override {
        Check(subsystem == 0x100, "camera subsystem argument");
        Log("active camera");
        return ++camera_queries == 1 ? first_camera : second_camera;
    }
    std::uint32_t InternalIdSlot(std::uint32_t camera) override {
        Log("id slot");
        Check(camera == first_camera || camera == second_camera,
              "active camera used for ID slot");
        return camera_queries == 1 ? first_slot : second_slot;
    }
    std::uint32_t GameInstance() override {
        Log("game");
        ++game_queries;
        return 0x1000;
    }
    std::uint32_t GameState() override {
        Log("state");
        return 0x9000;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x118c) {
            Log("object word");
            return game_queries == 2 && camera_queries == 1 ?
                       updated_object : object;
        }
        if (address == first_slot) {
            Log("first camera word");
            return 0x4001;
        }
        if (address == second_slot) {
            Log("second camera word");
            return ++camera_reads == 1 ? 0x5001 : 0x5002;
        }
        if (address == 0x9588) {
            Log("texture flag");
            return texture_overlay;
        }
        if (address == 0x9554) {
            Log("text flag");
            return text_overlay;
        }
        if (address == 0x1184) {
            Log("texture word");
            return 0x7000;
        }
        throw std::runtime_error("unexpected word address");
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == 0x957c, "game speed offset");
        Log("speed");
        return scale;
    }
    std::uint32_t AudioManager() override {
        Log("audio manager");
        return 0xa000;
    }
    void UpdateAudio(std::uint32_t manager, float elapsed) override {
        Check(manager == 0xa000 && elapsed == 1.5f,
              "audio manager and scaled time");
        Log("audio update");
    }
    void UpdateGameObject(std::uint32_t value, float elapsed,
                          std::uint32_t camera_id,
                          std::uint32_t override_camera) override {
        Check(value == object && elapsed == 1.5f && camera_id == 0x4001,
              "object update arguments");
        Check(override_camera == 0 || override_camera == 0x7777,
              "object update camera override");
        Log("object update");
    }
    void SetRenderOption(std::uint32_t option,
                         std::uint32_t value) override {
        Check((option == 20 && value == 2) ||
                  (option == 6 && value == 1) ||
                  (option == 8 && value == 1), "render option values");
        calls.emplace_back("option " + std::to_string(option));
    }
    void RenderGameObject(std::uint32_t value,
                          std::uint32_t slot) override {
        Check(value == object && slot == second_slot,
              "render object and second camera slot");
        Log("render object");
    }
    void CameraClear(std::uint32_t camera, std::uint32_t color,
                     std::uint32_t flags) override {
        Check((camera == 0x5001 || camera == 0x7777) &&
                  color == 0x907928 && flags == 3,
              "clear camera, color and flags");
        Log("clear camera");
    }
    std::uint32_t CameraBeginUpdate(std::uint32_t camera) override {
        Check(camera == (camera_reads ? 0x5002 : 0x7777),
              "begin update rereads slot without override");
        Log("begin camera");
        return begun_camera;
    }
    void CameraEndUpdate(std::uint32_t camera) override {
        Check(camera == begun_camera, "end updated camera");
        Log("end camera");
    }
    void ShowGameRaster(std::uint32_t camera) override {
        Check(camera == begun_camera, "show updated camera");
        Log("show raster");
    }
    std::uint32_t RenderSystem() override {
        Log("render system");
        return 0x8000;
    }
    void ClearRenderCollection(std::uint32_t system) override {
        Check(system == 0x8000, "render collection system");
        Log("clear collection");
    }
    void DrawTexturedRectangle(
        std::uint32_t slot, std::uint32_t texture,
        std::uint32_t x, std::uint32_t y, std::uint32_t width,
        std::uint32_t height,
        const std::array<std::uint8_t, 4>& color) override {
        Check(slot == second_slot && texture == 0x7000 &&
                  x == 544 && y == 400 && width == 256 && height == 200 &&
                  color == std::array<std::uint8_t, 4>{255, 255, 255, 255},
              "texture overlay rectangle");
        Log("draw texture");
    }
    void DrawTextCollection(std::uint32_t system,
                            std::uint32_t slot) override {
        Check(system == 0x8000 && slot == second_slot,
              "text overlay arguments");
        Log("draw text");
    }
    std::uint32_t FrameCallback() override {
        Log("frame callback");
        return 0x5c7f30;
    }
    void ApplyTextureProperties(std::uint32_t texture_object,
                                float elapsed,
                                std::uint32_t callback) override {
        Check(texture_object == 0x1178 && elapsed == 3.0f &&
                  callback == 0x5c7f30, "texture postprocess arguments");
        Log("apply texture");
    }
    std::int32_t HandleDeviceLoss() override {
        Log("device loss");
        return device_result;
    }
    std::uint32_t EffectManager() override {
        Log("effect manager");
        return 0xb000 + ++effect_queries * 0x100;
    }
    void UpdateRenderParams(std::uint32_t manager,
                            double elapsed) override {
        Check(manager == 0xb100 && elapsed == 3.0,
              "effect parameters and elapsed time");
        Log("effect params");
    }
    std::uint32_t CameraContainer(std::uint32_t camera) override {
        Check(camera == second_camera, "second camera container");
        Log("camera container");
        return 0xc000;
    }
    void PrepareRenderFrame(std::uint32_t frame,
                            std::uint32_t container) override {
        Check(frame == 0xb304 && container == 0xc000,
              "prepare frame from second effect manager");
        Log("prepare frame");
    }
    void EndRenderFrame(std::uint32_t frame) override {
        Check(frame == 0xb404, "end frame from third effect manager");
        Log("end frame");
    }
    void RenderScene(std::uint32_t frame) override {
        Check(frame == 0xb504, "scene frame from fourth effect manager");
        Log("render scene");
    }
    void RenderEffectPasses(std::uint32_t manager,
                            std::uint32_t camera) override {
        Check(manager == 0xb500 && camera == begun_camera,
              "effect passes from fifth manager");
        Log("effect passes");
    }
};

bool Has(const Runtime& runtime, const char* call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

void InOrder(const Runtime& runtime,
             std::initializer_list<const char*> calls) {
    auto it = runtime.calls.begin();
    for (const auto* call : calls) {
        it = std::find(it, runtime.calls.end(), call);
        Check(it != runtime.calls.end(), "required call order");
        ++it;
    }
}

void TestNormalFrame() {
    Runtime runtime;
    Check(pal4::Pal4GameFrameRenderer::Render(3.0, 0, runtime) == -37,
          "normal frame returns device-loss result");
    InOrder(runtime, {"video mode", "init cameras", "active camera",
                      "id slot", "game", "object word", "audio manager",
                      "state", "speed", "audio update", "state",
                      "first camera word", "speed", "object update", "game",
                      "object word", "init cameras", "active camera",
                      "id slot", "second camera word", "clear camera",
                      "second camera word", "begin camera", "option 20",
                      "option 6", "option 8", "render object",
                      "draw texture", "draw text", "end camera",
                      "show raster", "clear collection", "apply texture",
                      "device loss"});
    Check(runtime.camera_reads == 2 && runtime.game_queries == 4,
          "normal frame rereads camera and game instance");
}

void TestEarlyExits() {
    Runtime runtime;
    runtime.mode_result = 0;
    Check(pal4::Pal4GameFrameRenderer::Render(3.0, 0, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"video mode"},
          "video-mode rejection exits immediately");
    runtime = Runtime{};
    runtime.first_camera = 0;
    Check(pal4::Pal4GameFrameRenderer::Render(3.0, 0, runtime) == 0 &&
              !Has(runtime, "game"), "missing first camera exits");
    runtime = Runtime{};
    runtime.object = 0;
    Check(pal4::Pal4GameFrameRenderer::Render(3.0, 0, runtime) == 0x1000 &&
              !Has(runtime, "audio manager"), "missing object returns game");
    runtime = Runtime{};
    runtime.updated_object = 0;
    Check(pal4::Pal4GameFrameRenderer::Render(3.0, 0, runtime) == 0x1000 &&
              !Has(runtime, "clear camera"),
          "normal frame stops when object changes during update");
    runtime = Runtime{};
    runtime.second_camera = 0;
    Check(pal4::Pal4GameFrameRenderer::Render(3.0, 0, runtime) == 0 &&
              !Has(runtime, "device loss"), "missing second camera exits");
}

void TestNormalBranches() {
    Runtime runtime;
    runtime.second_slot = 0;
    Check(pal4::Pal4GameFrameRenderer::Render(3.0, 0, runtime) == -37 &&
              !Has(runtime, "clear camera") && Has(runtime, "apply texture"),
          "null second ID still postprocesses texture");
    runtime = Runtime{};
    runtime.begun_camera = 0;
    Check(pal4::Pal4GameFrameRenderer::Render(3.0, 0, runtime) == -37 &&
              !Has(runtime, "render object") &&
              Has(runtime, "apply texture"),
          "begin failure still postprocesses texture");
    runtime = Runtime{};
    Check(pal4::Pal4GameFrameRenderer::Render(3.0, 0x7777, runtime) == -37 &&
              runtime.camera_reads == 0 && !Has(runtime, "texture flag") &&
              !Has(runtime, "draw text"),
          "override camera suppresses overlays and slot reads");
    runtime = Runtime{};
    runtime.texture_overlay = false;
    runtime.text_overlay = false;
    Check(pal4::Pal4GameFrameRenderer::Render(3.0, 0, runtime) == -37 &&
              !Has(runtime, "draw texture") &&
              !Has(runtime, "draw text") &&
              !Has(runtime, "apply texture"),
          "disabled overlay flags skip drawing and postprocessing");
}

void TestEffectFrame() {
    Runtime runtime;
    Check(pal4::Pal4GameFrameRenderer::RenderWithEffects(3.0, 0, runtime) ==
              -37, "effect frame returns device-loss result");
    InOrder(runtime, {"object update", "init cameras", "active camera",
                      "id slot", "effect manager", "effect params",
                      "effect manager", "camera container",
                      "prepare frame", "option 20", "option 6",
                      "option 8", "render object", "draw texture",
                      "draw text", "effect manager", "end frame",
                      "effect manager", "render scene",
                      "second camera word", "clear camera",
                      "second camera word", "begin camera",
                      "effect manager", "effect passes", "end camera",
                      "show raster", "clear collection", "apply texture",
                      "device loss"});
    Check(runtime.effect_queries == 5 && runtime.camera_reads == 2 &&
              runtime.game_queries == 3,
          "effect frame independently reacquires effect manager");
}

void TestEffectBranches() {
    Runtime runtime;
    runtime.updated_object = 0;
    Check(pal4::Pal4GameFrameRenderer::RenderWithEffects(3.0, 0, runtime) ==
              -37 && Has(runtime, "render object"),
          "effect frame does not recheck updated object");
    runtime = Runtime{};
    runtime.second_slot = 0;
    Check(pal4::Pal4GameFrameRenderer::RenderWithEffects(3.0, 0, runtime) ==
              -37 && runtime.effect_queries == 1 &&
              Has(runtime, "effect params") &&
              !Has(runtime, "prepare frame") &&
              Has(runtime, "apply texture"),
          "effect parameters update with null second ID");
    runtime = Runtime{};
    runtime.begun_camera = 0;
    Check(pal4::Pal4GameFrameRenderer::RenderWithEffects(3.0, 0, runtime) ==
              -37 && runtime.effect_queries == 4 &&
              Has(runtime, "render scene") &&
              !Has(runtime, "effect passes") &&
              Has(runtime, "apply texture"),
          "effect scene renders before failed camera begin");
    runtime = Runtime{};
    Check(pal4::Pal4GameFrameRenderer::RenderWithEffects(3.0, 0x7777,
                                                         runtime) == -37 &&
              runtime.camera_reads == 0 &&
              !Has(runtime, "draw texture") &&
              !Has(runtime, "apply texture") &&
              runtime.effect_queries == 5,
          "effect override skips overlays but renders effect passes");
    runtime = Runtime{};
    runtime.second_camera = 0;
    Check(pal4::Pal4GameFrameRenderer::RenderWithEffects(3.0, 0,
                                                         runtime) == 0 &&
              runtime.effect_queries == 0 &&
              !Has(runtime, "device loss"),
          "missing second camera exits before effect update");
}

}  // namespace

int main() {
    try {
        TestNormalFrame();
        TestEarlyExits();
        TestNormalBranches();
        TestEffectFrame();
        TestEffectBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
