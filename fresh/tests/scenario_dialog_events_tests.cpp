#include "pal4/scenario_dialog.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScenarioEventRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x3000u;
    std::uint32_t owner = 0x4000u;
    std::uint32_t subscription_callback = 0xffffffffu;
    std::uint32_t display_object = 0u;

    Runtime() {
        words[0x1024u] = 0x2000u;
        words[0x4004u] = 3u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 24u, "callback size");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t SubscribeMouseUp(
        std::uint32_t window, std::uint32_t callback) override {
        Check(window == 0x2000u, "root mouse-up subscription");
        calls.emplace_back("subscribe");
        subscription_callback = callback;
        return owner;
    }
    std::uint32_t DestroyOwner(
        std::uint32_t pointer, std::uint32_t flags) override {
        Check(pointer == 0x4000u && flags == 1u,
              "owner deleting destructor");
        calls.emplace_back("destroy");
        return 0xbeefu;
    }
    void HandleTextDisplay(std::uint32_t object) override {
        calls.emplace_back("display");
        display_object = object;
    }
};

void CheckHandlers() {
    Runtime runtime;
    for (const std::uint32_t value : {0u, 1u, 0xffffffffu}) {
        runtime.calls.clear();
        runtime.words[0x201cu] = value;
        runtime.display_object = 0u;
        Check(pal4::ScenarioDialog::HandleEditboxMouseUp(
                  0x1000u, 0x2000u, runtime) == 1u &&
                  runtime.display_object ==
                      (value == 0u ? 0x1000u : 0u) &&
                  runtime.calls == (value == 0u
                      ? std::vector<std::string>{"read", "display"}
                      : std::vector<std::string>{"read"}),
              "mouse event field gate");
    }
    for (const std::uint32_t key : {27u, 28u, 29u}) {
        runtime.calls.clear();
        runtime.words[0x2010u] = key;
        runtime.display_object = 0u;
        Check(pal4::ScenarioDialog::HandleEditboxKey(
                  0x1028u, 0x2000u, runtime) == 1u &&
                  runtime.display_object ==
                      (key == 28u ? 0x1000u : 0u),
              "key 28 advances owner text");
    }
    Check(pal4::ScenarioDialog::AlwaysTrue(0u) == 1u &&
              pal4::ScenarioDialog::AlwaysTrue(0xffffffffu) == 1u,
          "no-op callback returns one");
}

void CheckSubscription(std::uint32_t allocation,
                       std::uint32_t owner,
                       std::uint32_t initial_count,
                       std::uint32_t expected_result) {
    Runtime runtime;
    runtime.allocation = allocation;
    runtime.owner = owner;
    runtime.words[0x4004u] = initial_count;
    Check(pal4::ScenarioDialog::RegisterEditboxEvent(
              0x1000u, runtime) == expected_result &&
              runtime.subscription_callback == allocation,
          "subscription return and callback pointer");
    if (allocation != 0u) {
        Check(runtime.words.at(0x3008u) == 0x4b7fb0u &&
                  runtime.words.at(0x300cu) == 0u &&
                  runtime.words.at(0x3010u) == 0x1000u &&
                  runtime.words.at(0x3000u) == 0x844334u,
              "callback fields and vtable");
    }
    if (owner != 0u) {
        Check(runtime.words.at(0x4004u) == initial_count - 1u,
              "owner reference decrement");
        Check(runtime.calls.back() ==
                  (initial_count == 1u ? "destroy" : "write"),
              "owner release branch");
    } else {
        Check(runtime.calls.back() == "subscribe",
              "null owner has no release");
    }
}

}  // namespace

int main() {
    try {
        CheckHandlers();
        CheckSubscription(0x3000u, 0x4000u, 3u, 0x4004u);
        CheckSubscription(0x3000u, 0x4000u, 1u, 0xbeefu);
        CheckSubscription(0u, 0x4000u, 1u, 0xbeefu);
        CheckSubscription(0x3000u, 0u, 0u, 0u);
        std::cout << "scenario_dialog_events_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
