#include "pal4/loading_progress_updater.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ILoadingProgressRuntime {
    std::vector<std::string> events;
    bool video_ready = true;
    std::uint32_t window = 0x2000u;
    std::uint32_t state = 0x4000u;
    std::uint32_t state_type = 6u;
    std::uint32_t camera = 0x6000u;
    unsigned camera_reads = 0;

    bool HandleVideoModeChange() override {
        events.emplace_back("video");
        return video_ready;
    }
    std::uint32_t GetUiFrameManager() override {
        events.emplace_back("ui-manager");
        return 0x1000u;
    }
    std::uint32_t CreateLoadingWindow(std::uint32_t manager) override {
        Check(manager == 0x1000u, "loading window manager");
        events.emplace_back("create-window");
        return window;
    }
    void UpdateLoadingWindow(std::uint32_t handle, float progress) override {
        Check(handle == window && progress == 0.375f,
              "loading window receives progress");
        events.emplace_back("progress");
    }
    void UpdateUi(std::uint32_t manager, double delta) override {
        Check(manager == 0x1000u && delta == 0.1000000014901161,
              "UI frame uses original float-promoted delta");
        events.emplace_back("ui-update");
    }
    std::uint32_t GetGameController() override {
        events.emplace_back("game");
        return 0x3000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x318cu) {
            events.emplace_back("state-slot");
            return state;
        }
        Check(address == 0x5000u, "camera slot address");
        events.emplace_back("camera-slot");
        return camera + camera_reads++;
    }
    std::uint32_t GetStateType(std::uint32_t handle) override {
        Check(handle == state, "state type receiver");
        events.emplace_back("state-type");
        return state_type;
    }
    std::uint32_t InitializeCameraSubsystem() override {
        events.emplace_back("camera-system");
        return 0x7000u;
    }
    std::uint32_t GetActiveCameraSlot(std::uint32_t system) override {
        Check(system == 0x7000u, "camera subsystem receiver");
        events.emplace_back("active-camera-slot");
        return 0x5000u;
    }
    void ClearCamera(std::uint32_t handle, std::uint32_t color,
                     std::uint32_t flags) override {
        Check(handle == 0x6000u && color == 0x8f6048u && flags == 3u,
              "clear arguments");
        events.emplace_back("clear");
    }
    void BeginCameraUpdate(std::uint32_t handle) override {
        Check(handle == 0x6001u, "begin rereads camera slot");
        events.emplace_back("begin");
    }
    void SetRenderState(std::uint32_t key, std::uint32_t value) override {
        events.emplace_back("render:" + std::to_string(key) + ":" +
                            std::to_string(value));
    }
    void RenderState(std::uint32_t handle,
                     std::uint32_t camera_slot) override {
        Check(handle == state && camera_slot == 0x5000u,
              "state render receives camera slot pointer");
        events.emplace_back("state-render");
    }
    void EndCameraUpdate(std::uint32_t handle) override {
        Check(handle == 0x6002u, "end rereads camera slot");
        events.emplace_back("end");
    }
    void ShowGameRaster(std::uint32_t handle) override {
        Check(handle == 0x6003u, "raster display rereads camera slot");
        events.emplace_back("show");
    }
    void HandleDeviceLoss() override { events.emplace_back("device-loss"); }
};

void Branches() {
    Runtime runtime;
    runtime.video_ready = false;
    pal4::LoadingProgressUpdater::Update(0.375f, runtime);
    Check(runtime.events == std::vector<std::string>{"video"},
          "video failure returns immediately");

    runtime = Runtime{};
    runtime.window = 0;
    pal4::LoadingProgressUpdater::Update(0.375f, runtime);
    Check(runtime.events ==
              (std::vector<std::string>{"video", "ui-manager",
                                        "create-window", "device-loss"}),
          "missing loading window still handles device loss");

    for (const auto state : {0u, 0x4000u}) {
        runtime = Runtime{};
        runtime.state = state;
        runtime.state_type = 5u;
        pal4::LoadingProgressUpdater::Update(0.375f, runtime);
        auto expected = std::vector<std::string>{
            "video", "ui-manager", "create-window", "progress",
            "ui-manager", "ui-update", "game", "state-slot"};
        if (state != 0u) expected.emplace_back("state-type");
        expected.emplace_back("device-loss");
        Check(runtime.events == expected && runtime.camera_reads == 0,
              "null or non-loading state skips rendering");
    }
}

void FullRender() {
    Runtime runtime;
    pal4::LoadingProgressUpdater::Update(0.375f, runtime);
    Check(runtime.events == (std::vector<std::string>{
              "video", "ui-manager", "create-window", "progress",
              "ui-manager", "ui-update", "game", "state-slot",
              "state-type", "camera-system", "active-camera-slot",
              "camera-slot", "clear", "camera-slot", "begin",
              "render:20:2", "render:6:1", "render:8:1", "state-render",
              "camera-slot", "end", "camera-slot", "show", "device-loss"}) &&
              runtime.camera_reads == 4u,
          "loading-state render and camera rereads follow IDA order");

    pal4::LoadingProgressRuntimeStub stub;
    pal4::LoadingProgressUpdater::Update(0.375f, stub);
}

}  // namespace

int main() {
    try {
        Branches();
        FullRender();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
