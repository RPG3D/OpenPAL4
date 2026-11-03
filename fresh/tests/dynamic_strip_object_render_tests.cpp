#include "pal4/dynamic_strip_object_render.h"

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

struct Runtime final : pal4::IDynamicStripObjectRenderRuntime {
    std::vector<std::string> calls;
    std::uint8_t owner_flag = 0u;
    std::uint32_t manager = 0x5000u;
    std::uint8_t valid = 1u;
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x1053u, "owner flag at +83");
        calls.push_back("read_owner_flag");
        return owner_flag;
    }
    std::uint32_t GetObjectTransform(std::uint32_t object) override {
        Check(object == 0x3000u, "resource object at +108");
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
    void RenderVisual(const pal4::DynamicStripObjectState& state) override {
        Check(state.words[27u] == 0x3000u,
              "strip renderer receives object resource block");
        calls.push_back("render");
    }
};

pal4::DynamicStripObjectState ActiveState() {
    pal4::DynamicStripObjectState state{};
    state.words[1u] = 0x01000001u;
    state.words[3u] = 0x1000u;
    state.words[27u] = 0x3000u;
    return state;
}

void TestRender() {
    auto state = ActiveState();
    Runtime runtime;
    using pal4::DynamicStripObjectRenderer;
    state.words[1u] = 0x01000000u;
    DynamicStripObjectRenderer::Render(state, 0x7000u, runtime);
    Check(runtime.calls.empty(), "inactive object skips render");
    state.words[1u] = 1u;
    DynamicStripObjectRenderer::Render(state, 0x7000u, runtime);
    Check(runtime.calls.empty(), "render flag gate skips render");
    state = ActiveState();
    state.words[3u] = 0u;
    DynamicStripObjectRenderer::Render(state, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{"render"},
          "absent owner renders directly");
    runtime.calls.clear();
    state.words[3u] = 0x1000u;
    DynamicStripObjectRenderer::Render(state, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "read_owner_flag", "render"},
          "owner without transform flag renders directly");
    runtime.calls.clear();
    runtime.owner_flag = 1u;
    DynamicStripObjectRenderer::Render(state, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "read_owner_flag", "get_transform", "manager", "validate",
              "render"},
          "valid transform renders after native validation");
    runtime.calls.clear();
    runtime.valid = 0u;
    DynamicStripObjectRenderer::Render(state, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "read_owner_flag", "get_transform", "manager", "validate"},
          "invalid transform suppresses visual renderer");
}

void TestMissingManager() {
    auto state = ActiveState();
    Runtime runtime;
    runtime.owner_flag = 1u;
    runtime.manager = 0u;
    try {
        pal4::DynamicStripObjectRenderer::Render(state, 0x7000u, runtime);
        Check(false, "missing manager must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "\xb5\xa5\xbc\xfe\xb6\xd4\xcf\xf3\xce\xb4\xb1\xbb"
                  "\xb4\xb4\xbd\xa8\xa3\xac\xc7\xeb\xd4\xda\xca\xb9"
                  "\xd3\xc3\xd6\xae\xc7\xb0\xb4\xb4\xbd\xa8\xa3\xa1" &&
                  runtime.calls == std::vector<std::string>{
                      "read_owner_flag", "get_transform", "manager"},
              "exception keeps original GBK bytes and prethrow call order");
    }
}

}  // namespace

int main() {
    TestRender();
    TestMissingManager();
}
