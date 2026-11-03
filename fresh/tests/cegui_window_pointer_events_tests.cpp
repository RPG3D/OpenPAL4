#include "pal4/cegui_window_pointer_events.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICeguiWindowPointerRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::unordered_map<std::uint32_t, pal4::CeguiPoint> parent_sizes;
    std::vector<std::string> calls;
    std::vector<double> aligned_inputs;
    pal4::CeguiMouseEvent first_event{};
    pal4::CeguiMouseEvent second_event{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read_word");
        return words[address];
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read_float");
        return floats.at(address);
    }
    pal4::CeguiPoint ParentSize(std::uint32_t window) override {
        calls.emplace_back("parent_size");
        return parent_sizes.at(window);
    }
    double PixelAlignOffset() override {
        calls.emplace_back("align_offset");
        return 0.5;
    }
    std::int32_t FloatToInt(double value) override {
        calls.emplace_back("float_to_int");
        aligned_inputs.push_back(value);
        return static_cast<std::int32_t>(value);
    }
    std::uint32_t MouseEventVtable() override {
        calls.emplace_back("event_vtable");
        return 0x841234u;
    }
    std::uint32_t InvokeWindowEvent(
        std::uint32_t window, std::uint32_t slot,
        pal4::CeguiMouseEvent& event) override {
        Check(window == 0x2000u, "mouse event window");
        if (slot == 180u) {
            calls.emplace_back("mouse_180");
            first_event = event;
            event.handled = 1u;
            return 0x1111u;
        }
        Check(slot == 184u, "mouse event second slot");
        calls.emplace_back("mouse_184");
        second_event = event;
        return 0x2222u;
    }
};

void Configure(Runtime& runtime) {
    runtime.parent_sizes[0x1000u] = {100.0f, 80.0f};
    runtime.floats[0x1000u + 236u] = 0.25f;
    runtime.floats[0x1000u + 240u] = 3.0f;
    runtime.floats[0x1000u + 244u] = 0.5f;
    runtime.floats[0x1000u + 248u] = 4.0f;
    runtime.words[0x1000u + 68u] = 0u;

    runtime.parent_sizes[0x2000u] = {50.0f, 50.0f};
    runtime.floats[0x2000u + 236u] = 0.5f;
    runtime.floats[0x2000u + 240u] = 1.0f;
    runtime.floats[0x2000u + 244u] = 0.2f;
    runtime.floats[0x2000u + 248u] = 2.0f;
    runtime.floats[0x2000u + 268u] = 20.0f;
    runtime.floats[0x2000u + 272u] = 30.0f;
    runtime.words[0x2000u + 68u] = 0x1000u;
}

void CheckPosition() {
    Runtime runtime;
    Configure(runtime);
    const auto root = pal4::CeguiWindowPointerEvents::AbsolutePosition(
        0x1000u, runtime);
    Check(root.x == 28.0f && root.y == 44.0f,
          "root aligned coordinates and pixel offsets");
    runtime.aligned_inputs.clear();
    const auto child = pal4::CeguiWindowPointerEvents::AbsolutePosition(
        0x2000u, runtime);
    Check(child.x == 54.0f && child.y == 56.0f,
          "child absolute coordinates include parent origin");
    Check(runtime.aligned_inputs == std::vector<double>{
              25.5, 10.5, 25.5, 40.5},
          "both axes use injected pixel alignment conversion");
}

void CheckMouseEvent() {
    Runtime runtime;
    Configure(runtime);
    Check(pal4::CeguiWindowPointerEvents::HandleMouseAtCenter(
              0x2000u, runtime) == 0x2222u,
          "second virtual call return forwarded");
    const auto& event = runtime.first_event;
    Check(event.vtable == 0x841234u && event.handled == 0u &&
          event.window == 0x2000u &&
          event.position.x == 64.0f && event.position.y == 71.0f,
          "mouse event vtable, window, center and initial handled flag");
    Check(event.move_delta.x == 0.0f && event.move_delta.y == 0.0f &&
          event.button == 0u && event.state_a == 1u &&
          event.state_b == 0u && event.state_c == 1u,
          "mouse event fixed fields");
    Check(runtime.second_event.handled == 1u,
          "second virtual call sees first call mutation");
    Check(runtime.calls[runtime.calls.size() - 2u] == "mouse_180" &&
          runtime.calls.back() == "mouse_184",
          "mouse event dispatch order");
}

}  // namespace

int main() {
    try {
        CheckPosition();
        CheckMouseEvent();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
