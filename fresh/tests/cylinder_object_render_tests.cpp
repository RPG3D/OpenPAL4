#include "pal4/cylinder_object_render.h"

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

struct Runtime final : pal4::ICylinderObjectRenderRuntime {
    std::vector<std::string> calls;
    std::uint8_t owner_flag = 0u;
    std::uint32_t manager = 0x5000u;
    std::uint8_t valid = 1u;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x1053u, "owner flag at +83");
        calls.push_back("read_owner_flag");
        return owner_flag;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x2008u, "model object at +8");
        calls.push_back("read_model_object");
        return 0x3000u;
    }
    std::uint32_t GetObjectTransform(std::uint32_t object) override {
        Check(object == 0x3000u, "object transform target");
        calls.push_back("get_transform");
        return 0x4000u;
    }
    std::uint32_t EffectManager() override {
        calls.push_back("manager");
        return manager;
    }
    std::uint8_t ValidateTransform(std::uint32_t value,
                                   std::uint32_t context,
                                   std::uint32_t transform) override {
        Check(value == 0x5000u && context == 0x7000u &&
                  transform == 0x4030u,
              "manager validates frame transform +48");
        calls.push_back("validate");
        return valid;
    }
    std::uint8_t RenderModel(std::uint32_t model,
                             std::uint32_t ebp_value,
                             std::uint32_t context,
                             std::uint32_t repeated_context) override {
        Check(model == 0x2000u && ebp_value == 0x6000u &&
                  context == 0x7000u && repeated_context == context,
              "render receives model, EBP and repeated context");
        calls.push_back("render");
        return 0x9au;
    }
};

pal4::CylinderObjectState ActiveState() {
    pal4::CylinderObjectState state{};
    state.words[1u] = 0x01000001u;
    state.words[3u] = 0x1000u;
    state.words[22u] = 0x2000u;
    return state;
}

void TestRender() {
    auto state = ActiveState();
    Runtime runtime;
    using pal4::CylinderObjectRenderer;
    state.words[1u] = 0x01000000u;
    Check(CylinderObjectRenderer::Render(
              state, 0x6000u, 0x7000u, runtime) == 0u &&
              runtime.calls.empty(),
          "inactive render exits before runtime calls");
    state.words[1u] = 1u;
    Check(CylinderObjectRenderer::Render(
              state, 0x6000u, 0x7000u, runtime) == 0u &&
              runtime.calls.empty(),
          "disabled render flag exits before runtime calls");
    state = ActiveState();
    state.words[3u] = 0u;
    Check(CylinderObjectRenderer::Render(
              state, 0x6000u, 0x7000u, runtime) == 0x9au &&
              runtime.calls == std::vector<std::string>{"render"},
          "absent owner renders directly");
    runtime.calls.clear();
    state.words[3u] = 0x1000u;
    Check(CylinderObjectRenderer::Render(
              state, 0x6000u, 0x7000u, runtime) == 0x9au &&
              runtime.calls == std::vector<std::string>{
                  "read_owner_flag", "render"},
          "owner without transform flag renders directly");
    runtime.calls.clear();
    runtime.owner_flag = 1u;
    Check(CylinderObjectRenderer::Render(
              state, 0x6000u, 0x7000u, runtime) == 0x9au &&
              runtime.calls == std::vector<std::string>{
                  "read_owner_flag", "read_model_object", "get_transform",
                  "manager", "validate", "render"},
          "valid transform renders after native validation");
    runtime.calls.clear();
    runtime.valid = 0u;
    Check(CylinderObjectRenderer::Render(
              state, 0x6000u, 0x7000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read_owner_flag", "read_model_object", "get_transform",
                  "manager", "validate"},
          "invalid transform suppresses render");
}

void TestMissingManager() {
    auto state = ActiveState();
    Runtime runtime;
    runtime.owner_flag = 1u;
    runtime.manager = 0u;
    try {
        static_cast<void>(pal4::CylinderObjectRenderer::Render(
            state, 0x6000u, 0x7000u, runtime));
        Check(false, "missing manager must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "\xb5\xa5\xbc\xfe\xb6\xd4\xcf\xf3\xce\xb4\xb1\xbb"
                  "\xb4\xb4\xbd\xa8\xa3\xac\xc7\xeb\xd4\xda\xca\xb9"
                  "\xd3\xc3\xd6\xae\xc7\xb0\xb4\xb4\xbd\xa8\xa3\xa1" &&
                  runtime.calls == std::vector<std::string>{
                      "read_owner_flag", "read_model_object",
                      "get_transform", "manager"},
              "exception retains IDA GBK bytes and prethrow call order");
    }
}

}  // namespace

int main() {
    TestRender();
    TestMissingManager();
}
