#include "pal4/clump_object_visibility.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IClumpObjectVisibilityRuntime {
    std::vector<std::string> calls;
    std::uint32_t effect_manager = 0x4000u;
    std::uint32_t world = 0x5000u;
    bool owner_allows = true;
    pal4::ClumpObjectState* state = nullptr;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1014u, "facet uses manager clump at +20");
        calls.push_back("clump");
        return 0x2000u;
    }
    std::uint32_t EffectManager() override {
        calls.push_back("effect_manager");
        return effect_manager;
    }
    void RemoveClumpAndProcessAtomics(std::uint32_t manager,
                                    std::uint32_t clump) override {
        Check(manager == 0x4000u && clump == 0x2000u,
              "deactivate effect-system arguments");
        calls.push_back("remove_effect");
    }
    void PerformOwnerAction2(std::uint32_t owner,
                             std::uint32_t clump) override {
        Check(owner == 0x3000u && clump == 0x2000u,
              "deactivate owner arguments");
        calls.push_back("owner_stop");
    }
    std::uint32_t WorldForClump(std::uint32_t clump) override {
        Check(clump == 0x2000u, "world lookup clump");
        calls.push_back("world");
        return world;
    }
    std::uint8_t RemoveClump(std::uint32_t value,
                             std::uint32_t clump) override {
        Check(value == 0x5000u && clump == 0x2000u,
              "remove clump arguments");
        calls.push_back("remove_clump");
        return 0x5au;
    }
    bool OwnerAllowsClump(std::uint32_t owner,
                           std::uint32_t clump) override {
        Check(owner == 0x3000u && clump == 0x2000u,
              "activate owner query arguments");
        calls.push_back("allow");
        return owner_allows;
    }
    void PerformOwnerAction(std::uint32_t owner,
                            std::uint32_t clump) override {
        Check(owner == 0x3000u && clump == 0x2000u,
              "activate owner action arguments");
        Check(state != nullptr && (state->words[1u] & 0x00ff0000u) != 0u,
              "owner action runs after active update byte set");
        calls.push_back("owner_start");
    }
    std::uint8_t HandleEffectAddition(std::uint32_t manager,
                                      std::uint32_t clump) override {
        Check(manager == 0x4000u && clump == 0x2000u,
              "activate effect-system arguments");
        calls.push_back("add_effect");
        return 0xa5u;
    }
};

pal4::ClumpObjectState ActiveState() {
    pal4::ClumpObjectState state{};
    state.words[1u] = 0x01010101u;
    state.words[4u] = 0x3000u;
    state.words[18u] = 0x1000u;
    return state;
}

void TestInactiveAndDeactivate() {
    pal4::ClumpObjectState state{};
    Runtime runtime;
    Check(pal4::ClumpObjectVisibilityFacet::Deactivate(state, runtime) == 0u &&
              pal4::ClumpObjectVisibilityFacet::Activate(state, runtime) == 0u &&
              runtime.calls.empty(),
          "inactive facet avoids effect and owner callbacks");

    state = ActiveState();
    Check(pal4::ClumpObjectVisibilityFacet::Deactivate(state, runtime) ==
              0x5au && (state.words[1u] & 0x00ff0000u) == 0u &&
              (state.words[1u] & 0xff000000u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "effect_manager", "clump", "remove_effect", "clump",
                  "owner_stop", "clump", "world", "remove_clump"},
          "deactivate clears bytes before native removal sequence");

    runtime.calls.clear();
    state = ActiveState();
    runtime.world = 0u;
    Check(pal4::ClumpObjectVisibilityFacet::Deactivate(state, runtime) == 0u &&
              runtime.calls.back() == "world",
          "missing world skips native clump removal");
}

void TestActivate() {
    auto state = ActiveState();
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::ClumpObjectVisibilityFacet::Activate(state, runtime) ==
              0xa5u &&
              (state.words[1u] & 0x00ff0000u) == 0x00010000u &&
              (state.words[1u] & 0xff000000u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "effect_manager", "clump", "remove_effect", "clump",
                  "owner_stop", "clump", "world", "remove_clump",
                  "clump", "allow", "world", "owner_start",
                  "effect_manager", "clump", "add_effect"},
          "activate calls deactivation first and suppresses render for owned world");

    runtime.calls.clear();
    runtime.owner_allows = false;
    Check(pal4::ClumpObjectVisibilityFacet::Activate(state, runtime) ==
              0xa5u && (state.words[1u] & 0xff000000u) == 0x01000000u &&
              runtime.calls == std::vector<std::string>{
                  "effect_manager", "clump", "remove_effect", "clump",
                  "owner_stop", "clump", "world", "remove_clump",
                  "clump", "allow", "owner_start", "effect_manager",
                  "clump", "add_effect"},
          "owner rejection leaves render flag set and skips second world lookup");
}

void TestMissingEffectManager() {
    auto state = ActiveState();
    Runtime runtime;
    runtime.effect_manager = 0u;
    bool threw = false;
    try {
        static_cast<void>(
            pal4::ClumpObjectVisibilityFacet::Deactivate(state, runtime));
    } catch (const std::runtime_error& error) {
        threw = std::string_view(error.what()) ==
            "\xB5\xA5\xBC\xFE\xB6\xD4\xCF\xF3\xCE\xB4\xB1\xBB"
            "\xB4\xB4\xBD\xA8\xA3\xAC\xC7\xEB\xD4\xDA\xCA\xB9"
            "\xD3\xC3\xD6\xAE\xC7\xB0\xB4\xB4\xBD\xA8\xA3\xA1";
    }
    Check(threw && (state.words[1u] & 0xffff0000u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "effect_manager", "clump"},
          "missing manager throws after clearing flags and reading clump");
}

}  // namespace

int main() {
    TestInactiveAndDeactivate();
    TestActivate();
    TestMissingEffectManager();
}
