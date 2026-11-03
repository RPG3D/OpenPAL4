#include "pal4/scene_ui_component.h"

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

struct Runtime final : pal4::ISceneUiComponentRuntime {
    std::vector<std::string> calls;
    std::string prefix = "Minimap/";
    std::string name;
    int find_count = 0;
    int throw_at_find = -1;
    bool throw_load = false;
    bool missing_name = false;

    void LoadLayout(std::uint32_t address, std::string_view layout,
                    std::uint32_t parent, std::uint32_t flags) override {
        Check(address == 0x5000u && layout == "Minimap" &&
                  parent == 0u && flags == 0u,
              "layout arguments");
        calls.emplace_back("layout");
        if (throw_load) throw std::runtime_error("layout");
    }
    void DestroyBase(std::uint32_t address) override {
        Check(address == 0x5000u, "base destructor address");
        calls.emplace_back("destroy-base");
    }
    void InitializeNameBuffer() override {
        calls.emplace_back("init-buffer");
        name.clear();
    }
    void AssignPrefix(std::uint32_t source_address, std::uint32_t offset,
                      std::uint32_t count) override {
        Check(source_address == 0x5014u && offset == 0u &&
                  count == 0xffffffffu,
              "prefix record and full-range assignment");
        calls.emplace_back("assign");
        name = prefix;
    }
    void AppendSuffix(std::string_view suffix, bool inline_append) override {
        calls.push_back(std::string(inline_append ? "inline:" : "raw:") +
                        std::string(suffix));
        if (!missing_name) name += suffix;
    }
    std::string_view CurrentName() override {
        calls.emplace_back("current-name");
        return missing_name ? std::string_view{} : std::string_view{name};
    }
    std::uint32_t FindWindow(std::uint32_t component,
                             std::string_view window_name) override {
        Check(component == 0x5000u, "window lookup uses component address");
        calls.push_back("find:" + std::string(window_name));
        const auto index = find_count++;
        if (index == throw_at_find) throw std::runtime_error("find");
        return 0x6000u + static_cast<std::uint32_t>(index);
    }
    void SetHorizontalFormatting(std::uint32_t window,
                                 std::uint32_t format) override {
        Check(window == 0x6003u && format == 2u,
              "StaticScene horizontal formatting");
        calls.emplace_back("format");
    }
    std::uint32_t CleanupNameBuffer() override {
        calls.emplace_back("cleanup-buffer");
        return 0x7000u;
    }
    void SetParent(std::uint32_t address, std::uint8_t enabled,
                   std::uint32_t parent) override {
        Check(address == 0x5000u && enabled == 1u && parent == 0x9000u,
              "parent thunk forwards all arguments");
        calls.emplace_back("set-parent");
    }
};

void CheckConstructorAndControls() {
    pal4::SceneUiComponentState state;
    Runtime runtime;
    Check(pal4::SceneUiComponent::Construct(
              state, 0x5000u, "Minimap", runtime) == 0x5000u,
          "constructor returns this address");
    Check(state.Read(0u) == 0x843ad0u &&
              state.Read(0x28u) == 0x6000u &&
              state.Read(0x34u) == 0x6001u &&
              state.Read(0x38u) == 0x6002u &&
              state.Read(0x2cu) == 0x6003u &&
              state.Read(0x3cu) == 0x6004u &&
              state.Read(0x30u) == 0x6005u,
          "vtable and six control offsets");
    const std::vector<std::string> expected{
        "layout", "init-buffer",
        "assign", "raw:ImgMinimap", "current-name", "find:Minimap/ImgMinimap",
        "assign", "raw:BtnMinimap", "current-name", "find:Minimap/BtnMinimap",
        "assign", "raw:BtnMarker", "current-name", "find:Minimap/BtnMarker",
        "assign", "raw:StaticScene", "current-name", "find:Minimap/StaticScene",
        "format",
        "assign", "inline:BtnSkill", "current-name", "find:Minimap/BtnSkill",
        "assign", "inline:dituanniudi1", "current-name",
        "find:Minimap/dituanniudi1", "cleanup-buffer"};
    Check(runtime.calls == expected,
          "control names, append paths, formatting and cleanup order");

    runtime.calls.clear();
    pal4::SceneUiComponent::SetParent(0x5000u, 1u, 0x9000u, runtime);
    Check(runtime.calls == std::vector<std::string>{"set-parent"},
          "parent wrapper forwards once");
}

void CheckDirectInitializationAndMissingText() {
    pal4::SceneUiComponentState state;
    Runtime runtime;
    runtime.missing_name = true;
    Check(pal4::SceneUiComponent::InitializeControls(
              state, 0x5000u, runtime) == 0x7000u &&
              runtime.find_count == 6,
          "direct initializer forwards cleanup result");
    for (const auto& call : runtime.calls) {
        if (call.rfind("find:", 0u) == 0u) {
            Check(call == "find:", "null text falls back to empty name");
        }
    }
    Check(runtime.calls.back() == "cleanup-buffer",
          "direct initializer cleans temporary text");
}

void CheckFailureUnwind() {
    pal4::SceneUiComponentState state;
    Runtime runtime;
    runtime.throw_at_find = 2;
    bool caught = false;
    try {
        (void)pal4::SceneUiComponent::Construct(
            state, 0x5000u, "Minimap", runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && state.Read(0u) == 0x843ad0u &&
              state.Read(0x28u) == 0x6000u &&
              state.Read(0x34u) == 0x6001u &&
              state.Read(0x38u) == 0u &&
              runtime.calls[runtime.calls.size() - 2u] == "cleanup-buffer" &&
              runtime.calls.back() == "destroy-base",
          "failed lookup cleans text then constructed base");

    Runtime layout_failure;
    layout_failure.throw_load = true;
    caught = false;
    try {
        (void)pal4::SceneUiComponent::Construct(
            state, 0x5000u, "Minimap", layout_failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && layout_failure.calls ==
              std::vector<std::string>{"layout"},
          "failed base layout has no constructed-base cleanup");
}

}  // namespace

int main() {
    CheckConstructorAndControls();
    CheckDirectInitializationAndMissingText();
    CheckFailureUnwind();
}
