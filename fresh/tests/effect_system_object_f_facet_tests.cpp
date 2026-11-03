#include "pal4/effect_system_object_f_facet.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemObjectFFacetRuntime {
    std::vector<std::string> events;
    bool activate = false;
    std::uint32_t world = 0x9000u;
    std::uint32_t removed = 0x123u;
    std::uint32_t component = 0x6000u;
    std::uint32_t clump = 0x7000u;
    std::uint32_t read_count = 0u;

    void InitializeFacet(pal4::EffectSystemObjectFState& state,
                         std::uint32_t offset) override {
        events.push_back("init:" + std::to_string(offset));
        if (activate)
            state.words[1u] |= 1u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        ++read_count;
        if (address == 0x5030u)
            return component;
        Check(address == component + 8u, "clump chain address");
        return clump;
    }
    std::uint32_t FindWorld(std::uint32_t input) override {
        events.push_back("world:" + std::to_string(input));
        Check(input == clump, "first clump lookup");
        return world;
    }
    std::uint32_t RemoveClump(std::uint32_t input_world,
                              std::uint32_t input_clump) override {
        events.push_back("remove:" + std::to_string(input_world));
        Check(input_world == world && input_clump == clump,
              "RenderWare removal operands");
        return removed;
    }
};

void TestStartFlags() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[1u] = 0xaabbcc00u;
    Check(pal4::EffectSystemObjectFFacet::Start(state, runtime) == 0u &&
              state.words[1u] == 0xaabbcc00u &&
              runtime.events == std::vector<std::string>{"init:20"},
          "inactive facet still invokes base initialization");
    runtime.activate = true;
    runtime.events.clear();
    Check(pal4::EffectSystemObjectFFacet::Start(state, runtime) == 1u &&
              state.words[1u] == 0x0101cc01u &&
              runtime.events == std::vector<std::string>{"init:20"},
          "active facet sets bytes +6/+7 after base initialization");
}

void TestStopAndWorldRemoval() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[29u] = 0x5000u;
    Check(pal4::EffectSystemObjectFFacet::Stop(state, runtime) == 0u &&
              runtime.events.empty(), "inactive facet does not touch clump");
    state.words[1u] = 0x0101cc01u;
    Check(pal4::EffectSystemObjectFFacet::Stop(state, runtime) == 0x23u &&
              state.words[1u] == 0x0000cc01u &&
              runtime.read_count == 4u &&
              runtime.events == std::vector<std::string>{
                  "read:20528", "read:24584", "world:28672",
                  "read:20528", "read:24584", "remove:36864"},
          "world removal re-reads native clump chain and returns low byte");
    runtime.events.clear();
    runtime.read_count = 0u;
    runtime.world = 0u;
    Check(pal4::EffectSystemObjectFFacet::Stop(state, runtime) == 0u &&
              runtime.read_count == 2u &&
              runtime.events.back() == "world:28672",
          "missing world skips second chain and removal");
}

}  // namespace

int main() {
    TestStartFlags();
    TestStopAndWorldRemoval();
}
