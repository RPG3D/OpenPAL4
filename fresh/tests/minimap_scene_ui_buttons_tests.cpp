#include "pal4/minimap_scene_ui_buttons.h"

#include <array>
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

struct Runtime final : pal4::IMinimapSceneUiButtonRuntime {
    struct Binding {
        std::uint32_t window;
        pal4::MinimapUiEvent event;
        std::uint32_t handler;
    };
    std::vector<std::string> calls;
    std::vector<Binding> bindings;
    std::vector<std::uint32_t> releases;
    std::string text;
    std::uint32_t next_callback = 0x8000u;
    std::uint32_t current_handler = 0u;
    int lookup_count = 0;
    int subscribe_count = 0;
    bool null_allocation = false;
    bool throw_lookup = false;
    bool throw_subscribe = false;
    bool missing_text = false;

    void InitializeNameBuffer() override {
        calls.emplace_back("init");
    }
    void AssignPrefix(std::uint32_t source, std::uint32_t offset,
                      std::uint32_t count) override {
        Check(source == 0x5014u && offset == 0u &&
                  count == 0xffffffffu,
              "root prefix record and range");
        calls.emplace_back("assign");
        text = "portrait/";
    }
    void AppendSuffix(std::string_view suffix, bool inline_append) override {
        calls.push_back(std::string(inline_append ? "inline:" : "raw:") +
                        std::string(suffix));
        if (!missing_text) text += suffix;
    }
    std::string_view CurrentName() override {
        return missing_text ? std::string_view{} : std::string_view{text};
    }
    std::uint32_t FindWindow(std::uint32_t owner,
                             std::string_view name) override {
        Check(owner == 0x5000u, "root lookup owner");
        calls.push_back("find:" + std::string(name));
        const auto index = lookup_count++;
        if (throw_lookup && index == 4) throw std::runtime_error("lookup");
        return 0x6000u + static_cast<std::uint32_t>(index);
    }
    std::uint32_t AllocateCallback(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "callback allocation size");
        calls.emplace_back("allocate");
        if (null_allocation) return 0u;
        const auto callback = next_callback;
        next_callback += 0x20u;
        return callback;
    }
    std::uint32_t ConstructCallback(
        std::uint32_t storage, std::uint32_t handler,
        std::uint32_t argument, std::uint32_t owner) override {
        Check(storage != 0u && argument == 0u && owner == 0x5000u,
              "callback constructor parameters");
        current_handler = handler;
        calls.emplace_back("construct");
        return storage;
    }
    void Subscribe(std::uint32_t window, pal4::MinimapUiEvent event,
                   std::uint32_t callback) override {
        Check(callback != 0u || null_allocation,
              "callback pointer or allocation failure");
        bindings.push_back({window, event, current_handler});
        calls.emplace_back("subscribe");
        ++subscribe_count;
        if (throw_subscribe && subscribe_count == 5)
            throw std::runtime_error("subscribe");
    }
    void ReleaseCallback(std::uint32_t callback) override {
        releases.push_back(callback);
        calls.emplace_back("release");
    }
    std::uint32_t CleanupNameBuffer() override {
        calls.emplace_back("cleanup");
        return 0x7000u;
    }
};

void CheckNormalPath() {
    pal4::MinimapSceneUiState state;
    Runtime runtime;
    Check(pal4::MinimapSceneUiButtons::Initialize(
              state, 0x5000u, runtime) == 0x7000u,
          "initializer returns text cleanup result");
    Check(state.ReadWord(0xb4u) == 0x6000u &&
              state.ReadWord(0xc0u) == 0x6001u &&
              state.ReadWord(0xb8u) == 0x6003u &&
              state.ReadWord(0xc4u) == 0x6004u &&
              state.ReadWord(0xc8u) == 0x6005u &&
              state.ReadWord(0xbcu) == 0x6006u,
          "six persistent controls use IDA offsets");
    Check(runtime.lookup_count == 7 &&
              runtime.bindings.size() == 9u &&
              runtime.releases.size() == 9u,
          "help lookup is local and three buttons receive three events each");
    const std::array<Runtime::Binding, 9> expected{{
        {0x6000u, pal4::MinimapUiEvent::Clicked, 0x479030u},
        {0x6000u, pal4::MinimapUiEvent::MouseDown, 0x47a940u},
        {0x6000u, pal4::MinimapUiEvent::MouseUp, 0x47a950u},
        {0x6002u, pal4::MinimapUiEvent::Clicked, 0x479090u},
        {0x6002u, pal4::MinimapUiEvent::MouseDown, 0x47a940u},
        {0x6002u, pal4::MinimapUiEvent::MouseUp, 0x47a950u},
        {0x6003u, pal4::MinimapUiEvent::Clicked, 0x4797c0u},
        {0x6003u, pal4::MinimapUiEvent::MouseDown, 0x47a940u},
        {0x6003u, pal4::MinimapUiEvent::MouseUp, 0x47a950u},
    }};
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto& got = runtime.bindings[index];
        const auto& want = expected[index];
        Check(got.window == want.window && got.event == want.event &&
                  got.handler == want.handler,
              "event binding window, event and handler order");
    }
    const std::array<std::string, 7> names{{
        "BtnSystem", "ImgJumpHint", "BtnHelp", "BtnRun",
        "ImgHpMask", "ImgRageMask", "StaticTimer"}};
    std::size_t name_index = 0u;
    for (const auto& call : runtime.calls) {
        if (call.rfind("find:", 0u) == 0u) {
            Check(call == "find:portrait/" + names[name_index++],
                  "full control name and lookup order");
        }
    }
    Check(name_index == names.size() &&
              runtime.calls[runtime.calls.size() - 3u] ==
                  "inline:StaticTimer" &&
              runtime.calls.back() == "cleanup",
          "StaticTimer uses inline append then cleans temporary text");
}

void CheckAlternateAndFailurePaths() {
    pal4::MinimapSceneUiState state;
    Runtime null_runtime;
    null_runtime.null_allocation = true;
    null_runtime.missing_text = true;
    (void)pal4::MinimapSceneUiButtons::Initialize(
        state, 0x5000u, null_runtime);
    Check(null_runtime.bindings.size() == 9u &&
              null_runtime.releases.empty(),
          "null callbacks still reach subscriptions without local release");
    for (const auto& call : null_runtime.calls) {
        if (call.rfind("find:", 0u) == 0u) {
            Check(call == "find:", "missing text uses empty lookup name");
        }
    }

    Runtime lookup_failure;
    lookup_failure.throw_lookup = true;
    bool caught = false;
    try {
        (void)pal4::MinimapSceneUiButtons::Initialize(
            state, 0x5000u, lookup_failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && lookup_failure.bindings.size() == 9u &&
              lookup_failure.releases.size() == 9u &&
              lookup_failure.calls.back() == "cleanup",
          "lookup exception cleans temporary name buffer");

    Runtime subscribe_failure;
    subscribe_failure.throw_subscribe = true;
    caught = false;
    try {
        (void)pal4::MinimapSceneUiButtons::Initialize(
            state, 0x5000u, subscribe_failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && subscribe_failure.bindings.size() == 5u &&
              subscribe_failure.releases.size() == 5u &&
              subscribe_failure.calls[subscribe_failure.calls.size() - 2u] ==
                  "release" &&
              subscribe_failure.calls.back() == "cleanup",
          "subscription exception releases callback before text cleanup");
}

}  // namespace

int main() {
    CheckNormalPath();
    CheckAlternateAndFailurePaths();
}
