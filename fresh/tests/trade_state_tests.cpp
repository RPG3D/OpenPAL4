#include "pal4/trade_state.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ITradeStateRuntime {
    std::vector<std::string> calls;
    std::uint32_t active = 0u;
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x1000u) {
            calls.emplace_back("read-trade"); return active;
        }
        Check(address == 0x5000u, "frame word read");
        calls.emplace_back("read-frame"); return 0x1234u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x6000u && value == 0u, "trade console reset");
        calls.emplace_back("clear-console");
    }
    std::uint32_t TradeSystem() override {
        calls.emplace_back("trade-system"); return 0x1000u;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena"); return 0x2000u;
    }
    void ProcessWorldFrame(std::uint32_t arena,
                           std::uint32_t frame_value) override {
        Check(arena == 0x2000u && frame_value == 0x1234u,
              "world frame processing arguments");
        calls.emplace_back("process-world");
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("ui"); return 0x3000u;
    }
    void SetUIFlag(std::uint32_t manager, std::uint32_t value) override {
        Check(manager == 0x3000u && value == 1u, "trade UI flag");
        calls.emplace_back("set-flag");
    }
    void PreRenderUI(std::uint32_t manager,
                     std::uint32_t frame) override {
        Check(manager == 0x3000u && frame == 0x5000u,
              "UI pre-render arguments");
        calls.emplace_back("pre-render");
    }
    std::uint32_t RenderUI(std::uint32_t manager,
                            std::uint32_t frame) override {
        Check(manager == 0x3000u && frame == 0x5000u,
              "UI render arguments");
        calls.emplace_back("render-ui"); return 0x55u;
    }
    void UpdateUI(std::uint32_t manager, double delta) override {
        Check(manager == 0x3000u && delta == 0.25,
              "UI update widened delta");
        calls.emplace_back("update-ui");
    }
    std::uint32_t Console() override {
        calls.emplace_back("console"); return 0x6000u;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("script"); return 0x7000u;
    }
    void ProcessWaitHandlers(std::uint32_t logger, double delta) override {
        Check(logger == 0x7054u && delta == 0.25,
              "trade script wait arguments");
        calls.emplace_back("process-waits");
    }
    std::uint32_t CameraManager() override {
        calls.emplace_back("camera-manager"); return 0x8000u;
    }
    std::uint32_t MainCameraName() override { return 0x9000u; }
    std::uint32_t UICameraName() override { return 0xa000u; }
    std::uint32_t ActivateCamera(
        std::uint32_t manager, std::uint32_t name) override {
        Check(manager == 0x8000u, "camera manager argument");
        if (name == 0x9000u) {
            calls.emplace_back("main-camera"); return 0x66u;
        }
        Check(name == 0xa000u, "UI camera name");
        calls.emplace_back("ui-camera"); return 0x77u;
    }
    std::uint32_t RenderEffectTransform(
        std::uint32_t manager, double delta) override {
        Check(manager == 0x3000u && delta == 0.25,
              "trade effect transform arguments");
        calls.emplace_back("effect-transform"); return 0x88u;
    }
    void CloseShop(std::uint32_t system) override {
        Check(system == 0x1000u, "trade close-shop argument");
        calls.emplace_back("close-shop");
    }
    void CleanupEffects(std::uint32_t manager) override {
        Check(manager == 0x3000u, "trade cleanup manager");
        calls.emplace_back("cleanup-effects");
    }
};

void TestTradeState() {
    Runtime runtime;
    Check(pal4::TradeState::EnterWrapper(99u, 98u, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{
              "ui", "set-flag", "console", "clear-console"},
          "trade entry wrapper and console initialization");
    runtime.calls.clear();
    Check(pal4::TradeState::Render(0x5000u, runtime) == 0x55u &&
          runtime.calls == std::vector<std::string>{
              "trade-system", "read-trade", "read-frame", "arena",
              "process-world", "ui", "render-ui"},
          "closed trade renders world before UI");
    runtime.calls.clear(); runtime.active = 1u;
    Check(pal4::TradeState::Render(0x5000u, runtime) == 0x55u &&
          runtime.calls == std::vector<std::string>{
              "trade-system", "read-trade", "ui", "pre-render",
              "ui", "render-ui"},
          "active trade pre-renders UI without world processing");
    runtime.calls.clear(); runtime.active = 0u;
    Check(pal4::TradeState::Update(0.25f, 99u, 98u, runtime) == 0x66u &&
          runtime.calls == std::vector<std::string>{
              "script", "process-waits", "ui", "update-ui",
              "trade-system", "read-trade", "camera-manager", "main-camera"},
          "closed trade returns MainCamera activation result");
    runtime.calls.clear(); runtime.active = 1u;
    Check(pal4::TradeState::Update(0.25f, 99u, 98u, runtime) == 0x88u &&
          runtime.calls == std::vector<std::string>{
              "script", "process-waits", "ui", "update-ui",
              "trade-system", "read-trade", "camera-manager", "ui-camera",
              "ui", "effect-transform"},
          "active trade returns effect transform result");
    runtime.calls.clear();
    Check(pal4::TradeState::LeaveWrapper(99u, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{
              "trade-system", "close-shop", "ui", "cleanup-effects"},
          "trade leave closes shop before effects");
}

}  // namespace

int main() {
    try {
        TestTradeState();
        std::cout << "trade state tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
