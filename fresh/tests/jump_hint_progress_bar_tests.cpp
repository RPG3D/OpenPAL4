#include "pal4/jump_hint_progress_bar.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}

struct Runtime final : pal4::IJumpHintProgressRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::string layout;
    std::string requested_name;
    std::uint32_t name_field = 0u;
    std::uint32_t image = 0x6000u;
    std::uint32_t width_window = 0u;
    std::uint32_t x_window = 0u;
    std::uint32_t top_window = 0u;
    bool top = false;
    float image_x = 10.0f;
    float image_width = 100.0f;
    float set_width = 0.0f;
    float set_x = 0.0f;
    std::uint32_t visible_result = 1u;
    std::uint32_t base_visible_result = 0xabcdu;
    std::uint32_t base_visible = 0u;
    std::uint32_t base_option = 1u;

    void LoadLayout(std::uint32_t object,
                    const char* value) override {
        calls.emplace_back("load");
        Check(object == 0x1000u, "layout object");
        layout = value;
    }
    std::string ReadName(std::uint32_t address) override {
        calls.emplace_back("read-name");
        name_field = address;
        return "JumpHint/";
    }
    std::uint32_t GetWindow(std::uint32_t object,
                            const std::string& name) override {
        calls.emplace_back("get-window");
        Check(object == 0x1000u, "window owner");
        requested_name = name;
        return image;
    }
    float GetX(std::uint32_t window) override {
        calls.emplace_back("get-x");
        Check(window == image, "image X window");
        return image_x;
    }
    float GetWidth(std::uint32_t window) override {
        calls.emplace_back("get-width");
        Check(window == image, "image width window");
        return image_width;
    }
    void SetWidth(std::uint32_t window, float width) override {
        calls.emplace_back("set-width");
        width_window = window;
        set_width = width;
    }
    std::uint32_t SetX(std::uint32_t window, float x) override {
        calls.emplace_back("set-x");
        x_window = window;
        set_x = x;
        return 0xfeedu;
    }
    void SetAlwaysOnTop(std::uint32_t window,
                        bool enabled) override {
        calls.emplace_back("top");
        top_window = window;
        top = enabled;
    }
    std::uint32_t IsVisible(std::uint32_t object) override {
        calls.emplace_back("is-visible");
        Check(object == 0x1000u, "visible object");
        return visible_result;
    }
    std::uint32_t SetVisibleBase(std::uint32_t object,
                                 std::uint32_t visible,
                                 std::uint32_t option) override {
        calls.emplace_back("visible-base");
        Check(object == 0x1000u, "base visible object");
        base_visible = visible;
        base_option = option;
        return base_visible_result;
    }
    std::uint32_t DestructBase(std::uint32_t object) override {
        calls.emplace_back("destruct-base");
        Check(object == 0x1000u, "base destruct object");
        return 0xbeefu;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-word");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write-word");
        words[address] = value;
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read-float");
        return floats.at(address);
    }
    void WriteFloat(std::uint32_t address,
                    float value) override {
        calls.emplace_back("write-float");
        floats[address] = value;
    }
};

void Seed(Runtime& runtime) {
    runtime.words[0x1024u] = 0x7000u;
    runtime.words[0x1028u] = runtime.image;
    runtime.floats[0x102cu] = 10.0f;
    runtime.floats[0x1030u] = 100.0f;
    runtime.floats[0x1034u] = 0.0f;
}

void CheckConstruct() {
    Runtime runtime;
    Seed(runtime);
    Check(pal4::JumpHintProgressBar::Construct(0x1000u, runtime) ==
              0x1000u,
          "construct returns object");
    Check(runtime.layout == "JumpHintWindow" &&
          runtime.name_field == 0x1014u &&
          runtime.requested_name == "JumpHint/ImgProgess" &&
          runtime.words.at(0x1000u) ==
              pal4::JumpHintProgressBar::kVtable &&
          runtime.words.at(0x1028u) == runtime.image,
          "layout, name, vtable and image");
    Check(Near(runtime.floats.at(0x102cu), 10.0f) &&
          Near(runtime.floats.at(0x1030u), 100.0f) &&
          Near(runtime.floats.at(0x1034u), 0.0f) &&
          Near(runtime.set_width, 100.0f) &&
          Near(runtime.set_x, 10.0f) &&
          runtime.top_window == 0x7000u && runtime.top,
          "image geometry, zero progress and topmost root");
    Check(runtime.calls.front() == "load" &&
          runtime.calls.back() == "top", "constructor call order");
}

void CheckDestruct() {
    Runtime runtime;
    Check(pal4::JumpHintProgressBar::Destruct(
              0x1000u, runtime) == 0xbeefu &&
          runtime.words.at(0x1000u) ==
              pal4::JumpHintProgressBar::kVtable &&
          runtime.calls ==
              std::vector<std::string>{"write-word", "destruct-base"},
          "destructor restores vtable before base");
}

void CheckProgress() {
    Runtime runtime;
    Seed(runtime);
    Check(pal4::JumpHintProgressBar::SetProgress(
              0x1000u, 0.25f, runtime) == 0xfeedu &&
          Near(pal4::JumpHintProgressBar::GetProgress(
              0x1000u, runtime), 0.25f) &&
          runtime.width_window == runtime.image &&
          runtime.x_window == runtime.image &&
          Near(runtime.set_width, 75.0f) &&
          Near(runtime.set_x, 35.0f),
          "progress stores value and shifts image from right");
}

void CheckVisible() {
    Runtime runtime;
    Seed(runtime);
    runtime.floats[0x1034u] = 0.8f;
    Check(pal4::JumpHintProgressBar::SetVisible(
              0x1000u, 0x100u, runtime) == 0xabcdu &&
          runtime.base_visible == 0x100u &&
          runtime.base_option == 0u &&
          Near(runtime.floats.at(0x1034u), 0.8f),
          "zero low byte does not reset progress");
    Check(pal4::JumpHintProgressBar::SetVisible(
              0x1000u, 0x101u, runtime) == 0xfeedu &&
          Near(runtime.floats.at(0x1034u), 0.0f),
          "nonzero low byte resets progress");
}

void CheckUpdate() {
    Runtime runtime;
    Seed(runtime);
    runtime.visible_result = 0x100u;
    Check(pal4::JumpHintProgressBar::Update(
              0x1000u, 0.5f, runtime) == 0x100u &&
          Near(runtime.floats.at(0x1034u), 0.0f),
          "invisible update preserves result and progress");
    runtime.visible_result = 1u;
    Check(pal4::JumpHintProgressBar::Update(
              0x1000u, 0.5f, runtime) == 0xfeedu &&
          Near(runtime.floats.at(0x1034u), 0.2f),
          "visible update advances by 0.4 times delta");
    runtime.floats[0x1034u] = 1.0f;
    (void)pal4::JumpHintProgressBar::Update(0x1000u, 0.0f, runtime);
    Check(Near(runtime.floats.at(0x1034u), 1.0f),
          "one is retained");
    (void)pal4::JumpHintProgressBar::Update(0x1000u, 0.1f, runtime);
    Check(Near(runtime.floats.at(0x1034u), 0.0f),
          "past one wraps to zero");
    runtime.floats[0x1034u] = -0.2f;
    (void)pal4::JumpHintProgressBar::Update(0x1000u, 0.0f, runtime);
    Check(Near(runtime.floats.at(0x1034u), -0.2f),
          "negative progress remains negative");
    runtime.floats[0x1034u] =
        std::numeric_limits<float>::quiet_NaN();
    (void)pal4::JumpHintProgressBar::Update(0x1000u, 0.0f, runtime);
    Check(std::isnan(runtime.floats.at(0x1034u)),
          "unordered comparison retains NaN");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckDestruct();
        CheckProgress();
        CheckVisible();
        CheckUpdate();
        std::cout << "jump_hint_progress_bar_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
