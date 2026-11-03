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

struct Runtime final : pal4::IUiFrameManagerPortraitRuntime {
    std::uint32_t result = 0;
    bool throw_on_update = false;
    std::vector<std::string> calls;

    std::uint32_t FindFrame(std::uint32_t manager,
                            const char* name) override {
        Check(manager == 0x5000u && std::string(name) == "portrait",
              "lookup uses the manager and portrait frame name");
        calls.emplace_back("find");
        return result;
    }

    void SetFlagAndUpdateVisibility(std::uint32_t frame,
                                    std::uint8_t flag) override {
        Check(frame == 0x7000u && flag == 0xffu,
              "the exact frame and low flag byte are forwarded");
        calls.emplace_back("update");
        if (throw_on_update) throw std::runtime_error("update failed");
    }
};

}  // namespace

int main() {
    try {
        Runtime missing;
        pal4::UiFrameManagerSingleton::SetPortraitFlag(
            0x5000u, 0xffu, missing);
        Check(missing.calls == std::vector<std::string>{"find"},
              "missing portrait frame skips the setter");

        Runtime found;
        found.result = 0x7000u;
        pal4::UiFrameManagerSingleton::SetPortraitFlag(
            0x5000u, 0xffu, found);
        Check(found.calls == std::vector<std::string>{"find", "update"},
              "portrait flag is set after lookup");

        found.calls.clear();
        found.throw_on_update = true;
        bool threw = false;
        try {
            pal4::UiFrameManagerSingleton::SetPortraitFlag(
                0x5000u, 0xffu, found);
        } catch (const std::runtime_error& error) {
            threw = std::string(error.what()) == "update failed";
        }
        Check(threw && found.calls ==
                           std::vector<std::string>{"find", "update"},
              "setter failure propagates after lookup");
        std::cout << "ui_frame_manager_portrait_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
