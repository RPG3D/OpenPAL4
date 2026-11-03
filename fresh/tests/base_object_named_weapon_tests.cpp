#include "pal4/base_object_named_weapon.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
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

struct Runtime final : pal4::IBaseObjectNamedWeaponRuntime {
    pal4::BaseObjectHand hand{pal4::BaseObjectHand::Right};
    std::vector<Event> events;
    bool loaded{true};
    bool hook_present{true};
    bool effect_ready{true};
    bool effect_available{true};
    bool linsha{};
    bool fail_texture{};
    bool texture_used{};
    std::string cleaned_path;

    std::uint32_t ClumpOffset() const {
        return hand == pal4::BaseObjectHand::Right ? 0x22cu : 0x39cu;
    }
    std::uint32_t EffectOffset() const {
        return hand == pal4::BaseObjectHand::Right ? 0x4f8u : 0x508u;
    }
    void CleanupNamed(pal4::BaseObjectState&,
                       pal4::BaseObjectHand selected) override {
        Check(selected == hand, "setup cleans the matching named weapon");
        events.emplace_back('C', 0u);
    }
    void AddAndProcessScripts() override { events.emplace_back('S', 0u); }
    void CleanupScripts() override { events.emplace_back('s', 0u); }
    std::string CreatePath(std::uint32_t directory) override {
        Check(directory == 0x77u, "path builder receives directory address");
        events.emplace_back('P', directory);
        return "root\\dir\\";
    }
    void CleanupPath(std::string& path) override {
        events.emplace_back('p', 0u);
        cleaned_path = path;
    }
    std::uint32_t GameInstance() override {
        events.emplace_back('G', 0u);
        return 0x7000u;
    }
    bool LoadClump(pal4::BaseObjectState& state,
                   std::uint32_t resource_offset, std::string_view label,
                   std::string_view path, std::string_view model,
                   std::uint32_t game, std::uint32_t zero0,
                   std::uint32_t zero1, std::uint32_t one) override {
        Check(resource_offset == (hand == pal4::BaseObjectHand::Right ?
                                      0x218u : 0x388u) &&
                  label == (hand == pal4::BaseObjectHand::Right ?
                                "PALRole-RHWeapon" : "PALRole-LHWeapon") &&
                  path == "root\\dir\\blade\\" && model == "blade" &&
                  game == 0x71acu && zero0 == 0u && zero1 == 0u &&
                  one == 1u,
              "clump loader receives exact hand resource and flag arguments");
        events.emplace_back('L', resource_offset);
        if (loaded) state.Write(ClumpOffset(), 0x2000u);
        return loaded;
    }
    std::uint32_t TextureDictionary() override {
        events.emplace_back('D', 0u);
        return 0x8000u;
    }
    std::uint32_t ResolveTexturePath(
        std::uint32_t dictionary, std::string_view path) override {
        Check(dictionary == 0x8000u &&
                  path == "root\\dir\\blade\\skin.dds",
              "texture dictionary receives the appended full path");
        events.emplace_back('Q', dictionary);
        return 0x9000u;
    }
    void LoadTexture(pal4::BaseObjectState&, std::uint32_t offset,
                     std::string_view texture,
                     std::uint32_t resolved) override {
        Check(offset == 0x388u && texture == "skin.dds" &&
                  resolved == 0x9000u,
              "both hands load optional texture through object +0x388");
        events.emplace_back('T', offset);
        texture_used = true;
        if (fail_texture) throw std::runtime_error("texture load failed");
    }
    std::uint32_t GetUserData(std::uint32_t clump,
                              std::string_view hook) override {
        Check(clump == 0x2000u && hook == "hook_hand",
              "named clump queries its hand hook");
        events.emplace_back('H', clump);
        return hook_present ? 0x3000u : 0u;
    }
    void HideHookAtomics(std::uint32_t clump) override {
        Check(clump == 0x2000u, "hook atomics hidden on loaded clump");
        events.emplace_back('h', clump);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back('r', address);
        if (address == 0x2004u) return 0x5000u;
        if (address == 0x3004u) return 0x6000u;
        Check(false, "named weapon reads only clump and hook frame pointers");
        return 0u;
    }
    void InvertTransform(std::uint32_t destination,
                        std::uint32_t source) override {
        Check(destination == 0x5010u && source == 0x6010u,
              "named weapon inverts hook frame transform");
        events.emplace_back('c', destination);
    }
    void UpdateFrame(std::uint32_t frame) override {
        Check(frame == 0x5000u, "frame update uses loaded weapon frame");
        events.emplace_back('f', frame);
    }
    std::int32_t CompareRoleToLinsha(std::uint32_t name) override {
        Check(name == 0x1234u, "left-hand rotation compares role name");
        events.emplace_back('n', name);
        return linsha ? 0 : 1;
    }
    void RotateMatrix(std::uint32_t matrix, std::uint32_t axis,
                      float angle, std::uint32_t mode) override {
        Check(matrix == 0x5010u && mode == 2u &&
                  ((axis == 0x8433a0u &&
                    angle == (linsha && hand == pal4::BaseObjectHand::Left ?
                                  -90.0f : 90.0f)) ||
                   (hand == pal4::BaseObjectHand::Right &&
                    axis == 0x8433acu && angle == 180.0f)),
              "named weapon applies IDA-backed role and hand rotation");
        events.emplace_back('R', axis);
    }
    void ProcessUserData(std::uint32_t body,
                         std::string_view hook,
                         std::uint32_t frame) override {
        Check(body == 0x1000u && frame == 0x5000u &&
                  hook == (hand == pal4::BaseObjectHand::Right ?
                               "hook_RHWeapon" : "hook_LHWeapon"),
              "named weapon attaches its frame at selected body hook");
        events.emplace_back('u', body);
    }
    bool EffectReady(pal4::BaseObjectState&,
                      std::uint32_t offset) override {
        Check(offset == EffectOffset() || offset == EffectOffset() + 8u,
              "named effect readiness checks both hand-specific handles");
        events.emplace_back('e', offset);
        return effect_ready;
    }
    void DisplayAndCleanupEffect(pal4::BaseObjectState&,
                                 std::uint32_t offset) override {
        events.emplace_back('d', offset);
    }
    bool EffectAvailable(std::uint32_t name) override {
        Check(name == 0x11u || name == 0x22u,
              "effect predicate sees the selected XML effect name");
        events.emplace_back('v', name);
        return effect_available;
    }
    std::uint32_t EffectSingleton() override {
        events.emplace_back('i', 0u);
        return 0xa000u;
    }
    pal4::BaseObjectEffectHandle LoadXmlEffect(
        std::uint32_t singleton, std::uint32_t name,
        std::uint32_t clump) override {
        Check(singleton == 0xa000u && clump == 0x2000u,
              "XML effect receives singleton and loaded clump");
        events.emplace_back('x', name);
        return {name + 0x100u, name + 0x200u};
    }
    std::uint32_t ArenaManager() override {
        events.emplace_back('A', 0u);
        return 0xb000u;
    }
    void AddClump(std::uint32_t arena, std::uint32_t clump) override {
        Check(arena == 0xb000u && clump == 0x2000u,
              "arena receives the newly loaded clump");
        events.emplace_back('a', clump);
    }
    void RemoveClump(std::uint32_t arena, std::uint32_t clump) override {
        Check(arena == 0xb000u && clump == 0x2000u,
              "cleanup removes the named clump from arena");
        events.emplace_back('m', clump);
    }
    void RemoveFrameChild(std::uint32_t body,
                          std::string_view hook,
                          std::uint32_t frame) override {
        Check(body == 0x1000u && frame == 0x5000u &&
                  hook == (hand == pal4::BaseObjectHand::Right ?
                               "hook_RHWeapon" : "hook_LHWeapon"),
              "cleanup detaches frame from the selected body hook");
        events.emplace_back('F', frame);
    }
    void CleanupClump(pal4::BaseObjectState&,
                      std::uint32_t offset,
                      std::uint32_t game) override {
        Check(offset == (hand == pal4::BaseObjectHand::Right ?
                             0x218u : 0x388u) && game == 0x71acu,
              "cleanup targets hand resource with game resource collection");
        events.emplace_back('K', offset);
    }
};

std::uint32_t Setup(pal4::BaseObjectState& state, Runtime& runtime,
                    std::string_view texture, std::uint32_t effect0,
                    std::uint32_t effect1) {
    return runtime.hand == pal4::BaseObjectHand::Right ?
        pal4::BaseObjectNamedWeapon::SetupRight(
            state, "blade", texture, 0x77u, effect0, effect1, runtime) :
        pal4::BaseObjectNamedWeapon::SetupLeft(
            state, "blade", texture, 0x77u, effect0, effect1, runtime);
}

std::uint32_t Cleanup(pal4::BaseObjectState& state, Runtime& runtime) {
    return runtime.hand == pal4::BaseObjectHand::Right ?
        pal4::BaseObjectNamedWeapon::CleanupRight(state, runtime) :
        pal4::BaseObjectNamedWeapon::CleanupLeft(state, runtime);
}

void CheckSetup(pal4::BaseObjectHand hand, bool linsha,
                bool texture_used, bool effects) {
    pal4::BaseObjectState state{};
    state.Write(0x14u, 0x1000u);
    state.Write(4u, 0x1234u);
    Runtime runtime;
    runtime.hand = hand;
    runtime.linsha = linsha;
    const auto texture = texture_used ? "skin.dds" : "";
    const auto result = Setup(state, runtime, texture,
                              effects ? 0x11u : 0u,
                              effects ? 0x22u : 0u);
    Check(result == 1u && runtime.texture_used == texture_used &&
              state.Read(runtime.ClumpOffset()) == 0x2000u &&
              runtime.events.front() == Event{'C', 0u} &&
              runtime.events.back() == Event{'p', 0u},
          "both named weapon setups complete and clean temporary path");
    Check(runtime.cleaned_path ==
              (texture_used ? "root\\dir\\blade\\skin.dds" :
                              "root\\dir\\blade\\"),
          "temporary path retains optional texture component until cleanup");
    Check(std::count(runtime.events.begin(), runtime.events.end(),
                     Event{'s', 0u}) == 1 &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         Event{'f', 0x5000u}) == 2 &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         Event{'R', 0x8433acu}) ==
                  (hand == pal4::BaseObjectHand::Right ? 1 : 0),
          "scripts, frame updates and right-only second rotation match IDA");
    for (std::uint32_t index = 0u; index < 2u; ++index) {
        const auto offset = runtime.EffectOffset() + index * 8u;
        const auto name = index == 0u ? 0x11u : 0x22u;
        Check(state.Read(offset) == (effects ? name + 0x100u : 0u) &&
                  state.Read(offset + 4u) ==
                      (effects ? name + 0x200u : 0u),
              "optional XML effects write only their selected handle pairs");
    }
}

void CheckSetupFailures(pal4::BaseObjectHand hand) {
    pal4::BaseObjectState state{};
    Runtime runtime;
    runtime.hand = hand;
    Check(Setup(state, runtime, "", 0u, 0u) == 0u &&
              runtime.events.empty(),
          "missing owner clump bypasses scripts and path allocation");

    state.Write(0x14u, 0x1000u);
    runtime.loaded = false;
    Check(Setup(state, runtime, "", 0u, 0u) == 0u &&
              runtime.events == std::vector<Event>{
                  {'C', 0u}, {'S', 0u}, {'P', 0x77u}, {'G', 0u},
                  {'L', hand == pal4::BaseObjectHand::Right ?
                            0x218u : 0x388u}, {'s', 0u}, {'p', 0u}},
          "failed clump load cleans scripts and path before returning zero");

    runtime.events.clear();
    runtime.loaded = true;
    runtime.hook_present = false;
    Check(Setup(state, runtime, "", 0u, 0u) == 0u &&
              runtime.events.back() == Event{'p', 0u} &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         Event{'s', 0u}) == 1,
          "missing hand hook returns after script and path cleanup");
}

void CheckTextureFailure() {
    pal4::BaseObjectState state{};
    state.Write(0x14u, 0x1000u);
    Runtime runtime;
    runtime.fail_texture = true;
    bool threw = false;
    try {
        (void)Setup(state, runtime, "skin.dds", 0u, 0u);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events.back() == Event{'p', 0u} &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         Event{'s', 0u}) == 0,
          "exception unwinds temporary path without inventing script cleanup");
}

void CheckCleanup(pal4::BaseObjectHand hand, bool ready) {
    pal4::BaseObjectState state{};
    Runtime runtime;
    runtime.hand = hand;
    runtime.effect_ready = ready;
    state.Write(runtime.ClumpOffset(), 0x2000u);
    Check(Cleanup(state, runtime) == 0u && runtime.events.empty(),
          "missing owner clump prevents named cleanup");
    state.Write(0x14u, 0x1000u);
    Check(Cleanup(state, runtime) == 1u &&
              state.Read(runtime.ClumpOffset()) == 0x2000u,
          "named cleanup delegates clump ownership to resource manager");
    std::vector<Event> expected{
        {'A', 0u}, {'m', 0x2000u}, {'r', 0x2004u}, {'F', 0x5000u},
        {'e', runtime.EffectOffset()}};
    if (ready) expected.emplace_back('d', runtime.EffectOffset());
    expected.emplace_back('e', runtime.EffectOffset() + 8u);
    if (ready) expected.emplace_back('d', runtime.EffectOffset() + 8u);
    expected.insert(expected.end(), {{'G', 0u},
                                     {'K', hand == pal4::BaseObjectHand::Right ?
                                               0x218u : 0x388u}});
    Check(runtime.events == expected,
          "named cleanup follows arena, frame, effects and resource order");
}

}  // namespace

int main() {
    for (auto hand : {pal4::BaseObjectHand::Right,
                      pal4::BaseObjectHand::Left}) {
        for (bool linsha : {false, true}) {
            for (bool texture : {false, true}) {
                for (bool effects : {false, true}) {
                    CheckSetup(hand, linsha, texture, effects);
                }
            }
        }
        CheckSetupFailures(hand);
        CheckCleanup(hand, false);
        CheckCleanup(hand, true);
    }
    CheckTextureFailure();
}
