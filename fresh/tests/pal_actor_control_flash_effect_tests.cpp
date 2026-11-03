#include "pal4/pal_actor_control_flash_effect.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlFlashRuntime {
    std::vector<std::string> events;
    std::vector<bool> need_results;
    std::uint32_t* expected_handle{};
    std::uint32_t property_context{};
    std::uint32_t updated_property{};

    bool NeedsUpdate(std::uint32_t* handle) override {
        Check(handle == expected_handle, "effect handle offset");
        events.emplace_back("need");
        const bool result = need_results.front();
        need_results.erase(need_results.begin());
        return result;
    }
    void HandleEvent(std::uint32_t* handle, std::uint32_t event) override {
        Check(handle == expected_handle && event == 0u,
              "handle event arguments");
        events.emplace_back("event");
    }
    pal4::ActorControlEffectHandle CreateEffect(
        std::string_view name, std::uint32_t parameter_address,
        std::uint32_t enabled, std::uint32_t extra) override {
        Check(name == "flash" &&
                  parameter_address == pal4::PalActorControlFlashEffect::kParameterAddress &&
                  enabled == 1u && extra == 0u,
              "flash effect creation arguments");
        events.emplace_back("create");
        return {0x1234u, 0x5678u};
    }
    void ResetTemporary(pal4::ActorControlEffectHandle& temporary) override {
        Check(temporary.object == 0x1234u && temporary.token == 0x5678u &&
                  expected_handle[0] == temporary.object &&
                  expected_handle[1] == temporary.token,
              "handle must be stored before temporary reset");
        IActorControlFlashRuntime::ResetTemporary(temporary);
        Check(temporary.object == 0xffffffffu && temporary.token == 0u,
              "temporary handle reset values");
        events.emplace_back("reset");
    }
    std::uint32_t ReadEffectProperty(std::uint32_t context_address) override {
        property_context = context_address;
        events.emplace_back("read");
        return 0x9abcu;
    }
    void UpdateProperties(std::uint32_t* handle,
                          std::uint32_t property) override {
        Check(handle == expected_handle, "property update handle");
        updated_property = property;
        events.emplace_back("update");
    }
};

}  // namespace

int main() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    runtime.expected_handle = state.words.data() + 279u;
    Check(!pal4::PalActorControlFlashEffect::Update(state, 0u, runtime) &&
              runtime.events.empty(),
          "null context exits before effect calls");

    runtime.need_results = {true};
    Check(pal4::PalActorControlFlashEffect::Update(state, 0x2000u, runtime) &&
              runtime.events == std::vector<std::string>{"need", "event"},
          "non-flash mode processes pending handle event");

    runtime = {};
    runtime.expected_handle = state.words.data() + 279u;
    runtime.need_results = {false, true};
    state.words[33u] = 2u;
    Check(pal4::PalActorControlFlashEffect::Update(state, 0x3000u, runtime) &&
              state.words[279u] == 0x1234u &&
              state.words[280u] == 0x5678u &&
              runtime.property_context == 0x3000u &&
              runtime.updated_property == 0x9abcu &&
              runtime.events == std::vector<std::string>{
                  "need", "create", "reset", "need", "read", "update"},
          "flash mode creates handle and updates its property");

    runtime = {};
    runtime.expected_handle = state.words.data() + 279u;
    runtime.need_results = {true, false};
    Check(pal4::PalActorControlFlashEffect::Update(state, 0x4000u, runtime) &&
              runtime.events == std::vector<std::string>{
                  "need", "event", "create", "reset", "need"},
          "second needs-update gate skips property read");
}
