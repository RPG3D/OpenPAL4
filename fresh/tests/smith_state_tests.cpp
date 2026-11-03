#include "pal4/smith_state.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithStateRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::optional<std::uint32_t> configured;
    bool smith_flag = false;
    Runtime() { words[0x3000u + 412u] = 0x8000u; }

    std::uint32_t UIFrameManager() override {
        calls.emplace_back("ui"); return 0x3000u;
    }
    void SetUIFlag(std::uint32_t manager, std::uint32_t value) override {
        Check(manager == 0x3000u && value == 1u, "smith UI flag");
        calls.emplace_back("set-ui-flag");
    }
    std::uint32_t Console() override {
        calls.emplace_back("console"); return 0x4000u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-camera"); return words[address];
    }
    void AddUIElement(std::uint32_t manager,
                      std::string_view name) override {
        Check(manager == 0x3000u && name == "BJ", "smith UI element");
        calls.emplace_back("add-BJ");
    }
    std::uint32_t GameState() override {
        calls.emplace_back("game-state"); return 0x5000u;
    }
    std::optional<std::uint32_t> ReadIntegerAttribute(
        std::uint32_t config, std::string_view key) override {
        Check(config == 0x6000u && key == "Smith_OpenWithSystem",
              "smith config attribute");
        calls.emplace_back("read-config"); return configured;
    }
    void GetActiveCamera(std::uint32_t camera) override {
        Check(camera == 0x8000u, "smith UI active camera");
        calls.emplace_back("get-active-camera");
    }
    std::uint32_t CameraManager() override {
        calls.emplace_back("camera-manager"); return 0x9000u;
    }
    std::uint32_t UICameraName() override { return 0xa000u; }
    void ActivateCamera(std::uint32_t manager,
                        std::uint32_t name) override {
        Check(manager == 0x9000u && name == 0xa000u,
              "smith UI camera activation");
        calls.emplace_back("activate-camera");
    }
    std::uint32_t SmithSystem() override {
        calls.emplace_back("smith-system"); return 0xb000u;
    }
    void SetSmithFlag(std::uint32_t system, bool enabled) override {
        Check(system == 0xb000u, "smith flag system");
        smith_flag = enabled;
        calls.emplace_back("set-smith-flag");
    }
    void SetSmithState(std::uint32_t system,
                       std::uint32_t raw_delta) override {
        Check(system == 0xb000u && raw_delta ==
              std::bit_cast<std::uint32_t>(0.25f),
              "smith state receives raw delta bits");
        calls.emplace_back("set-smith-state");
    }
    void ClearSmithFlags(std::uint32_t system) override {
        Check(system == 0xb000u, "smith clear system");
        calls.emplace_back("clear-smith-flags");
    }
    void UpdateUI(std::uint32_t manager, double delta) override {
        Check(manager == 0x3000u && delta == 0.25,
              "smith UI update widened delta");
        calls.emplace_back("update-ui");
    }
    std::uint32_t RenderEffectTransform(
        std::uint32_t manager, double delta) override {
        Check(manager == 0x3000u && delta == 0.25,
              "smith effect widened delta");
        calls.emplace_back("effect-transform"); return 0x77u;
    }
    void CleanupEffects(std::uint32_t manager) override {
        Check(manager == 0x3000u, "smith cleanup manager");
        calls.emplace_back("cleanup-effects");
    }
};

void TestEntryAndLeave() {
    Runtime runtime;
    Check(pal4::SmithState::EnterWrapper(
              99u, 98u, 0u, runtime) == 1u &&
          !runtime.smith_flag &&
          runtime.words[0x5000u + 1412u] == 0u &&
          runtime.words[0x5000u + 1364u] == 0u &&
          runtime.calls == std::vector<std::string>{
              "ui", "set-ui-flag", "console", "write:16384",
              "ui", "add-BJ", "game-state", "write:21892",
              "game-state", "write:21844", "ui", "read-camera",
              "get-active-camera", "camera-manager", "activate-camera",
              "smith-system", "set-smith-flag"},
          "null config entry order and cleared game flags");
    runtime.calls.clear(); runtime.configured = 1u;
    Check(pal4::SmithState::Enter(0x6000u, runtime) == 1u &&
          runtime.smith_flag &&
          runtime.calls[10] == "read-config",
          "positive system config enables smith flag");
    runtime.calls.clear(); runtime.configured = 0xffffffffu;
    Check(pal4::SmithState::Enter(0x6000u, runtime) == 1u &&
          !runtime.smith_flag, "negative config disables smith flag");
    runtime.calls.clear(); runtime.configured = std::nullopt;
    Check(pal4::SmithState::Enter(0x6000u, runtime) == 1u &&
          !runtime.smith_flag, "missing config value defaults to zero");
    runtime.calls.clear();
    Check(pal4::SmithState::LeaveWrapper(99u, 98u, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{
              "smith-system", "clear-smith-flags", "ui", "cleanup-effects"},
          "leave wrapper forwards cleanup order");
}

void TestUpdate() {
    Runtime runtime;
    Check(pal4::SmithState::Update(0.25f, 99u, 98u, runtime) == 0x77u &&
          runtime.calls == std::vector<std::string>{
              "smith-system", "set-smith-state", "ui", "update-ui",
              "ui", "effect-transform"},
          "smith update uses raw and widened delta in service order");
}

}  // namespace

int main() {
    try {
        TestEntryAndLeave();
        TestUpdate();
        std::cout << "smith state tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
