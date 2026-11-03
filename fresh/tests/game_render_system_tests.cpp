#include "pal4/game_render_system.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class RenderWare final : public pal4::IRenderWareApi, public pal4::IRenderWarePresentation {
public:
    explicit RenderWare(std::vector<std::string>& events) : events(events) {}

    std::uint32_t ShowRaster(const std::uint32_t camera,
                             const std::uint32_t window,
                             const std::uint32_t flags) override {
        events.emplace_back("show");
        Check(camera == 0x4000u && window == 0x5000u && flags <= 1u,
              "raster call arguments");
        last_flags = flags;
        return 77u + flags;
    }
    void DestroyTextureDictionary(const std::uint32_t dictionary) override {
        events.emplace_back("destroy");
        destroyed.push_back(dictionary);
    }
    void CloseCharset() override { events.emplace_back("charset-close"); }
    void ForEachTextureDictionary(pal4::GameRenderSystem& system,
                                  const std::uint32_t data) override {
        events.emplace_back("each");
        Check(data == 0u, "dictionary iteration receives null user data");
        Check(system.DestroyTexture(0x10u, *this) == 0x10u &&
                  system.DestroyTexture(0x20u, *this) == 0x20u,
              "dictionary callback returns its input pointer");
    }
    void CloseFileSystemManager() override {
        events.emplace_back("fs-manager-close");
    }
    void EngineStop() override { events.emplace_back("stop"); }
    void EngineClose() override { events.emplace_back("close"); }
    void EngineShutdown() override { events.emplace_back("shutdown"); }

    std::vector<std::string>& events;
    std::vector<std::uint32_t> destroyed;
    std::uint32_t last_flags = 0u;
};

class Platform final : public pal4::IGameRenderPlatform {
public:
    explicit Platform(std::vector<std::string>& events) : events(events) {}

    std::int32_t FloatToLong(const float value) override {
        events.emplace_back("ftol");
        inputs.push_back(value);
        return static_cast<std::int32_t>(value);
    }
    void ClientToScreen(const std::uint32_t window, std::int32_t& x,
                        std::int32_t& y) override {
        events.emplace_back("client-to-screen");
        Check(window == 0x5000u, "viewport uses game window handle");
        x += 100;
        y += 200;
    }
    std::int32_t SetCursorPos(const std::int32_t x,
                              const std::int32_t y) override {
        events.emplace_back("cursor");
        cursor_x = x;
        cursor_y = y;
        return 47;
    }
    std::int32_t ChangeDisplaySettings(const std::uint32_t mode,
                                        const std::uint32_t flags) override {
        events.emplace_back("display");
        Check(mode == 0u && flags == 0u,
              "shutdown restores default display settings");
        return -1;
    }
    std::uint32_t EndTimePeriod(const std::uint32_t period) override {
        events.emplace_back("end-period");
        last_period = period;
        return 66u;
    }

    std::vector<std::string>& events;
    std::vector<float> inputs;
    std::int32_t cursor_x = 0;
    std::int32_t cursor_y = 0;
    std::uint32_t last_period = 0u;
};

class CameraView final : public pal4::ICameraViewRuntime {
public:
    void AdjustCameraView(const std::uint32_t camera,
                          std::array<std::int32_t, 4>& rectangle,
                          const float view_window,
                          const std::uint32_t mode) override {
        ++calls;
        Check(camera == 0x7777u &&
                  rectangle == std::array<std::int32_t, 4>{1, 2, 800, 600} &&
                  view_window == 0.75f && mode == 3u,
              "resize forwards camera, four arguments and config values");
        rectangle[2] = 1024;
    }
    std::uint32_t calls = 0u;
};

}  // namespace

int main() {
    {
        pal4::GameRenderState bound(0x9000u);
        bound.Write(0x20u, 1u);
        std::array<std::byte, pal4::GameRenderState::kBytes + 1u> bytes{};
        auto unaligned = std::span(bytes).subspan(1u);
        Check(!bound.BindExternal(0x1234u, unaligned.first(
                  pal4::GameRenderState::kBytes - 1u)),
              "external state rejects undersized backing");
        Check(bound.BindExternal(0x1234u, unaligned) &&
                  bound.Address() == 0x1234u,
              "external state accepts unaligned native backing");
        std::uint32_t raw = 0u;
        std::memcpy(&raw, unaligned.data() + 0x20u, sizeof(raw));
        Check(raw == 1u, "binding copies initialized model state");
        constexpr std::uint32_t external_value = 0x12345678u;
        std::memcpy(unaligned.data() + 0x5a4u, &external_value,
                    sizeof(external_value));
        Check(bound.Read(0x5a4u) == external_value,
              "model sees direct unaligned native writes");
        bound.WriteShared(0x28u, 7u);
        std::memcpy(&raw, unaligned.data() + 0x28u, sizeof(raw));
        Check(raw == 7u && bound.ReadShared(0x28u) == 7u,
              "shared model writes reach native state");
        bound.UnbindExternal();
        Check(bound.Address() == 0x9000u &&
                  bound.Read(0x5a4u) == external_value &&
                  bound.ReadShared(0x28u) == 7u,
              "unbinding retains final state without dangling storage");
    }
    std::vector<std::string> events;
    pal4::GameRenderState state(0x9000u);
    state.Write(0u, 0x5000u);
    pal4::GameRenderSystem system(state);
    RenderWare renderware(events);
    Platform platform(events);

    state.Write(0x54cu, 1u);
    Check(system.ShowGameRaster(0x4000u, renderware) == 77u &&
              renderware.last_flags == 0u,
          "missing first flag uses raster mode zero");
    state.Write(8u, 1u);
    Check(system.ShowGameRaster(0x4000u, renderware) == 78u &&
              renderware.last_flags == 1u,
          "both state fields enable raster mode one");
    state.Write(0x54cu, 0u);
    Check(system.ShowGameRaster(0x4000u, renderware) == 77u &&
              renderware.last_flags == 0u,
          "missing second flag disables raster mode one");

    events.clear();
    state.Write(0x2cu, 0xa5a5u);
    Check(system.SetViewportCenter(12.75f, -3.5f, platform) == 0x9000 &&
              state.Read(0x2cu) == 0xa5a5u && events.empty(),
          "inactive viewport returns state address without writes");
    state.Write(0x20u, 1u);
    Check(system.SetViewportCenter(12.75f, -3.5f, platform) == 47 &&
              state.Read(0x2cu) == 0u &&
              platform.inputs == std::vector<float>{12.75f, -3.5f} &&
              platform.cursor_x == 112 && platform.cursor_y == 197 &&
              events == std::vector<std::string>{
                  "ftol", "ftol", "client-to-screen", "cursor"},
          "viewport converts coordinates, moves to screen and returns result");

    events.clear();
    Check(system.ShutdownRenderSystem(renderware, platform) == -1 &&
              events == std::vector<std::string>{
                  "charset-close", "each", "destroy", "destroy", "fs-manager-close",
                  "stop", "close", "shutdown", "display"} &&
              renderware.destroyed ==
                  std::vector<std::uint32_t>{0x10u, 0x20u},
          "render shutdown preserves exact callback and platform order");

    state.Write(0x18u, 7u);
    events.clear();
    Check(system.EndTimePeriod(platform) == 66u &&
              platform.last_period == 7u &&
              events == std::vector<std::string>{"end-period"},
          "timeEndPeriod forwards game-state period and return value");

    CameraView camera_view;
    std::array<std::uint32_t, 12> resize_config{};
    resize_config[4] = 3u;
    resize_config[5] = std::bit_cast<std::uint32_t>(0.75f);
    pal4::GameRenderSystem::ResizeRenderScreen(
        {1, 2, 800, 600}, 0u, resize_config, camera_view);
    Check(camera_view.calls == 0u, "null camera skips adjustment");
    pal4::GameRenderSystem::ResizeRenderScreen(
        {1, 2, 800, 600}, 0x7777u, resize_config, camera_view);
    Check(camera_view.calls == 1u, "non-null camera adjusts once");
    std::cout << "game render system tests passed\n";
}
