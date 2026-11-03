#include "pal4/scenario_dialog.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScenarioVisibilityRuntime {
    std::vector<std::string> calls;
    bool active = false;
    std::uint32_t instances = 0u;

    std::uint32_t ScriptInstance() override {
        calls.emplace_back("script");
        return 0x2000u + (instances++ * 0x100u);
    }
    bool AudioActive(std::uint32_t audio) override {
        Check(audio == 0x2020u, "audio active offset");
        calls.emplace_back("active");
        return active;
    }
    void PlayAudio(std::uint32_t audio) override {
        Check(audio == 0x2120u, "second script instance audio");
        calls.emplace_back("play");
    }
    std::string ReadGlobalString(std::uint32_t address) override {
        Check(address == 0x8dc780u, "empty text global");
        calls.emplace_back("empty");
        return "";
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        if (address == 0x1038u) return 0x3000u;
        Check(address == 0x102cu, "portrait address");
        return 0x4000u;
    }
    void SetText(std::uint32_t window,
                 const std::string& text) override {
        Check(window == 0x3000u && text.empty(), "clear editbox");
        calls.emplace_back("text");
    }
    void SetImage(std::uint32_t window,
                  std::uint32_t image) override {
        Check(window == 0x4000u && image == 0u,
              "clear portrait");
        calls.emplace_back("image");
    }
    void Layout(std::uint32_t object,
                std::uint32_t option) override {
        Check(object == 0x1000u && option == 1u,
              "hide layout option");
        calls.emplace_back("layout");
    }
    void Close(std::uint32_t object, std::uint32_t visible,
               std::uint32_t parent) override {
        Check(object == 0x1000u && visible == 0u && parent == 0u,
              "virtual hide arguments");
        calls.emplace_back("close");
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    std::uint32_t RemoveHandler(
        std::uint32_t array, std::uint32_t handler) override {
        Check(array == 0x5134u && handler == 0x1028u,
              "remove embedded handler");
        calls.emplace_back("remove");
        return 0x9090u;
    }
};

void CheckHide(bool active) {
    Runtime runtime;
    runtime.active = active;
    Check(pal4::ScenarioDialog::HideAndClear(
              0x1000u, runtime) == 0x9090u &&
              runtime.instances == (active ? 2u : 1u),
          "audio branch and remove result");
    const std::vector<std::string> suffix{
        "empty", "read", "text", "read", "image", "layout",
        "close", "manager", "remove"};
    auto expected = active
        ? std::vector<std::string>{"script", "active", "script", "play"}
        : std::vector<std::string>{"script", "active"};
    expected.insert(expected.end(), suffix.begin(), suffix.end());
    Check(runtime.calls == expected, "hide and clear order");
}

}  // namespace

int main() {
    try {
        CheckHide(false);
        CheckHide(true);
        std::cout << "scenario_dialog_visibility_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
