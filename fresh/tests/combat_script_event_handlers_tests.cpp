#include "pal4/combat_script_event_handlers.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kOwner = 0x1000u;
constexpr std::uint32_t kRouter = 0x2000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::uint32_t mode;
    std::uint32_t descriptor;
    std::uint32_t owner;
    std::uint32_t callback;
};

struct Runtime final : pal4::ICombatScriptEventHandlerRuntime {
    std::vector<std::string> order;
    std::vector<Call> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t router_calls = 0u;
    std::uint32_t fail_at = 0u;

    void RemoveAll(std::uint32_t owner) override {
        Check(owner == kOwner, "old registrations are removed for owner");
        order.emplace_back("remove");
    }
    std::uint32_t GetRouter() override {
        ++router_calls;
        order.emplace_back("router");
        return kRouter;
    }
    std::uint32_t Register(
        std::uint32_t router, std::uint32_t mode,
        std::uint32_t descriptor, std::uint32_t owner,
        std::uint32_t callback) override {
        Check(router == kRouter, "registration uses router instance");
        order.emplace_back("register");
        calls.push_back({mode, descriptor, owner, callback});
        if (calls.size() == fail_at) throw std::runtime_error("register failed");
        return 0xa000u + static_cast<std::uint32_t>(calls.size()) * 0x10u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        order.emplace_back("write:" + std::to_string(address));
    }
};

void RegistrationOrderAndLayout() {
    constexpr std::array<std::uint32_t, 17> descriptors{
        0x8b8b8cu, 0x8b596cu, 0x8b5980u, 0x8b597cu, 0x8b5984u,
        0x8b5998u, 0x8b5970u, 0x8b5974u, 0x8b594cu, 0x8b5968u,
        0x8b5978u, 0x8b5964u, 0x8b5960u, 0x8b598cu, 0x8b5988u,
        0x8b8b88u, 0x8b62f0u};
    constexpr std::array<std::uint32_t, 17> callbacks{
        0x574970u, 0x574600u, 0x574650u, 0x5746a0u, 0x5746f0u,
        0x574740u, 0x574790u, 0x5747e0u, 0x574b90u, 0x574b60u,
        0x574b40u, 0x574800u, 0x574830u, 0x574860u, 0x574890u,
        0x5748c0u, 0x574b10u};
    Runtime runtime;
    Check(pal4::CombatScriptEventHandlers::Initialize(kOwner, runtime) ==
              0xa110u && runtime.calls.size() == 17u &&
              runtime.router_calls == 17u &&
              runtime.order.front() == "remove",
          "initializer removes old handlers and registers all 17");
    for (std::uint32_t index = 0u; index < descriptors.size(); ++index) {
        const auto& call = runtime.calls[index];
        const auto entry = kOwner + index * 8u;
        const auto handle = 0xa000u + (index + 1u) * 0x10u;
        Check(call.mode == (index == 0u ? 1u : 3u) &&
                  call.descriptor == descriptors[index] &&
                  call.owner == kOwner &&
                  call.callback == callbacks[index] &&
                  runtime.words[entry] == handle &&
                  runtime.words[entry + 4u] == descriptors[index],
              "each slot preserves IDA registration parameters and result");
        const auto offset = 1u + index * 4u;
        Check(runtime.order[offset] == "router" &&
                  runtime.order[offset + 1u] == "register" &&
                  runtime.order[offset + 2u] == "write:" +
                      std::to_string(entry + (index == 16u ? 4u : 0u)) &&
                  runtime.order[offset + 3u] == "write:" +
                      std::to_string(entry + (index == 16u ? 0u : 4u)),
              "registration and slot writes use native order");
    }
}

void PartialFailure() {
    Runtime runtime;
    runtime.fail_at = 3u;
    bool threw = false;
    try {
        (void)pal4::CombatScriptEventHandlers::Initialize(kOwner, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls.size() == 3u &&
              runtime.words.contains(kOwner) &&
              runtime.words.contains(kOwner + 8u) &&
              !runtime.words.contains(kOwner + 16u),
          "failed registration leaves only completed pairs written");
}

}  // namespace

int main() {
    try {
        RegistrationOrderAndLayout();
        PartialFailure();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
