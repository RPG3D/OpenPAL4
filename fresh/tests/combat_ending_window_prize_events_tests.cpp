#include "pal4/combat_ending_window_prize_events.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kWindow = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IEventSubscriptionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t group = 0u;
    std::uint32_t returned_handle = 0x3000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t EventKey(std::string_view name) override {
        Check(name == "combat_prize", "ending event name");
        calls.emplace_back("key");
        return 0x2000u;
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x4000u;
    }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t key, std::uint32_t owner,
                           std::uint32_t callback) override {
        Check(router == 0x4000u && mode == 2u && key == 0x2000u &&
                  owner == kWindow && callback == 0x545ae0u,
              "ending event registration arguments");
        calls.emplace_back("register");
        return returned_handle;
    }
    std::uint32_t FindEventGroup(std::uint32_t router,
                                 std::uint32_t key) override {
        Check(router == 0x4000u && key == 0x2000u,
              "ending event group lookup");
        calls.emplace_back("group");
        return group;
    }
    void ReleaseOwnedSlot(std::uint32_t slot) override {
        Check(slot == 0x5108u, "ending event payload slot");
        calls.emplace_back("release");
    }
    void FreeNode(std::uint32_t node) override {
        Check(node == 0x5100u, "ending event node address");
        calls.emplace_back("free");
    }
};

void CheckInitialize() {
    Runtime runtime;
    runtime.words[kWindow + 0x3cu] = 0x9999u;
    Check(pal4::CombatEndingWindowPrizeEvents::Initialize(
              kWindow, runtime) == 0x3000u &&
              runtime.words[kWindow + 0x38u] == 0x3000u &&
              runtime.words[kWindow + 0x3cu] == 0x2000u &&
              runtime.calls ==
                  (std::vector<std::string>{
                      "key", "router", "register", "write:4156",
                      "write:4152"}),
          "prize subscription stores key before handler and returns it");
    Runtime failed;
    failed.returned_handle = 0u;
    Check(pal4::CombatEndingWindowPrizeEvents::Initialize(
              kWindow, failed) == 0u &&
              failed.words[kWindow + 0x38u] == 0u &&
              failed.words[kWindow + 0x3cu] == 0x2000u,
          "failed prize registration still records the event key");
}

void CheckCleanup() {
    Runtime runtime;
    runtime.words[kWindow + 0x38u] = 0x3000u;
    runtime.words[kWindow + 0x3cu] = 0x2000u;
    runtime.group = 0x6000u;
    runtime.words[0x6004u] = 0x5000u;
    runtime.words[0x6008u] = 1u;
    runtime.words[0x5000u] = 0x5100u;
    runtime.words[0x5004u] = 0x5100u;
    runtime.words[0x5100u] = 0x5000u;
    runtime.words[0x5104u] = 0x5000u;
    runtime.words[0x510cu] = 0x3000u;
    Check(pal4::CombatEndingWindowPrizeEvents::Cleanup(
              kWindow, runtime) == 0u &&
              runtime.words[0x5000u] == 0x5000u &&
              runtime.words[0x5004u] == 0x5000u &&
              runtime.words[0x6008u] == 0u &&
              runtime.words[kWindow + 0x38u] == 0u &&
              runtime.words[kWindow + 0x3cu] == 0u,
          "prize cleanup unlinks matching handler and clears slot pair");
    Check(runtime.calls[4] == "release" && runtime.calls[5] == "free",
          "prize cleanup releases callback before freeing node");

    Runtime missing;
    missing.words[kWindow + 0x38u] = 0x3000u;
    missing.words[kWindow + 0x3cu] = 0x2000u;
    Check(pal4::CombatEndingWindowPrizeEvents::Cleanup(
              kWindow, missing) == 0u &&
              missing.words[kWindow + 0x38u] == 0u &&
              missing.words[kWindow + 0x3cu] == 0u,
          "prize cleanup clears a handle even without its event group");
    Runtime inactive;
    inactive.words[kWindow + 0x3cu] = 0x9999u;
    Check(pal4::CombatEndingWindowPrizeEvents::Cleanup(
              kWindow, inactive) == 0u &&
              inactive.words[kWindow + 0x3cu] == 0x9999u,
          "inactive prize cleanup retains the old key");
}

void CheckReinitialize() {
    Runtime runtime;
    runtime.words[kWindow + 0x38u] = 0x4444u;
    runtime.words[kWindow + 0x3cu] = 0x2000u;
    Check(pal4::CombatEndingWindowPrizeEvents::Initialize(
              kWindow, runtime) == 0x3000u &&
              runtime.calls.front() == "router" &&
              runtime.calls[1] == "group" &&
              runtime.words[kWindow + 0x38u] == 0x3000u,
          "prize reinitialization cleans the old handle first");
}

}  // namespace

int main() {
    try {
        CheckInitialize();
        CheckCleanup();
        CheckReinitialize();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
