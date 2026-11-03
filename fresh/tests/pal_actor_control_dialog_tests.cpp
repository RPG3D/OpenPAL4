#include "pal4/pal_actor_control_dialog.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlDialogRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> events;
    bool enabled = true;
    bool restrict_height = true;
    double property = 2.0;
    std::uint32_t iterate_result = 0x1234u;
    pal4::ActorControlEffectPoint offset{};

    void Float(std::uint32_t address, float value) {
        memory[address] = std::bit_cast<std::uint32_t>(value);
    }
    std::uint32_t IterateAndProcess(std::uint32_t actor) override {
        Check(actor == 0x1000u, "dialog iterator actor");
        events.emplace_back("iterate");
        return iterate_result;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto it = memory.find(address);
        return it == memory.end() ? 0u : it->second;
    }
    float ReadFloat(std::uint32_t address) override {
        return std::bit_cast<float>(ReadWord(address));
    }
    bool DialogEnabled(std::uint32_t dialog) override {
        Check(dialog == 0x5000u, "dialog enabled address");
        events.emplace_back("enabled");
        return enabled;
    }
    bool RestrictHeight(std::uint32_t dialog) override {
        Check(dialog == 0x5000u, "dialog height address");
        events.emplace_back("height");
        return restrict_height;
    }
    double ActorProperty(std::uint32_t actor) override {
        Check(actor == 0x1000u, "dialog actor property address");
        events.emplace_back("property");
        return property;
    }
    float PlanarMagnitude(
        pal4::ActorControlEffectPoint value) override {
        offset = value;
        events.emplace_back("distance");
        return IActorControlDialogRuntime::PlanarMagnitude(value);
    }
    void AddActiveItem(std::uint32_t dialog, float weight) override {
        Check(dialog == 0x5000u && weight == 1.5f,
              "active dialog arguments");
        events.emplace_back("active");
    }
};

void ConfigurePositions(Runtime& runtime) {
    runtime.memory[0x5000u + 440u] = 1u;
    runtime.Float(0x5000u + 172u, 1.0f);
    runtime.Float(0x5000u + 176u, 2.0f);
    runtime.Float(0x5000u + 180u, 3.0f);
    runtime.Float(0x1000u + 172u, 1.0f);
    runtime.Float(0x1000u + 176u, 3.0f);
    runtime.Float(0x1000u + 180u, 6.0f);
    runtime.Float(0x1000u + 236u, 2.0f);
}

void TestRelease() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    Check(pal4::PalActorControlDialog::Release(state, runtime) == 0u &&
              runtime.events.empty(),
          "null actor returns zero without traversal");
    state.words[1] = 0x1000u;
    Check(pal4::PalActorControlDialog::Release(state, runtime) ==
                  0xffffffffu && runtime.events.empty(),
          "zero count returns wrapped count minus one");
    state.words[19] = 2u;
    Check(pal4::PalActorControlDialog::Release(state, runtime) == 1u &&
              state.words[19] == 2u && runtime.events.empty(),
          "release does not mutate the count");
    state.words[19] = 1u;
    Check(pal4::PalActorControlDialog::Release(state, runtime) ==
                  0x1234u &&
              runtime.events == std::vector<std::string>{"iterate"},
          "count one invokes dialog traversal and forwards its return");
}

void TestVisitGatesAndDistance() {
    Runtime runtime;
    ConfigurePositions(runtime);
    runtime.memory[0x5000u + 440u] = 0u;
    Check(pal4::PalActorControlDialog::Visit(
              0x5000u, 0x1000u, runtime) == 0x5000u &&
              runtime.events.empty(),
          "missing active field skips callback");
    runtime.memory[0x5000u + 440u] = 1u;
    runtime.memory[0x5000u + 388u] = 1u;
    Check(pal4::PalActorControlDialog::Visit(
              0x5000u, 0x1000u, runtime) == 0x5000u &&
              runtime.events.empty(),
          "busy dialog skips callback");
    runtime.memory[0x5000u + 388u] = 0u;
    runtime.enabled = false;
    Check(pal4::PalActorControlDialog::Visit(
              0x5000u, 0x1000u, runtime) == 0x5000u &&
              runtime.events == std::vector<std::string>{"enabled"},
          "virtual enabled gate skips distance");
    runtime.events.clear();
    runtime.enabled = true;
    Check(pal4::PalActorControlDialog::Visit(
              0x5000u, 0x1000u, runtime) == 0x5000u &&
              runtime.offset.x == 0.0f && runtime.offset.y == 0.0f &&
              runtime.offset.z == 3.0f &&
              runtime.events.back() == "active",
          "planar magnitude selects within strict radius");
    runtime.events.clear();
    runtime.Float(0x1000u + 236u, 1.0f);
    static_cast<void>(pal4::PalActorControlDialog::Visit(
        0x5000u, 0x1000u, runtime));
    Check(runtime.events.back() == "distance",
          "equal distance does not add active dialog");
}

void TestStrictHeight() {
    Runtime runtime;
    ConfigurePositions(runtime);
    runtime.property = 1.0;
    static_cast<void>(pal4::PalActorControlDialog::Visit(
        0x5000u, 0x1000u, runtime));
    Check(runtime.events.back() == "property",
          "equal vertical tolerance rejects height-restricted dialog");
    runtime.events.clear();
    runtime.restrict_height = false;
    static_cast<void>(pal4::PalActorControlDialog::Visit(
        0x5000u, 0x1000u, runtime));
    Check(runtime.events.back() == "active" &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "property") == runtime.events.end(),
          "unrestricted dialog skips vertical property query");
    runtime.events.clear();
    runtime.restrict_height = true;
    runtime.property = std::numeric_limits<double>::quiet_NaN();
    static_cast<void>(pal4::PalActorControlDialog::Visit(
        0x5000u, 0x1000u, runtime));
    Check(runtime.events.back() == "active",
          "unordered x87 vertical comparison follows processing branch");
}

}  // namespace

int main() {
    TestRelease();
    TestVisitGatesAndDistance();
    TestStrictHeight();
}
