#include "pal4/smith_window.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Event {
    std::uint32_t key;
    std::uint32_t first;
    std::uint32_t second;
    bool operator==(const Event&) const = default;
};

struct Runtime final : pal4::ISmithWindowCloseConfirmRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t event = 0x2000u;
    std::uint32_t event_value = 1u;
    std::uint8_t base_result = 0xa5u;
    bool active = false;
    std::uint32_t saved_object = 0u;
    std::vector<Event> events;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == event + 0x10u, "event value offset");
        calls.emplace_back("read-event");
        return event_value;
    }
    std::uint8_t HandleBaseKeyEvent(
        std::uint32_t owner, std::uint32_t source) override {
        Check(owner == object && source == event,
              "base handler arguments");
        calls.emplace_back("base-handler");
        return base_result;
    }
    bool SmithActive() override {
        calls.emplace_back("smith-active");
        return active;
    }
    void ClearModelPreview(std::uint32_t trade_object) override {
        saved_object = trade_object;
        calls.emplace_back("save-empty");
    }
    void ProcessUIEvent(std::uint32_t key,
                        std::uint32_t first,
                        std::uint32_t second) override {
        events.push_back({key, first, second});
        calls.emplace_back("ui-event");
    }
};

void CheckClose() {
    Runtime inactive;
    Check(pal4::SmithWindow::OnCloseConfirm(
              Runtime::object, Runtime::event, inactive) == 1u &&
              inactive.saved_object == Runtime::object - 0x28u &&
              inactive.events == (std::vector<Event>{{0x8a80b4u, 0u, 0u}}) &&
              inactive.calls == (std::vector<std::string>{
                  "read-event", "smith-active", "save-empty", "ui-event"}),
          "inactive close saves data and sends one event");

    Runtime active;
    active.active = true;
    Check(pal4::SmithWindow::OnCloseConfirm(
              Runtime::object, Runtime::event, active) == 1u &&
              active.events == (std::vector<Event>{
                  {0x8b0f1cu, 0u, 0u}, {0x8a80acu, 1u, 0u}}) &&
              active.calls == (std::vector<std::string>{
                  "read-event", "smith-active", "save-empty",
                  "ui-event", "ui-event"}),
          "active close sends ordered state and toolbar events");
}

void CheckBaseDelegation() {
    Runtime runtime;
    runtime.event_value = 2u;
    Check(pal4::SmithWindow::OnCloseConfirm(
              Runtime::object, Runtime::event, runtime) == 0xa5u &&
              runtime.saved_object == 0u && runtime.events.empty() &&
              runtime.calls == (std::vector<std::string>{
                  "read-event", "base-handler"}),
          "other event delegates and forwards base result");
}

}  // namespace

int main() {
    try {
        CheckClose();
        CheckBaseDelegation();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
