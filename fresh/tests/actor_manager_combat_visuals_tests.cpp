#include "pal4/actor_manager_combat_visuals.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kManager = 0x1000u;
constexpr std::uint32_t kParameter = 0x2000u;
constexpr std::uint32_t kDialog = 0x7000u;
constexpr std::uint32_t kHandle = kManager + 392u;

struct Runtime final : pal4::IActorManagerCombatVisualRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> dialog_results{
        kDialog, kDialog, kDialog, kDialog};
    std::size_t dialog_position = 0u;
    bool inactive = false;
    bool throw_on_properties = false;
    std::uint32_t text_result = 0xa000u;
    std::uint32_t created_name = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        trace.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint32_t CombatSystem() override {
        trace.push_back("combat-system");
        return 0x3000u;
    }
    bool CombatInactive(std::uint32_t combat) override {
        Check(combat == 0x3000u, "combat query owner");
        trace.push_back("inactive");
        return inactive;
    }
    void HandleExistingEffect(std::uint32_t handle,
                              std::uint32_t mode) override {
        Check(handle == kHandle && mode == 1u,
              "existing effect address/mode");
        trace.push_back("handle-existing");
    }
    std::uint32_t DialogData(std::uint32_t parameter) override {
        Check(parameter == kParameter &&
                  dialog_position < dialog_results.size(),
              "dialog parameter and lookup count");
        trace.push_back("dialog");
        return dialog_results[dialog_position++];
    }
    pal4::ActorManagerEffectHandle CreateEffect(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t position, std::uint32_t mode,
        std::uint32_t flags) override {
        Check(manager == kManager &&
                  position == pal4::ActorManagerCombatVisuals::kPosition &&
                  mode == 1u && flags == 0u,
              "visual effect creation constants");
        created_name = name;
        trace.push_back("effect:" + std::to_string(name));
        return {0x5010u, 0x6010u};
    }
    void ResetTemporaryEffect(
        pal4::ActorManagerEffectHandle& handle) override {
        Check(handle.id == 0x5010u && handle.owner == 0x6010u,
              "temporary effect value");
        trace.push_back("effect-reset");
        handle = {0xffffffffu, 0u};
    }
    std::uint32_t CreateText(std::uint32_t literal) override {
        Check(literal == pal4::ActorManagerCombatVisuals::kT1Text,
              "T1 text literal");
        trace.push_back("text-create");
        return text_result;
    }
    void ReleaseText(std::uint32_t text) override {
        trace.push_back("text-release:" + std::to_string(text));
    }
    void SetProperties(std::uint32_t handle, std::uint32_t text,
                       std::uint32_t source) override {
        Check(handle == kHandle && text == text_result,
              "effect properties handle/text");
        trace.push_back("set-properties:" + std::to_string(source));
        if (throw_on_properties)
            throw std::runtime_error("property setup failed");
    }
    std::uint32_t ChildPosition(std::uint32_t actor) override {
        Check(actor == kDialog, "child position dialog");
        trace.push_back("child-position");
        return 0x8000u;
    }
    std::uint32_t ChildMatrix(std::uint32_t actor) override {
        Check(actor == kDialog, "child matrix dialog");
        trace.push_back("child-matrix");
        return 0x9000u;
    }
    void UpdatePosition(std::uint32_t handle,
                        std::uint32_t position,
                        std::uint8_t mode) override {
        Check(handle == kHandle && position == 0x8000u && mode == 0u,
              "position update arguments");
        trace.push_back("update-position");
    }
    void UpdateMatrix(std::uint32_t handle, std::uint32_t matrix,
                      std::uint8_t mode) override {
        Check(handle == kHandle && matrix == 0x9000u && mode == 0u,
              "matrix update arguments");
        trace.push_back("update-matrix");
    }
    void UpdateOrientation(
        std::uint32_t handle, const std::array<float, 3>& angles,
        std::uint8_t mode) override {
        Check(handle == kHandle &&
                  angles == std::array<float, 3>{0.0f, 180.0f, 0.0f} &&
                  mode == 1u,
              "orientation update arguments");
        trace.push_back("update-orientation");
    }
    std::uint32_t FinalizeEffect(std::uint32_t handle) override {
        Check(handle == kHandle, "effect finalization handle");
        trace.push_back("finalize");
        return 0x1abu;
    }
};

void InactiveAndVariants() {
    Runtime inactive;
    inactive.inactive = true;
    Check(pal4::ActorManagerCombatVisuals::Handle(
              kManager, kParameter, inactive) == 1u &&
              inactive.trace == std::vector<std::string>{
                  "combat-system", "inactive"},
          "inactive combat exits before touching effects or dialog");

    constexpr std::array<std::uint32_t, 4> kNames{
        0x8b67a8u, 0x8b67a0u, 0x8b6798u, 0x8b6790u};
    for (std::uint32_t variant = 0u; variant < kNames.size(); ++variant) {
        Runtime runtime;
        runtime.words[kDialog + 1192u] = variant;
        Check(pal4::ActorManagerCombatVisuals::Handle(
                  kManager, kParameter, runtime) == 0xabu &&
                  runtime.created_name == kNames[variant] &&
                  runtime.words[kHandle] == 0x5010u &&
                  runtime.words[kHandle + 4u] == 0x6010u &&
                  runtime.dialog_position == 4u &&
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "effect-reset") <
                      std::find(runtime.trace.begin(), runtime.trace.end(),
                                "set-properties:" + std::to_string(
                                    kDialog + 724u)) &&
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "update-position") <
                      std::find(runtime.trace.begin(), runtime.trace.end(),
                                "update-matrix") &&
                  runtime.trace[runtime.trace.size() - 2u] ==
                      "update-orientation" &&
                  runtime.trace.back() == "finalize",
              "four dialog variants select CA15-CA18 and update transforms");
    }
}

void MissingVariantAndTextCleanup() {
    Runtime missing;
    missing.words[kDialog + 1192u] = 7u;
    missing.dialog_results[1] = 0u;
    missing.text_result = 0u;
    Check(pal4::ActorManagerCombatVisuals::Handle(
              kManager, kParameter, missing) == 0xabu &&
              missing.created_name == 0u &&
              missing.words[kHandle] == 0u &&
              std::find(missing.trace.begin(), missing.trace.end(),
                        "set-properties:0") != missing.trace.end() &&
              std::find_if(missing.trace.begin(), missing.trace.end(),
                           [](const std::string& item) {
                               return item.starts_with("text-release:");
                           }) == missing.trace.end(),
          "unknown variant leaves handle and null property source unchanged");

    Runtime throwing;
    throwing.words[kDialog + 1192u] = 0u;
    throwing.throw_on_properties = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::ActorManagerCombatVisuals::Handle(
            kManager, kParameter, throwing));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
              throwing.trace.back() == "text-release:40960",
          "temporary T1 text is released when property setup throws");
}

void ForwardEventGate() {
    Runtime inactive;
    inactive.inactive = true;
    pal4::ActorManagerCombatVisuals::ForwardEvent(kManager, inactive);
    Check(inactive.trace == std::vector<std::string>{
              "combat-system", "inactive"},
          "inactive combat suppresses effect event forwarding");

    Runtime active;
    pal4::ActorManagerCombatVisuals::ForwardEvent(kManager, active);
    Check(active.trace == std::vector<std::string>{
              "combat-system", "inactive", "handle-existing"},
          "active combat forwards mode-one event to manager effect slot");
}

}  // namespace

int main() {
    try {
        InactiveAndVariants();
        MissingVariantAndTextCleanup();
        ForwardEventGate();
        std::cout << "actor_manager_combat_visuals_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
