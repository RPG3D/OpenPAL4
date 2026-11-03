#include "pal4/click_game_window_lifecycle.h"

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

struct Runtime final : pal4::IClickGameWindowLifecycleRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t event = 0x2000u;
    static constexpr std::uint32_t root = 0x3000u;
    static constexpr std::uint32_t image = 0x4000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {object + 0x24u, root}, {object + 0x28u, image},
        {event + 0x1cu, 0u}, {event + 0x10u, 28u}};
    std::unordered_map<std::uint32_t, float> floats{
        {object + 0x2cu, 80.0f}, {object + 0x30u, 0.0f},
        {object + 0x34u, 0.0f}};
    std::vector<std::string> calls;
    std::uint8_t visible = 1u;
    std::uint32_t destroyed = 0u;
    std::uint32_t closed = 0u;
    std::uint32_t parent = 0u;
    std::uint8_t modal = 0u;
    bool resume_game = false;
    float rendered_width = 0.0f;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-word");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write-word");
        if (address == object + 0x34u) {
            Check(value == 0u, "timer reset bits");
            floats[address] = 0.0f;
        } else {
            words[address] = value;
        }
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read-float");
        return floats.at(address);
    }
    void WriteFloat(std::uint32_t address, float value) override {
        calls.emplace_back("write-float");
        floats[address] = value;
    }
    std::uint32_t DestroyBase(std::uint32_t address) override {
        destroyed = address;
        calls.emplace_back("destroy-base");
        return 0xbeefu;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    void ProcessUIEvents(std::uint32_t manager,
                         bool resume) override {
        Check(manager == 0x5000u, "UI manager");
        resume_game = resume;
        calls.emplace_back("process-events");
    }
    void SetParent(std::uint32_t address, std::uint8_t active,
                   std::uint32_t target) override {
        Check(address == object, "parent object");
        parent = target;
        modal = active;
        calls.emplace_back("set-parent");
    }
    void SetModalState(std::uint32_t window,
                       std::uint8_t active) override {
        Check(window == root && active == modal, "modal root and state");
        calls.emplace_back("set-modal");
    }
    void SetWidth(std::uint32_t window, float width) override {
        Check(window == image, "progress image");
        rendered_width = width;
        calls.emplace_back("set-width");
    }
    std::uint8_t IsVisible(std::uint32_t address) override {
        Check(address == object, "visibility object");
        calls.emplace_back("visible");
        return visible;
    }
    std::uint32_t Close(std::uint32_t address,
                        std::uint32_t first,
                        std::uint32_t second) override {
        Check(address == object && first == 0u && second == 0u,
              "close arguments");
        ++closed;
        calls.emplace_back("close");
        return 0x1234u;
    }
};

void CheckDestructionAndModal() {
    Runtime runtime;
    Check(pal4::ClickGameWindowLifecycle::Destruct(
              Runtime::object, runtime) == 0xbeefu &&
              runtime.words.at(Runtime::object) == 0x843e20u &&
              runtime.destroyed == Runtime::object &&
              runtime.calls == (std::vector<std::string>{
                  "write-word", "destroy-base"}),
          "destructor restores vtable before base destruction");

    runtime.calls.clear();
    runtime.floats[Runtime::object + 0x30u] = 0.75f;
    runtime.floats[Runtime::object + 0x34u] = 8.0f;
    pal4::ClickGameWindowLifecycle::SetModal(
        Runtime::object, 1u, 0x6000u, runtime);
    Check(!runtime.resume_game && runtime.parent == 0x6000u &&
              runtime.floats.at(Runtime::object + 0x30u) == 0.0f &&
              runtime.floats.at(Runtime::object + 0x34u) == 0.0f &&
              runtime.rendered_width == 0.0f && runtime.closed == 0u &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "process-events", "set-parent", "read-float",
                  "read-word", "write-float", "set-width", "read-word",
                  "write-word", "set-modal"}),
          "modal setup resets progress and timer in native order");

    Runtime inactive;
    pal4::ClickGameWindowLifecycle::SetModal(
        Runtime::object, 0u, 0u, inactive);
    Check(inactive.resume_game && inactive.modal == 0u,
          "nonmodal state resumes UI events");
}

void CheckTimer() {
    Runtime hidden;
    hidden.visible = 0u;
    Check(pal4::ClickGameWindowLifecycle::UpdateTimer(
              Runtime::object, 40.0, hidden) == 0u &&
              hidden.closed == 0u &&
              hidden.floats.at(Runtime::object + 0x34u) == 0.0f,
          "hidden timer does not advance");

    Runtime below;
    below.floats[Runtime::object + 0x34u] = 29.0f;
    Check(pal4::ClickGameWindowLifecycle::UpdateTimer(
              Runtime::object, 0.5, below) == 0x100u &&
              below.floats.at(Runtime::object + 0x34u) == 29.5f &&
              below.closed == 0u,
          "timer below thirty stays open");
    Check(pal4::ClickGameWindowLifecycle::UpdateTimer(
              Runtime::object, 0.5, below) == 0x1234u &&
              below.closed == 1u,
          "timer closes at thirty");

    Runtime unordered;
    Check(pal4::ClickGameWindowLifecycle::UpdateTimer(
              Runtime::object,
              std::numeric_limits<double>::quiet_NaN(), unordered) ==
              0x100u && unordered.closed == 0u,
          "unordered x87 comparison does not close timer");
}

void CheckProgress() {
    Runtime runtime;
    Check(pal4::ClickGameWindowLifecycle::UpdateProgress(
              Runtime::object, 0.5f, runtime) == 0x100u &&
              runtime.floats.at(Runtime::object + 0x30u) == 0.5f &&
              runtime.rendered_width == 40.0f && runtime.closed == 0u,
          "progress scales image width");
    Check(pal4::ClickGameWindowLifecycle::UpdateProgress(
              Runtime::object, 1.0f, runtime) == 0x1234u &&
              runtime.rendered_width == 80.0f && runtime.closed == 1u,
          "full progress closes frame");

    Runtime unordered;
    Check(pal4::ClickGameWindowLifecycle::UpdateProgress(
              Runtime::object,
              std::numeric_limits<float>::quiet_NaN(), unordered) ==
              0x100u && std::isnan(unordered.rendered_width) &&
              unordered.closed == 0u,
          "unordered x87 comparison does not close progress");
}

void CheckInput() {
    Runtime click;
    Check(pal4::ClickGameWindowLifecycle::OnProgressClick(
              Runtime::object, Runtime::event, click) == 1u &&
              click.floats.at(Runtime::object + 0x30u) > 0.027f &&
              click.rendered_width > 2.2f,
          "primary click advances progress");
    Runtime other_click;
    other_click.words[Runtime::event + 0x1cu] = 1u;
    Check(pal4::ClickGameWindowLifecycle::OnProgressClick(
              Runtime::object, Runtime::event, other_click) == 1u &&
              other_click.floats.at(Runtime::object + 0x30u) == 0.0f &&
              other_click.calls == (std::vector<std::string>{"read-word"}),
          "other click is ignored");

    Runtime key;
    Check(pal4::ClickGameWindowLifecycle::OnKeyUp(
              Runtime::object, Runtime::event, key) == 1u &&
              key.floats.at(Runtime::object + 0x30u) > 0.027f,
          "key code twenty-eight advances progress");
    Runtime other_key;
    other_key.words[Runtime::event + 0x10u] = 13u;
    Check(pal4::ClickGameWindowLifecycle::OnKeyUp(
              Runtime::object, Runtime::event, other_key) == 1u &&
              other_key.calls == (std::vector<std::string>{"read-word"}),
          "other key is ignored");
}

}  // namespace

int main() {
    try {
        CheckDestructionAndModal();
        CheckTimer();
        CheckProgress();
        CheckInput();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
