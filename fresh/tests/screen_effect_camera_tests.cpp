#include "pal4/screen_effect_camera.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScreenEffectCameraRuntime {
    std::int32_t ScreenWidth() override {
        events.emplace_back("width");
        return 800;
    }
    std::int32_t ScreenHeight() override {
        events.emplace_back("height");
        return 600;
    }
    std::uint32_t CreateRaster(
        std::int32_t width, std::int32_t height,
        std::uint32_t depth, std::uint32_t flags) override {
        Check(width == 800 && height == 600 && depth == 0u,
              "raster uses game dimensions and zero depth");
        events.push_back(flags == 5u ? "color raster" : "depth raster");
        Check(flags == 5u || flags == 1u, "two observed raster flags");
        return flags == 5u ? color_result : depth_result;
    }
    void DestroyRaster(std::uint32_t raster) override {
        Check(raster == 0x1001u || raster == 0x1002u,
              "destroy allocated raster");
        events.push_back(raster == 0x1001u ? "destroy color" :
                                                  "destroy depth");
    }
    std::uint32_t CreateCamera() override {
        events.emplace_back("create camera");
        return 0x2000u;
    }
    void SetCameraRaster(std::uint32_t camera, std::uint32_t offset,
                         std::uint32_t raster) override {
        Check(camera == 0x2000u, "set raster on effect camera");
        if (offset == 96u) {
            camera_color = raster;
            events.push_back(raster == 0u ? "clear color slot" :
                                             "set color slot");
        } else {
            Check(offset == 100u, "depth raster slot +100");
            camera_depth = raster;
            events.push_back(raster == 0u ? "clear depth slot" :
                                             "set depth slot");
        }
    }
    std::uint32_t CameraRaster(std::uint32_t camera,
                                std::uint32_t offset) override {
        Check(camera == 0x2000u, "read effect camera raster");
        events.push_back(offset == 96u ? "read color slot" :
                                         "read depth slot");
        Check(offset == 96u || offset == 100u, "raster read slot");
        return offset == 96u ? camera_color : camera_depth;
    }
    std::uint32_t CreateFrame() override {
        events.emplace_back("create frame");
        return 0x3000u;
    }
    void AttachFrame(std::uint32_t camera, std::uint32_t frame) override {
        Check(camera == 0x2000u && frame == 0x3000u,
              "frame attachment");
        events.emplace_back("attach frame");
    }
    float GameNearClip() override {
        events.emplace_back("near value");
        return 1.0f;
    }
    float GameFarClip() override {
        events.emplace_back("far value");
        return 1000.0f;
    }
    void SetNearClip(std::uint32_t camera, float value) override {
        Check(camera == 0x2000u && value == 1.0f, "near clip");
        events.emplace_back("set near");
    }
    void SetFarClip(std::uint32_t camera, float value) override {
        Check(camera == 0x2000u && value == 1000.0f, "far clip");
        events.emplace_back("set far");
    }
    std::array<float, 2> ActiveViewWindow() override {
        events.emplace_back("view values");
        return {0.5f, 0.75f};
    }
    void SetViewWindow(std::uint32_t camera,
                       const std::array<float, 2>& window) override {
        Check(camera == 0x2000u && window[0] == 0.5f &&
                  window[1] == 0.75f, "active view window");
        events.emplace_back("set view");
    }
    std::uint32_t CreateTexture(std::uint32_t raster) override {
        Check(raster == 0x1001u, "texture wraps color raster");
        events.emplace_back("create texture");
        return 0x4000u;
    }
    void ClearCamera(std::uint32_t camera, const std::uint32_t& color,
                     std::uint32_t flags) override {
        Check(camera == 0x2000u && color == 0xff000000u && flags == 3u,
              "camera clears opaque black with flags 3");
        events.emplace_back("clear camera");
    }
    std::uint32_t EndUpdate(std::uint32_t camera) override {
        Check(camera == 0x2000u, "end update camera");
        events.emplace_back("end update");
        return 0x1234u;
    }
    std::uint32_t ActiveGameCamera() override {
        events.emplace_back("active camera");
        return active_camera;
    }
    std::uint32_t WorldForCamera(std::uint32_t camera) override {
        Check(camera == 0x2000u || camera == 0x3001u,
              "world camera lookup");
        events.push_back(camera == 0x2000u ? "effect world" :
                                              "game world");
        return camera == 0x2000u ? effect_world : game_world;
    }
    void RemoveCamera(std::uint32_t world,
                      std::uint32_t camera) override {
        Check(world == effect_world && camera == 0x2000u,
              "remove effect camera from old world");
        events.emplace_back("remove camera");
    }
    void AddCamera(std::uint32_t world,
                   std::uint32_t camera) override {
        Check(world == game_world && camera == 0x2000u,
              "add effect camera to active world");
        events.emplace_back("add camera");
    }
    void DestroyTexture(std::uint32_t texture) override {
        Check(texture == 0x4000u, "destroy camera texture");
        events.emplace_back("destroy texture");
    }
    void DestroyCamera(std::uint32_t camera) override {
        Check(camera == 0x2000u, "destroy effect camera");
        events.emplace_back("destroy camera");
    }
    pal4::ScreenEffectCameraRecord* AllocateRecord(
        std::uint32_t bytes) override {
        Check(bytes == 24u, "allocate 24-byte camera record");
        events.emplace_back("allocate record");
        return allocation_succeeds ? &allocated : nullptr;
    }
    std::uint32_t FreeRecord(
        pal4::ScreenEffectCameraRecord* state) override {
        Check(state == &allocated, "free allocated camera record");
        events.emplace_back("free record");
        return 0x7777u;
    }

    std::vector<std::string> events;
    pal4::ScreenEffectCameraRecord allocated{};
    bool allocation_succeeds{true};
    std::uint32_t color_result{0x1001u};
    std::uint32_t depth_result{0x1002u};
    std::uint32_t camera_color{};
    std::uint32_t camera_depth{};
    std::uint32_t active_camera{0x3001u};
    std::uint32_t effect_world{0x5000u};
    std::uint32_t game_world{0x6000u};
};

void AllocationAndEndUpdate() {
    Runtime runtime;
    runtime.allocated = {9u, {1u, 2u, 3u}, 0x11u, 0x22u,
                         33, 44, 0x55667788u};
    runtime.allocation_succeeds = false;
    Check(pal4::ScreenEffectCamera::Allocate(runtime) == nullptr &&
              runtime.events == std::vector<std::string>({"allocate record"}),
          "allocation failure returns null");
    runtime.events.clear();
    runtime.allocation_succeeds = true;
    auto* state = pal4::ScreenEffectCamera::Allocate(runtime);
    Check(state == &runtime.allocated && state->active == 0u &&
              state->texture == 0u && state->camera == 0u &&
              state->width == 0 && state->height == 0 &&
              state->reserved[0] == 1u && state->reserved[2] == 3u &&
              state->clear_color == 0x55667788u,
          "factory writes only observed fields and preserves unwritten bytes");
    runtime.events.clear();
    Check(pal4::ScreenEffectCamera::EndUpdate(*state, 0xabcd1234u,
                                               runtime) == 0xabcd1234u &&
              runtime.events.empty(), "inactive end update preserves EAX");
    state->active = 1u;
    state->camera = 0x2000u;
    Check(pal4::ScreenEffectCamera::EndUpdate(*state, 0u, runtime) ==
              0x1234u && runtime.events ==
              std::vector<std::string>({"end update"}),
          "active end update forwards RenderWare result");
    runtime.events.clear();
    state->active = 0u;
    Check(pal4::ScreenEffectCamera::Destroy(state, 0u, runtime) == 0x7777u &&
              runtime.events == std::vector<std::string>({"free record"}),
          "destroy calls cleanup then allocator free");
    runtime.events.clear();
    Check(pal4::ScreenEffectCamera::Destroy(nullptr, 0x123u, runtime) ==
              0x123u && runtime.events.empty(),
          "null destroy preserves incoming EAX");
}

void InitializationFailures() {
    Runtime runtime;
    pal4::ScreenEffectCameraRecord state{};
    runtime.color_result = 0u;
    Check(pal4::ScreenEffectCamera::Initialize(state, runtime) == 0u &&
              state.active == 0u && state.width == 800 &&
              state.height == 600 && state.clear_color == 0xff000000u &&
              runtime.events == std::vector<std::string>({"width", "height",
                  "color raster", "depth raster", "destroy depth"}),
          "first raster failure still creates then destroys depth raster");
    runtime.events.clear();
    runtime.color_result = 0x1001u;
    runtime.depth_result = 0u;
    Check(pal4::ScreenEffectCamera::Initialize(state, runtime) == 0u &&
              runtime.events == std::vector<std::string>({"width", "height",
                  "color raster", "depth raster", "destroy color"}),
          "second raster failure destroys first raster");
}

void InitializationAndCleanup() {
    Runtime runtime;
    pal4::ScreenEffectCameraRecord state{};
    Check(pal4::ScreenEffectCamera::Initialize(state, runtime) == 1u &&
              state.active == 1u && state.camera == 0x2000u &&
              state.texture == 0x4000u && state.clear_color == 0xff000000u &&
              runtime.camera_color == 0x1001u &&
              runtime.camera_depth == 0x1002u,
          "successful camera initialization stores resources and active flag");
    Check(runtime.events == std::vector<std::string>({"width", "height",
              "color raster", "depth raster", "create camera",
              "set color slot", "set depth slot", "create frame",
              "attach frame", "near value", "far value", "set near",
              "set far", "view values", "set view", "read color slot",
              "create texture", "clear camera"}),
          "camera initialization follows observed order");
    runtime.events.clear();
    Check(pal4::ScreenEffectCamera::Initialize(state, runtime) == 1u &&
              runtime.events.size() > 10u &&
              runtime.events[0] == "effect world" &&
              runtime.events[1] == "remove camera" &&
              runtime.events[2] == "destroy texture" &&
              runtime.events[9] == "width",
          "reinitialization releases old resources before creating new ones");
    runtime.events.clear();
    pal4::ScreenEffectCamera::Cleanup(state, runtime);
    Check(state.active == 0u && state.texture == 0u && state.camera == 0u &&
              runtime.camera_color == 0u && runtime.camera_depth == 0u &&
              runtime.events == std::vector<std::string>({"effect world",
                  "remove camera", "destroy texture", "read color slot",
                  "clear color slot", "read depth slot", "destroy depth",
                  "clear depth slot", "destroy camera"}),
          "cleanup removes world, releases texture and depth, clears raster slots");
    runtime.events.clear();
    pal4::ScreenEffectCamera::Cleanup(state, runtime);
    Check(runtime.events.empty(), "inactive cleanup is inert");
}

void WorldSynchronization() {
    Runtime runtime;
    pal4::ScreenEffectCameraRecord state{};
    pal4::ScreenEffectCamera::SyncWorld(state, runtime);
    Check(runtime.events.empty(), "inactive world sync is inert");
    state.active = 1u;
    state.camera = 0x2000u;
    runtime.active_camera = 0u;
    pal4::ScreenEffectCamera::SyncWorld(state, runtime);
    Check(runtime.events == std::vector<std::string>({"active camera"}),
          "null active game camera gates sync");
    runtime.events.clear();
    runtime.active_camera = 0x3001u;
    state.camera = 0u;
    pal4::ScreenEffectCamera::SyncWorld(state, runtime);
    Check(runtime.events == std::vector<std::string>({"active camera"}),
          "null effect camera gates world comparison");
    runtime.events.clear();
    state.camera = 0x2000u;
    runtime.game_world = runtime.effect_world;
    pal4::ScreenEffectCamera::SyncWorld(state, runtime);
    Check(runtime.events == std::vector<std::string>({"active camera",
              "game world", "effect world"}),
          "same world causes no remove/add");
    runtime.events.clear();
    runtime.game_world = 0x6000u;
    pal4::ScreenEffectCamera::SyncWorld(state, runtime);
    Check(runtime.events == std::vector<std::string>({"active camera",
              "game world", "effect world", "remove camera", "add camera"}),
          "different worlds remove then add effect camera");
    runtime.events.clear();
    runtime.effect_world = 0u;
    pal4::ScreenEffectCamera::SyncWorld(state, runtime);
    Check(runtime.events == std::vector<std::string>({"active camera",
              "game world", "effect world", "add camera"}),
          "null old world only adds camera");
    runtime.events.clear();
    runtime.effect_world = 0x5000u;
    runtime.game_world = 0u;
    pal4::ScreenEffectCamera::SyncWorld(state, runtime);
    Check(runtime.events == std::vector<std::string>({"active camera",
              "game world", "effect world", "remove camera"}),
          "null new world only removes camera");
}

}  // namespace

int main() {
    AllocationAndEndUpdate();
    InitializationFailures();
    InitializationAndCleanup();
    WorldSynchronization();
    std::cout << "screen effect camera tests passed\n";
}
