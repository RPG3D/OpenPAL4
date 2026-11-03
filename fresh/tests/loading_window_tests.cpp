#include "pal4/loading_window.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(float left, float right) {
    return std::fabs(left - right) < 0.00001f;
}

struct Runtime final : pal4::LoadingWindowRuntimeStub {
    std::vector<std::string> calls;
    float width{};
    float mask_x{};
    float poser_x{};

    void LoadLayout(pal4::LoadingWindowState& state,
                    std::string_view layout, std::uint32_t first,
                    std::uint32_t second) override {
        Check(layout == "loading" && first == 0u && second == 0u,
              "loading layout parameters");
        calls.emplace_back("layout");
        state.layout_prefix = "Loading/";
    }
    std::uint32_t GetWindow(std::string_view name) override {
        calls.emplace_back("window:" + std::string(name));
        if (name == "Loading/ImgPoser") return 0x1000u;
        if (name == "Loading/ImgMask") return 0x2000u;
        Check(false, "unexpected loading control name");
        return 0u;
    }
    float GetXPosition(std::uint32_t window) override {
        calls.emplace_back("get-x:" + std::to_string(window));
        return window == 0x2000u ? 0.2f : 0.3f;
    }
    float GetWidth(std::uint32_t window) override {
        Check(window == 0x2000u, "mask width queried");
        calls.emplace_back("get-width");
        return 0.8f;
    }
    void SetWidth(std::uint32_t window, float value) override {
        Check(window == 0x2000u, "update width of mask");
        calls.emplace_back("set-width");
        width = value;
    }
    void SetXPosition(std::uint32_t window, float value) override {
        calls.emplace_back("set-x:" + std::to_string(window));
        if (window == 0x2000u) mask_x = value;
        else if (window == 0x1000u) poser_x = value;
        else Check(false, "position update targets loading control");
    }
    void SetParent(pal4::LoadingWindowState&, bool visible,
                   std::uint32_t parent) override {
        Check(parent == 0x3000u, "loading parent handle");
        calls.emplace_back(visible ? "parent:show" : "parent:hide");
    }
};

void TestConstruct() {
    pal4::LoadingWindowState state{};
    Runtime runtime;
    Check(&pal4::LoadingWindow::Construct(state, runtime) == &state,
          "constructor returns loading state");
    Check(state.vtable == 0x8438f8u && state.poser == 0x1000u &&
              state.mask == 0x2000u && Near(state.mask_x, 0.2f) &&
              Near(state.mask_width, 0.8f) && Near(state.poser_x, 0.3f),
          "constructor stores vtable, controls and initial geometry");
    Check(runtime.calls == std::vector<std::string>{
              "layout", "window:Loading/ImgPoser",
              "window:Loading/ImgMask", "get-x:8192",
              "get-width", "get-x:4096"},
          "constructor lookup and measurement order");
}

void TestProgressAndVisibility() {
    pal4::LoadingWindowState state{};
    Runtime runtime;
    (void)pal4::LoadingWindow::Construct(state, runtime);
    runtime.calls.clear();
    pal4::LoadingWindow::UpdateProgress(state, 0.25f, runtime);
    Check(Near(runtime.width, 0.6f) && Near(runtime.mask_x, 0.4f) &&
              Near(runtime.poser_x, 0.3f + 0.25f * 0.32249999f),
          "quarter progress resizes mask and moves both controls");
    Check(runtime.calls == std::vector<std::string>{
              "set-width", "set-x:8192", "set-x:4096"},
          "progress update order");

    pal4::LoadingWindow::UpdateProgress(state, 1.5f, runtime);
    Check(Near(runtime.width, -0.4f) && Near(runtime.mask_x, 1.4f),
          "progress is not clamped above one");
    pal4::LoadingWindow::UpdateProgress(state, -0.5f, runtime);
    Check(Near(runtime.width, 1.2f) && Near(runtime.mask_x, -0.2f),
          "progress is not clamped below zero");

    runtime.calls.clear();
    pal4::LoadingWindow::SetVisible(state, true, 0x3000u, runtime);
    Check(Near(runtime.width, 0.8f) && Near(runtime.mask_x, 0.2f) &&
              Near(runtime.poser_x, 0.3f) &&
              runtime.calls == std::vector<std::string>{
                  "set-width", "set-x:8192", "set-x:4096",
                  "parent:show"},
          "show resets progress before attaching parent");
    runtime.calls.clear();
    pal4::LoadingWindow::SetVisible(state, false, 0x3000u, runtime);
    Check(runtime.calls.back() == "parent:hide" &&
              runtime.calls.front() == "set-width",
          "hide also resets progress before parent change");
}

}  // namespace

int main() {
    TestConstruct();
    TestProgressAndVisibility();
}
