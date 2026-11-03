#include "pal4/thunder_renderer.h"

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

struct Runtime final : pal4::IThunderRenderRuntime {
    std::vector<std::string> events;
    bool owner_visible = true;
    bool manager_available = true;
    bool valid_transform = true;

    bool OwnerVisible(const std::uint32_t owner,
                      const std::uint32_t offset) override {
        Check(owner == 0x9000u && offset == 83u,
              "owner visibility at byte +83");
        events.emplace_back("owner");
        return owner_visible;
    }
    std::uint32_t ObjectTransform(const std::uint32_t resource) override {
        Check(resource == 0x7000u, "visual resource at owner +112");
        events.emplace_back("transform");
        return 0x8000u;
    }
    bool EffectManagerAvailable() override {
        events.emplace_back("manager");
        return manager_available;
    }
    bool ValidateTransform(const std::uint32_t context,
                           const std::uint32_t transform) override {
        Check(context == 0x3333u && transform == 0x8030u,
              "manager checks transform +48 and caller context");
        events.emplace_back("validate");
        return valid_transform;
    }
    void RenderVisual(pal4::ThunderObjectState&) override {
        events.emplace_back("render");
    }
};

}  // namespace

int main() {
    pal4::ThunderObjectState state{};
    Runtime runtime;
    pal4::ThunderRenderer::Render(state, 0x3333u, runtime);
    Check(runtime.events.empty(), "inactive object skips render");
    state.words[1u] = 1u;
    pal4::ThunderRenderer::Render(state, 0x3333u, runtime);
    Check(runtime.events.empty(), "hidden object skips render");

    state.words[1u] = 0x01000001u;
    pal4::ThunderRenderer::Render(state, 0x3333u, runtime);
    Check(runtime.events == std::vector<std::string>{"render"},
          "no owner renders without validation");
    state.words[3u] = 0x9000u;
    runtime.owner_visible = false;
    runtime.events.clear();
    pal4::ThunderRenderer::Render(state, 0x3333u, runtime);
    Check(runtime.events == std::vector<std::string>{"owner", "render"},
          "invisible owner renders without transform check");

    runtime.owner_visible = true;
    runtime.manager_available = false;
    runtime.events.clear();
    state.words[28u] = 0x7000u;
    bool caught = false;
    try {
        pal4::ThunderRenderer::Render(state, 0x3333u, runtime);
    } catch (const std::runtime_error& error) {
        constexpr std::string_view original(
            "\xb5\xa5\xbc\xfe\xb6\xd4\xcf\xf3\xce\xb4\xb1\xbb"
            "\xb4\xb4\xbd\xa8\xa3\xac\xc7\xeb\xd4\xda\xca\xb9"
            "\xd3\xc3\xd6\xae\xc7\xb0\xb4\xb4\xbd\xa8\xa3\xa1");
        caught = std::string_view(error.what()) == original;
    }
    Check(caught && runtime.events == std::vector<std::string>{
              "owner", "transform", "manager"},
          "missing manager throws original GBK message after transform lookup");

    runtime.manager_available = true;
    runtime.valid_transform = false;
    runtime.events.clear();
    pal4::ThunderRenderer::Render(state, 0x3333u, runtime);
    Check(runtime.events == std::vector<std::string>{
              "owner", "transform", "manager", "validate"},
          "invalid transform skips native render");
    runtime.valid_transform = true;
    runtime.events.clear();
    pal4::ThunderRenderer::Render(state, 0x3333u, runtime);
    Check(runtime.events == std::vector<std::string>{
              "owner", "transform", "manager", "validate", "render"},
          "validated transform reaches visual renderer");
}
