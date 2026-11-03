#include "pal4/system_menu_state.h"

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

struct Runtime final : pal4::ISystemMenuRuntime {
    std::vector<std::string> calls;
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x3000u + 412u, "UI camera field offset");
        calls.emplace_back("read-camera"); return 0x4000u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x5000u && value == 0u, "console clear");
        calls.emplace_back("clear-console");
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("ui"); return 0x3000u;
    }
    void SetUIFlag(std::uint32_t manager, std::uint32_t value) override {
        Check(manager == 0x3000u && value == 1u, "system UI flag");
        calls.emplace_back("set-flag");
    }
    std::uint32_t Console() override {
        calls.emplace_back("console"); return 0x5000u;
    }
    void AddUIElement(std::uint32_t manager,
                      std::string_view name) override {
        Check(manager == 0x3000u && name == "BJ", "system UI element");
        calls.emplace_back("add-BJ");
    }
    void SetupUICamera(std::uint32_t camera,
                       std::uint32_t enabled) override {
        Check(camera == 0x4000u && enabled == 0u, "camera setup flag");
        calls.emplace_back("setup-camera");
    }
    void CopyActiveCameraRaster(std::uint32_t manager) override {
        Check(manager == 0x3000u, "copy camera raster manager");
        calls.emplace_back("copy-raster");
    }
    void GetActiveCamera(std::uint32_t camera) override {
        Check(camera == 0x4000u, "active camera field");
        calls.emplace_back("get-active-camera");
    }
    std::uint32_t CameraManager() override {
        calls.emplace_back("camera-manager"); return 0x7000u;
    }
    std::uint32_t UICameraName() override { return 0x8000u; }
    void ActivateCamera(std::uint32_t manager,
                        std::uint32_t name) override {
        Check(manager == 0x7000u && name == 0x8000u,
              "UI camera activation");
        calls.emplace_back("activate-camera");
    }
    void DispatchEvent(std::uint32_t manager,
                       std::uint32_t descriptor,
                       std::uint32_t first,
                       std::uint32_t second) override {
        Check(manager == 0x3000u && first == 0u && second == 0u,
              "system close event payload");
        if (descriptor == pal4::SystemMenuState::kFirstCloseEvent)
            calls.emplace_back("first-close-event");
        else {
            Check(descriptor == pal4::SystemMenuState::kSecondCloseEvent,
                  "second close event descriptor");
            calls.emplace_back("second-close-event");
        }
    }
    void ToggleWindows(std::uint32_t manager, bool enabled) override {
        Check(manager == 0x3000u, "toggle windows manager");
        calls.emplace_back(enabled ? "windows-on" : "windows-off");
    }
    void PreRenderUI(std::uint32_t manager,
                     std::uint32_t frame) override {
        Check(manager == 0x3000u && frame == 0x6000u,
              "UI pre-render arguments");
        calls.emplace_back("pre-render");
    }
    std::uint32_t RenderUI(std::uint32_t manager,
                            std::uint32_t frame) override {
        Check(manager == 0x3000u && frame == 0x6000u,
              "UI render arguments");
        calls.emplace_back("render-ui"); return 0x77u;
    }
    void UpdateUI(std::uint32_t manager, double delta) override {
        Check(manager == 0x3000u && delta == 0.25,
              "UI update widened delta");
        calls.emplace_back("update-ui");
    }
    std::uint32_t RenderEffectTransform(
        std::uint32_t manager, double delta) override {
        Check(manager == 0x3000u && delta == 0.25,
              "effect update widened delta");
        calls.emplace_back("effect-transform"); return 0x88u;
    }
    void CleanupEffects(std::uint32_t manager) override {
        Check(manager == 0x3000u, "effect cleanup manager");
        calls.emplace_back("cleanup-effects");
    }
};

void TestSystemMenu() {
    Runtime runtime;
    Check(pal4::SystemMenuState::EnterWrapper(99u, 98u, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{
              "ui", "set-flag", "console", "clear-console",
              "ui", "add-BJ", "ui", "read-camera", "setup-camera",
              "ui", "copy-raster", "ui", "read-camera",
              "get-active-camera", "camera-manager", "activate-camera",
              "ui", "first-close-event", "ui", "second-close-event",
              "ui", "windows-on"},
          "system menu entry sequence and two zeroed events");
    runtime.calls.clear();
    Check(pal4::SystemMenuState::Render(0x6000u, runtime) == 0x77u &&
          runtime.calls == std::vector<std::string>{
              "ui", "pre-render", "ui", "render-ui"},
          "shared pre-render then render wrapper");
    runtime.calls.clear();
    Check(pal4::SystemMenuState::Update(0.25f, 99u, 98u, runtime) == 0x88u &&
          runtime.calls == std::vector<std::string>{
              "ui", "update-ui", "ui", "effect-transform"},
          "UI and effect update result");
    runtime.calls.clear();
    Check(pal4::SystemMenuState::LeaveWrapper(99u, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{
              "ui", "windows-off", "ui", "cleanup-effects"},
          "leave wrapper disables windows before effect cleanup");
}

}  // namespace

int main() {
    try {
        TestSystemMenu();
        std::cout << "system menu state tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
