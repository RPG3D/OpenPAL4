#include "pal4/ui_frame_manager_cegui.h"
#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct RendererRuntime final : pal4::ICeguiRendererDestructorRuntime {
    std::vector<std::uint32_t> calls;
    void DestructRenderer(std::uint32_t renderer) override {
        calls.push_back(renderer);
    }
};

struct RegisterRuntime final
    : pal4::IUiFrameManagerRegisterWindowsRuntime {
    std::vector<std::string> names;
    std::string fail_at;
    std::uint32_t RegisterFrame(std::uint32_t manager,
                                const char* name) override {
        Check(manager == 0x5000u, "window registration owner");
        names.emplace_back(name);
        if (names.back() == fail_at)
            throw std::runtime_error("register failure");
        return static_cast<std::uint32_t>(0xa000u + names.size());
    }
};

void CheckThunk() {
    RendererRuntime runtime;
    pal4::UiFrameManagerCegui::RendererDestructorThunk(
        0x1234u, runtime);
    pal4::UiFrameManagerCegui::RendererDestructorThunk(
        0u, runtime);
    Check(runtime.calls ==
              std::vector<std::uint32_t>{0x1234u, 0u},
          "renderer destructor thunk forwards receiver unchanged");
}

void CheckWindowRegistration() {
    RegisterRuntime runtime;
    Check(pal4::UiFrameManagerSingleton::RegisterWindows(
              0x5000u, runtime) == 0xa00bu,
          "returns final portrait registration");
    Check(runtime.names == std::vector<std::string>{
              "roleStateWindow", "EquipmentWindow", "magicWindow",
              "MissionWindow", "PropertyWindow", "SystemSetting",
              "sysToolBar", "SmithWindow", "TradeWindow",
              "sceneWindow", "portrait"},
          "all window names and order");

    RegisterRuntime failing;
    failing.fail_at = "PropertyWindow";
    bool threw = false;
    try {
        (void)pal4::UiFrameManagerSingleton::RegisterWindows(
            0x5000u, failing);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) == "register failure";
    }
    Check(threw && failing.names.size() == 5u &&
              failing.names.back() == "PropertyWindow",
          "registration stops at first failure");
}

}  // namespace

int main() {
    try {
        CheckThunk();
        CheckWindowRegistration();
        std::cout << "ui_frame_manager_followup_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
