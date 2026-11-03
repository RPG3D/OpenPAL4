#include "pal4/base_object_activity.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Event = std::pair<char, std::uint32_t>;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IBaseObjectActivityRuntime {
    std::vector<Event> events;
    bool first_dialog{};
    bool second_dialog{};
    bool main_loaded{};
    bool shadow_loaded{};
    bool fail_process{};
    bool rendered{};
    unsigned game_calls{};

    void ApplyMaterialTexture(pal4::BaseObjectState&) override {
        events.emplace_back('A', 0u);
    }
    void CleanupObject(pal4::BaseObjectState&) override {
        events.emplace_back('C', 0u);
    }
    bool DialogNeedsUpdate(pal4::BaseObjectState&,
                           std::uint32_t offset) override {
        events.emplace_back('N', offset);
        return offset == 0x534u ? first_dialog : second_dialog;
    }
    void UpdateDialog(pal4::BaseObjectState&,
                      std::uint32_t offset) override {
        events.emplace_back('U', offset);
    }
    void CleanupAnimationData(pal4::BaseObjectState& state,
                              std::uint32_t offset) override {
        Check(state.Read(0x530u) == 0u,
              "reset clears hook count before animation cleanup");
        events.emplace_back('D', offset);
    }
    std::uint32_t GameInstance() override {
        events.emplace_back('G', ++game_calls);
        return game_calls == 1u ? 0x7000u : 0x8000u;
    }
    void CleanupClump(pal4::BaseObjectState&,
                      std::uint32_t resource) override {
        events.emplace_back('L', resource);
    }
    std::uint32_t GetProperty(pal4::BaseObjectState& state) override {
        Check(state.Read(0x1f0u) == 0xaaaaaaaau,
              "reset reads property before clearing trailing fields");
        events.emplace_back('P', 0u);
        return 0x12345678u;
    }
    std::string CreateResourcePath() override {
        events.emplace_back('B', 0u);
        return "dir\\";
    }
    bool LoadClump(pal4::BaseObjectState& state, std::uint32_t kind,
                   std::string_view path, std::string_view model,
                   std::uint32_t resource, std::uint32_t option,
                   std::uint32_t zero, std::uint32_t one) override {
        Check(kind == 7u && path == "dir\\hero\\" && model == "hero" &&
                  resource == 0x71acu && option == 9u &&
                  zero == 0u && one == 1u,
              "main clump receives exact path and forwarding arguments");
        events.emplace_back('M', resource);
        state.Write(0x14u, 0x3333u);
        return main_loaded;
    }
    void HideHookAtomics(std::uint32_t clump) override {
        events.emplace_back('H', clump);
    }
    void LoadAnimation(pal4::BaseObjectState&, std::uint32_t clump,
                       std::string_view path, std::string_view model) override {
        Check(path == "dir\\hero\\" && model == "hero",
              "animation receives the constructed path and model");
        events.emplace_back('T', clump);
    }
    std::uint32_t HookCount(std::uint32_t clump,
                            std::string_view name) override {
        Check(name == "hook_ef_bottom", "hook lookup uses the IDA string");
        events.emplace_back('K', clump);
        return 5u;
    }
    bool LoadShadowClump(pal4::BaseObjectState& state,
                          std::uint32_t resource) override {
        events.emplace_back('S', resource);
        state.Write(0x30u, 0x4444u);
        return shadow_loaded;
    }
    void AttachAnimation(pal4::BaseObjectState&,
                          std::uint32_t shadow) override {
        events.emplace_back('F', shadow);
    }
    void ProcessAtomics(pal4::BaseObjectState& state) override {
        Check(state.Read(0x608u) == 0u && state.Read(0x604u) == 0u &&
                  state.Read(0x550u) == 0u && state.Read(0x560u) == 0u,
              "initialize clears selected fields before atomic processing");
        events.emplace_back('X', 0u);
    }
    void ProcessAtomics2(pal4::BaseObjectState&) override {
        events.emplace_back('Y', 0u);
        if (fail_process) throw std::runtime_error("process failure");
    }
    std::uint32_t CleanupPath(std::string& path) override {
        Check(path == "dir\\hero\\", "temporary path remains intact until cleanup");
        events.emplace_back('Z', 0u);
        return 0x99u;
    }
    bool RenderClump(pal4::BaseObjectState&,
                     std::uint32_t argument) override {
        events.emplace_back('R', argument);
        return rendered;
    }
    void ProcessAnimation(pal4::BaseObjectState&,
                          std::uint32_t offset,
                          std::uint32_t argument) override {
        Check(offset == 0x170u && argument == 0x22u,
              "animation receives embedded state and update argument");
        events.emplace_back('I', offset);
    }
    std::uint32_t UpdateCharacter(pal4::BaseObjectState& state,
                                  std::uint32_t offset,
                                  std::uint32_t address) override {
        Check(offset == 0x564u && address == 0x6000u &&
                  state.Read(offset) == 0x13579bdfu,
              "character virtual call receives embedded object and owner");
        events.emplace_back('V', offset);
        return 0xbeefu;
    }
    std::uint32_t SetPropertyVirtual(pal4::BaseObjectState& state,
                                     std::uint32_t address,
                                     std::uint32_t value,
                                     std::uint32_t mode) override {
        Check(state.Read(0u) == 0x8433d8u && address == 0x6000u &&
                  value == 0xfedcba98u && mode == 0u,
              "property setter forwards owner, value and zero to slot +4");
        events.emplace_back('Q', value);
        return 0x2468u;
    }
};

void CheckReset() {
    pal4::BaseObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    runtime.first_dialog = true;
    Check(pal4::BaseObjectActivity::Reset(state, runtime) == 0x12345678u,
          "reset forwards property result");
    Check(runtime.events == std::vector<Event>{
              {'A', 0u}, {'C', 0u}, {'N', 0x534u}, {'U', 0x534u},
              {'N', 0x53cu}, {'D', 0x170u}, {'G', 1u},
              {'L', 0x71acu}, {'P', 0u}},
          "reset applies material, dialog, animation and clump steps in order");
    for (auto offset : {0x530u, 0x1f0u, 0x608u, 0x604u, 0x550u, 0x560u}) {
        Check(state.Read(offset) == 0u,
              "reset clears exactly the selected final fields");
    }
    Check(state.Read(0x200u) == 0xaaaaaaaau,
          "reset retains unrelated state");

    runtime.events.clear();
    runtime.game_calls = 0u;
    runtime.first_dialog = false;
    runtime.second_dialog = true;
    state.Write(0x1f0u, 0xaaaaaaaau);
    (void)pal4::BaseObjectActivity::Reset(state, runtime);
    Check(runtime.events[2] == Event{'N', 0x534u} &&
              runtime.events[3] == Event{'N', 0x53cu} &&
              runtime.events[4] == Event{'U', 0x53cu},
          "second dialog is independently updated");
}

void CheckInitialize(bool main_loaded, bool shadow_loaded) {
    pal4::BaseObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    runtime.main_loaded = main_loaded;
    runtime.shadow_loaded = shadow_loaded;
    Check(pal4::BaseObjectActivity::Initialize(
              state, 7u, "hero", 9u, runtime) == 0x99u,
          "initialize returns temporary path cleanup result");
    std::vector<Event> expected{{'B', 0u}, {'G', 1u}, {'M', 0x71acu}};
    if (main_loaded) {
        expected.insert(expected.end(), {{'H', 0x3333u}, {'T', 0x3333u},
                                         {'K', 0x3333u}});
    }
    expected.insert(expected.end(), {{'G', 2u}, {'S', 0x81acu}});
    if (shadow_loaded) {
        expected.insert(expected.end(), {{'H', 0x4444u}, {'F', 0x4444u}});
    }
    expected.insert(expected.end(), {{'X', 0u}, {'Y', 0u}, {'Z', 0u}});
    Check(runtime.events == expected,
          "main and shadow load branches preserve complete call order");
    Check(state.Read(0x530u) == (main_loaded ? 5u : 0xaaaaaaaau),
          "hook count only changes when the main model loads");
}

void CheckInitializationFailure() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    runtime.fail_process = true;
    bool threw = false;
    try {
        (void)pal4::BaseObjectActivity::Initialize(
            state, 7u, "hero", 9u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events.back() == Event{'Z', 0u},
          "temporary path is cleaned after a late load failure");
}

void CheckStateUpdate() {
    pal4::BaseObjectState state{};
    state.words.fill(0xa5a5a5a5u);
    state.Write(0x564u, 0x13579bdfu);
    Runtime runtime;
    Check(pal4::BaseObjectActivity::SetStateFlag(state, 0u) == 0u &&
              state.Read(0x1f0u) == 0u &&
              pal4::BaseObjectActivity::UpdateState(
                  state, 0x6000u, 0x22u, runtime) == 0u &&
              runtime.events.empty(),
          "zero state flag bypasses rendering and character callback");

    Check(pal4::BaseObjectActivity::SetStateFlag(
              state, 0xffffffffu) == 0xffffffffu &&
              state.Read(0x1f0u) == 0xffffffffu,
          "state setter preserves all bits and returns the written value");
    Check(pal4::BaseObjectActivity::UpdateState(
              state, 0x6000u, 0x22u, runtime) == 0xbeefu &&
              runtime.events == std::vector<Event>{{'R', 0x22u},
                                                   {'V', 0x564u}},
          "render failure skips animation but still updates character");
    runtime.events.clear();
    runtime.rendered = true;
    Check(pal4::BaseObjectActivity::UpdateState(
              state, 0x6000u, 0x22u, runtime) == 0xbeefu &&
              runtime.events == std::vector<Event>{{'R', 0x22u},
                                                   {'I', 0x170u},
                                                   {'V', 0x564u}},
          "successful render updates animation before character callback");
}

void CheckPropertyForwarding() {
    pal4::BaseObjectState state{};
    state.Write(0u, 0x8433d8u);
    Runtime runtime;
    Check(pal4::BaseObjectActivity::SetProperty(
              state, 0x6000u, 0xfedcba98u, runtime) == 0x2468u &&
              runtime.events == std::vector<Event>{{'Q', 0xfedcba98u}},
          "property setter returns the virtual slot's result unchanged");
}

}  // namespace

int main() {
    CheckReset();
    for (bool main_loaded : {false, true}) {
        for (bool shadow_loaded : {false, true}) {
            CheckInitialize(main_loaded, shadow_loaded);
        }
    }
    CheckInitializationFailure();
    CheckStateUpdate();
    CheckPropertyForwarding();
}
