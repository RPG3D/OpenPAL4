#include "pal4/effect_object_render.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectObjectRenderRuntime {
    std::vector<std::string> calls;
    std::uint8_t visible = 1u;
    std::uint32_t manager = 0x8000u;
    std::uint8_t valid = 1u;

    std::uint8_t ReadByte(const std::uint32_t address) override {
        Check(address == 0x3053u, "instance visibility byte");
        calls.emplace_back("visibility");
        return visible;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(address == 0x30acu, "instance object handle");
        calls.emplace_back("object");
        return 0x7000u;
    }
    std::uint32_t GetObjectTransform(const std::uint32_t object) override {
        Check(object == 0x7000u, "transform lookup object");
        calls.emplace_back("transform");
        return 0x9000u;
    }
    std::uint32_t EffectManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    std::uint8_t ValidateTransform(const std::uint32_t input_manager,
                                   const std::uint32_t context,
                                   const std::uint32_t transform) override {
        Check(input_manager == 0x8000u && context == 0x6000u &&
                  transform == 0x9030u,
              "transform validation arguments");
        calls.emplace_back("validate");
        return valid;
    }
    void NoOpDraw(const std::uint32_t context) override {
        Check(context == 0x6000u, "no-op context");
        calls.emplace_back("noop");
    }
};

pal4::EffectObjectState State() {
    pal4::EffectObjectState state{};
    state.words[1u] = 0x01000001u;
    state.words[23u] = 0x3000u;
    return state;
}

void TestGates() {
    auto state = State();
    Runtime runtime;
    state.words[1u] = 0x01000000u;
    pal4::EffectObjectRenderer::Render(state, 0x6000u, runtime);
    state.words[1u] = 1u;
    pal4::EffectObjectRenderer::Render(state, 0x6000u, runtime);
    Check(runtime.calls.empty(), "activity and render flags gate native calls");

    state = State();
    state.words[23u] = 0u;
    pal4::EffectObjectRenderer::Render(state, 0x6000u, runtime);
    Check(runtime.calls == std::vector<std::string>{"noop"},
          "null instance still reaches native no-op render");
    runtime.calls.clear();
    state = State();
    runtime.visible = 0u;
    pal4::EffectObjectRenderer::Render(state, 0x6000u, runtime);
    Check(runtime.calls == std::vector<std::string>{"visibility", "noop"},
          "hidden instance skips transform validation");
}

void TestValidation() {
    auto state = State();
    Runtime runtime;
    pal4::EffectObjectRenderer::Render(state, 0x6000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "visibility", "object", "transform", "manager", "validate",
              "noop"},
          "visible instance validates before the original no-op callback");
    runtime.calls.clear();
    runtime.valid = 0u;
    pal4::EffectObjectRenderer::Render(state, 0x6000u, runtime);
    Check(runtime.calls.back() == "validate" && runtime.calls.size() == 5u,
          "failed validation skips native render");

    runtime.calls.clear();
    runtime.manager = 0u;
    bool threw = false;
    try {
        pal4::EffectObjectRenderer::Render(state, 0x6000u, runtime);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) ==
            "\xb5\xa5\xbc\xfe\xb6\xd4\xcf\xf3\xce\xb4\xb1\xbb"
            "\xb4\xb4\xbd\xa8\xa3\xac\xc7\xeb\xd4\xda\xca\xb9"
            "\xd3\xc3\xd6\xae\xc7\xb0\xb4\xb4\xbd\xa8\xa3\xa1";
    }
    Check(threw && runtime.calls.back() == "manager",
          "missing manager preserves original GBK exception bytes");
}

}  // namespace

int main() {
    TestGates();
    TestValidation();
}
