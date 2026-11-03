#include "pal4/strip_renderer.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IStripRenderRuntime {
    std::vector<std::string> events;
    std::uint8_t instance_visible = 1u;
    std::uint8_t valid = 1u;
    std::uint32_t manager = 0x5000u;
    bool straight = false;
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x4053u, "instance visibility byte");
        events.emplace_back("visibility");
        return instance_visible;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == (straight ? 0x3010u : 0x3104u),
              "branch-specific transform source");
        events.emplace_back("object");
        return 0x6000u;
    }
    std::uint32_t GetObjectTransform(std::uint32_t object) override {
        Check(object == 0x6000u, "native transform object");
        events.emplace_back("transform");
        return 0x7000u;
    }
    std::uint32_t EffectManager() override {
        events.emplace_back("manager");
        return manager;
    }
    std::uint8_t ValidateTransform(std::uint32_t value,
                                   std::uint32_t context,
                                   std::uint32_t translation) override {
        Check(value == 0x5000u && context == 0x1000u &&
                  translation == 0x7030u,
              "validation uses translation at transform +48");
        events.emplace_back("validate");
        return valid;
    }
    std::uint8_t RenderStraight(std::uint32_t model,
                                std::uint32_t caller_ebp,
                                std::uint32_t caller_edi) override {
        Check(model == 0x3000u && caller_ebp == 0x2000u &&
                  caller_edi == 0x2100u,
              "straight renderer forwards caller registers");
        events.emplace_back("straight");
        return 0x5au;
    }
    std::uint8_t RenderClump(std::uint32_t clump) override {
        Check(clump == 0x3100u, "cylinder clump handle");
        events.emplace_back("clump");
        return 0xa5u;
    }
};

pal4::StripObjectState State(bool straight) {
    pal4::StripObjectState state{};
    state.words[1u] = 0x01000001u;
    state.words[3u] = 0x4000u;
    state.words[34u] = 0x3100u;
    state.words[35u] = straight ? 1u : 0u;
    state.words[38u] = 0x3000u;
    return state;
}

void TestGatesAndBranches() {
    auto state = State(false);
    Runtime runtime;
    state.words[1u] = 0u;
    Check(pal4::StripRenderer::Render(
              state, 0x1000u, 0x2000u, 0x2100u, runtime) == 0u &&
              runtime.events.empty(), "inactive render gate");
    state.words[1u] = 1u;
    Check(pal4::StripRenderer::Render(
              state, 0x1000u, 0x2000u, 0x2100u, runtime) == 0u &&
              runtime.events.empty(), "hidden render gate");

    state = State(false);
    Check(pal4::StripRenderer::Render(
              state, 0x1000u, 0x2000u, 0x2100u, runtime) == 0xa5u &&
              runtime.events == std::vector<std::string>{
                  "visibility", "object", "transform", "manager",
                  "validate", "clump"},
          "cylinder transform validates before clump render");

    state = State(true);
    runtime.events.clear();
    runtime.straight = true;
    Check(pal4::StripRenderer::Render(
              state, 0x1000u, 0x2000u, 0x2100u, runtime) == 0x5au &&
              runtime.events.back() == "straight",
          "straight renderer uses model transform and caller registers");
    runtime.events.clear();
    runtime.instance_visible = 0u;
    Check(pal4::StripRenderer::Render(
              state, 0x1000u, 0x2000u, 0x2100u, runtime) == 0x5au &&
              runtime.events == std::vector<std::string>{
                  "visibility", "straight"},
          "instance without validation flag renders directly");
}

void TestValidationFailures() {
    auto state = State(false);
    Runtime runtime;
    runtime.valid = 0u;
    Check(pal4::StripRenderer::Render(
              state, 0x1000u, 0x2000u, 0x2100u, runtime) == 0u &&
              runtime.events.back() == "validate",
          "invalid transform prevents rendering");
    runtime.manager = 0u;
    runtime.events.clear();
    try {
        (void)pal4::StripRenderer::Render(
            state, 0x1000u, 0x2000u, 0x2100u, runtime);
        Check(false, "missing manager must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "\xb5\xa5\xbc\xfe\xb6\xd4\xcf\xf3\xce\xb4\xb1\xbb"
                  "\xb4\xb4\xbd\xa8\xa3\xac\xc7\xeb\xd4\xda\xca\xb9"
                  "\xd3\xc3\xd6\xae\xc7\xb0\xb4\xb4\xbd\xa8\xa3\xa1" &&
                  runtime.events.back() == "manager",
              "missing manager preserves native GBK exception");
    }
}

}  // namespace

int main() {
    TestGatesAndBranches();
    TestValidationFailures();
}
