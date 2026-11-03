#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerFrameEventsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::vector<std::uint32_t> freed;
    bool throw_on_event = false;
    bool throw_on_payload = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void CleanupEventNode(std::uint32_t list,
                          std::uint32_t node) override {
        Check(list == 0x500cu && (node == 0x7100u || node == 0x7200u),
              "event cleanup receives embedded list and node");
        events.emplace_back("event");
        if (throw_on_event) throw std::runtime_error("event failed");
    }
    void FreeNodePayload(std::uint32_t payload,
                         std::uint32_t flag) override {
        Check(flag == 0u &&
                  (payload == 0x8108u || payload == 0x8208u),
              "payload address and destruction flag");
        events.emplace_back("payload");
        if (throw_on_payload) throw std::runtime_error("payload failed");
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
        events.emplace_back("free");
    }
};

void Configure(Runtime& runtime, bool populated) {
    runtime.words[0x5010u] = 0x7000u;
    runtime.words[0x5014u] = populated ? 2u : 0u;
    runtime.words[0x7000u] = populated ? 0x7100u : 0x7000u;
    if (populated) {
        runtime.words[0x7100u] = 0x7200u;
        runtime.words[0x7200u] = 0x7000u;
    }
    runtime.words[0x5004u] = 0x8000u;
    runtime.words[0x5008u] = populated ? 2u : 0u;
    runtime.words[0x8000u] = populated ? 0x8100u : 0x8000u;
    runtime.words[0x8004u] = populated ? 0x8200u : 0x8000u;
    if (populated) {
        runtime.words[0x8100u] = 0x8200u;
        runtime.words[0x8104u] = 0x8000u;
        runtime.words[0x8200u] = 0x8000u;
        runtime.words[0x8204u] = 0x8100u;
    }
}

void CheckPopulated() {
    Runtime runtime;
    Configure(runtime, true);
    Check(pal4::UiFrameManagerSingleton::CleanupFrameEvents(
              0x5000u, runtime) == 0u, "cleanup returns zero");
    Check(runtime.events == std::vector<std::string>{
              "event", "event", "free", "payload", "free",
              "payload", "free", "free"},
          "event list precedes circular list and frees");
    Check(runtime.freed ==
              std::vector<std::uint32_t>{
                  0x7000u, 0x8100u, 0x8200u, 0x8000u},
          "event head and circular-list nodes released in order");
    Check(runtime.words[0x8000u] == 0x8000u &&
              runtime.words[0x8004u] == 0x8000u,
          "circular head is relinked to itself before release");
    Check(runtime.words[0x5010u] == 0u &&
              runtime.words[0x5014u] == 0u &&
              runtime.words[0x5004u] == 0u &&
              runtime.words[0x5008u] == 0u,
          "both list pointers and counts cleared");
}

void CheckEmptyAndExceptions() {
    Runtime empty;
    Configure(empty, false);
    Check(pal4::UiFrameManagerSingleton::CleanupFrameEvents(
              0x5000u, empty) == 0u &&
              empty.freed ==
                  std::vector<std::uint32_t>{0x7000u, 0x8000u} &&
              empty.words[0x5010u] == 0u &&
              empty.words[0x5004u] == 0u,
          "empty lists release only their heads");

    Runtime event_failure;
    Configure(event_failure, true);
    event_failure.throw_on_event = true;
    bool caught = false;
    try {
        (void)pal4::UiFrameManagerSingleton::CleanupFrameEvents(
            0x5000u, event_failure);
    } catch (const std::runtime_error& error) {
        caught = std::string(error.what()) == "event failed";
    }
    Check(caught && event_failure.freed.empty() &&
              event_failure.words[0x5010u] == 0x7000u,
          "event failure stops before either head is freed");

    Runtime payload_failure;
    Configure(payload_failure, true);
    payload_failure.throw_on_payload = true;
    caught = false;
    try {
        (void)pal4::UiFrameManagerSingleton::CleanupFrameEvents(
            0x5000u, payload_failure);
    } catch (const std::runtime_error& error) {
        caught = std::string(error.what()) == "payload failed";
    }
    Check(caught && payload_failure.freed ==
                        std::vector<std::uint32_t>{0x7000u} &&
              payload_failure.words[0x8000u] == 0x8200u &&
              payload_failure.words[0x8204u] == 0x8000u &&
              payload_failure.words[0x5008u] == 2u,
          "first node unlinks before payload failure; count stays unchanged");
}

}  // namespace

int main() {
    try {
        CheckPopulated();
        CheckEmptyAndExceptions();
        std::cout << "ui_frame_manager_frame_events_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
