#include "pal4/sequence_anim_render.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
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

struct Runtime final : pal4::ISequenceAnimRenderRuntime {
    std::vector<std::string> calls;
    std::uint8_t visible = 1u;
    std::uint32_t manager = 0x8000u;
    std::uint8_t valid = 1u;
    std::uint8_t rendered = 0x7bu;
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x3053u, "instance visibility byte");
        calls.emplace_back("visibility");
        return visible;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x3014u, "resource object handle");
        calls.emplace_back("object");
        return 0x7000u;
    }
    std::uint32_t GetObjectTransform(std::uint32_t object) override {
        Check(object == 0x7000u, "transform lookup object");
        calls.emplace_back("transform");
        return 0x9000u;
    }
    std::uint32_t EffectManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    std::uint8_t ValidateTransform(std::uint32_t input_manager,
                                   std::uint32_t context,
                                   std::uint32_t transform) override {
        Check(input_manager == 0x8000u && context == 0x6000u &&
                  transform == 0x9030u,
              "transform validation arguments");
        calls.emplace_back("validate");
        return valid;
    }
    std::uint8_t RenderResource(std::uint32_t resource,
                                std::uint32_t caller_ebp,
                                std::uint32_t caller_edi) override {
        Check(resource == 0x3000u && caller_ebp == 0x1111u &&
                  caller_edi == 0x2222u,
              "resource render and caller registers");
        calls.emplace_back("render");
        return rendered;
    }
};

pal4::SequenceAnimObjectState ReadyState() {
    pal4::SequenceAnimObjectState state{};
    state.words[1u] = 0x01000001u;
    state.words[3u] = 0x3000u;
    state.words[30u] = 0x3000u;
    state.words[13u] = std::bit_cast<std::uint32_t>(0.1f);
    return state;
}

void TestGates() {
    auto state = ReadyState();
    Runtime runtime;
    state.words[1u] = 0x01000000u;
    Check(pal4::SequenceAnimRenderer::Render(
              state, 0x6000u, 0x1111u, 0x2222u, runtime) == 0u,
          "inactive owner result");
    state.words[1u] = 1u;
    Check(pal4::SequenceAnimRenderer::Render(
              state, 0x6000u, 0x1111u, 0x2222u, runtime) == 0u,
          "render byte gate");
    state = ReadyState();
    for (const auto interval : {0.0f, -1.0f,
                                std::numeric_limits<float>::quiet_NaN()}) {
        state.words[13u] = std::bit_cast<std::uint32_t>(interval);
        Check(pal4::SequenceAnimRenderer::Render(
                  state, 0x6000u, 0x1111u, 0x2222u, runtime) == 0u,
              "strictly positive interval gate");
    }
    Check(runtime.calls.empty(), "gates avoid native calls");
}

void TestValidation() {
    auto state = ReadyState();
    Runtime runtime;
    Check(pal4::SequenceAnimRenderer::Render(
              state, 0x6000u, 0x1111u, 0x2222u, runtime) == 0x7bu &&
              runtime.calls == std::vector<std::string>{
                  "visibility", "object", "transform", "manager",
                  "validate", "render"},
          "visible instance validates before resource render");
    runtime.calls.clear();
    runtime.valid = 0u;
    Check(pal4::SequenceAnimRenderer::Render(
              state, 0x6000u, 0x1111u, 0x2222u, runtime) == 0u &&
              runtime.calls.back() == "validate" &&
              runtime.calls.size() == 5u,
          "failed transform validation skips render");

    runtime.calls.clear();
    runtime.valid = 1u;
    runtime.visible = 0u;
    Check(pal4::SequenceAnimRenderer::Render(
              state, 0x6000u, 0x1111u, 0x2222u, runtime) == 0x7bu &&
              runtime.calls == std::vector<std::string>{
                  "visibility", "render"},
          "hidden instance skips validation");
    runtime.calls.clear();
    state.words[3u] = 0u;
    Check(pal4::SequenceAnimRenderer::Render(
              state, 0x6000u, 0x1111u, 0x2222u, runtime) == 0x7bu &&
              runtime.calls == std::vector<std::string>{"render"},
          "null instance forwards resource render");

    state = ReadyState();
    runtime.calls.clear();
    runtime.visible = 1u;
    runtime.manager = 0u;
    bool threw = false;
    try {
        (void)pal4::SequenceAnimRenderer::Render(
            state, 0x6000u, 0x1111u, 0x2222u, runtime);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) ==
            "\xb5\xa5\xbc\xfe\xb6\xd4\xcf\xf3\xce\xb4\xb1\xbb"
            "\xb4\xb4\xbd\xa8\xa3\xac\xc7\xeb\xd4\xda\xca\xb9"
            "\xd3\xc3\xd6\xae\xc7\xb0\xb4\xb4\xbd\xa8\xa3\xa1";
    }
    Check(threw && runtime.calls.back() == "manager",
          "missing manager preserves GBK exception bytes");
}

}  // namespace

int main() {
    TestGates();
    TestValidation();
}
