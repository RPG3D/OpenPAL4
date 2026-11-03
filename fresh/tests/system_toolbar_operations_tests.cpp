#include "pal4/system_toolbar_operations.h"
#include "pal4/system_toolbar_click_actions.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

constexpr std::array<std::string_view, 7> kNames{
    "sysToolBar/Property", "sysToolBar/Magic", "sysToolBar/Role",
    "sysToolBar/Equipment", "sysToolBar/Task",
    "sysToolBar/System", "sysToolBar/Forging"};
constexpr std::array<std::uint32_t, 7> kKeys{
    0x8a80bcu, 0x8a81d0u, 0x8b00d4u, 0x8a8098u,
    0x8a8548u, 0x8b06a8u, 0x8b0f1cu};

struct Runtime final : pal4::ISystemToolbarOperationsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::tuple<std::uint32_t, std::uint32_t,
                           std::uint32_t, std::uint32_t>> events;
    std::vector<std::pair<std::uint32_t, bool>> selections;
    std::vector<std::string> compared_names;
    std::string clicked_name;
    std::uint32_t current_helper = 0u;
    std::uint32_t current_button = 0u;
    std::uint32_t mouse_window = 0u;
    std::uint32_t manager_calls = 0u;
    std::uint32_t game_calls = 0u;
    std::uint32_t scene_mode_calls = 0u;
    std::uint32_t smith_writes = 0u;
    std::int32_t scene_mode = 5;

    Runtime() {
        words[0x9008u] = 0xa000u;
        words[0x5000u + 36u] = 0x6000u;
        words[0x6000u + 68u] = 0x7777u;
        words[0xb000u + 0x188u] = 0xdeadbeefu; // Registry head must never be queried as a state.
        words[0xb000u + 0x18cu] = 0xc000u;
        for (std::uint32_t offset = 64u; offset <= 88u; offset += 4u) {
            words[0x5000u + offset] = 0x8000u + offset;
        }
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        Check(address == 0xe000u && value == 1u,
              "smith system flag");
        calls.emplace_back("smith-write");
        ++smith_writes;
    }
    bool CompareWindowName(const std::uint32_t name_object,
                           const std::string_view name) override {
        Check(name_object == 0xa000u + 892u,
              "CEGUI name field at source +892");
        calls.emplace_back("compare");
        compared_names.emplace_back(name);
        return clicked_name == name;
    }
    std::uint32_t FrameManager() override {
        ++manager_calls;
        calls.emplace_back("manager");
        return 0x4000u;
    }
    void DispatchEvent(const std::uint32_t tree,
                       const std::uint32_t key,
                       const std::uint32_t first,
                       const std::uint32_t second) override {
        Check(tree == 0x4104u, "event tree offset");
        calls.emplace_back("event");
        events.emplace_back(tree, key, first, second);
    }
    void SetSelected(const std::uint32_t button,
                     const bool selected) override {
        calls.emplace_back("select");
        selections.emplace_back(button, selected);
    }
    void SetCurrentSelected(const std::uint32_t helper,
                            const std::uint32_t button) override {
        calls.emplace_back("select-current");
        current_helper = helper;
        current_button = button;
    }
    std::uint32_t SmithSystem() override {
        calls.emplace_back("smith");
        return 0xe000u;
    }
    void SetVisible(const std::uint32_t frame,
                    const std::uint32_t visible,
                    const std::uint32_t parent) override {
        Check(frame == 0x5000u && visible == 0u && parent == 0u,
              "close virtual visibility call");
        calls.emplace_back("hide");
    }
    std::uint32_t Game() override {
        ++game_calls;
        calls.emplace_back("game");
        return 0xb000u;
    }
    std::int32_t SceneMode(const std::uint32_t scene) override {
        Check(scene == 0xc000u, "scene pointer");
        ++scene_mode_calls;
        calls.emplace_back("scene-mode");
        return scene_mode;
    }
    std::uint32_t TradeSystem() override {
        calls.emplace_back("trade-system");
        return 0xd000u;
    }
    std::uint8_t ClearTradeItemInfo(const std::uint32_t trade) override {
        Check(trade == 0xd000u, "trade system pointer");
        calls.emplace_back("clear-trade");
        return 0x12u;
    }
    std::uint8_t InitializeGameState(const std::uint32_t game,
                                     const std::uint32_t mode,
                                     const std::uint32_t argument) override {
        Check(game == 0xb000u && mode == 1u && argument == 0u,
              "game-state initialization arguments");
        calls.emplace_back("initialize-game");
        return 0x34u;
    }
    void HandleMouseEvent(const std::uint32_t window) override {
        calls.emplace_back("mouse");
        mouse_window = window;
    }
};

void TestClearAllSelection() {
    Runtime runtime;
    pal4::SystemToolbarOperations::ClearAllSelection(0x5000u, runtime);
    Check(runtime.manager_calls == 7u && runtime.events.size() == 7u &&
              runtime.selections.size() == 7u,
          "seven clear events and seven button resets");
    for (std::size_t index = 0u; index < 7u; ++index) {
        Check(runtime.events[index] ==
                  std::tuple{0x4104u, kKeys[index], 0u, 0u} &&
                  runtime.selections[index] ==
                  std::pair<std::uint32_t, bool>{
                      0x8040u + static_cast<std::uint32_t>(index) * 4u,
                      false},
              "clear event and button order");
        const auto start = index * 4u;
        Check(runtime.calls[start] == "manager" &&
                  runtime.calls[start + 1u] == "event" &&
                  runtime.calls[start + 2u] == "read" &&
                  runtime.calls[start + 3u] == "select",
              "clear dispatches before deselecting each button");
    }
}

void TestAllButtonNames() {
    for (std::size_t index = 0u; index < kNames.size(); ++index) {
        Runtime runtime;
        runtime.clicked_name = kNames[index];
        Check(pal4::SystemToolbarOperations::OnButtonClick(
                  0x5000u, 0x9000u, runtime) == 1u &&
                  runtime.events.size() == 8u &&
                  runtime.events.back() ==
                      std::tuple{0x4104u, kKeys[index], 1u, 0x7777u} &&
                  runtime.selections.size() == 8u &&
                  runtime.selections.back() ==
                      std::pair<std::uint32_t, bool>{
                          0x8040u + static_cast<std::uint32_t>(index) * 4u,
                          true} &&
                  runtime.current_helper == 0x5028u &&
                  runtime.current_button ==
                      0x8040u + static_cast<std::uint32_t>(index) * 4u &&
                  runtime.manager_calls == 8u &&
                  runtime.compared_names.size() == index + 1u &&
                  runtime.smith_writes == (index == 6u ? 1u : 0u),
              "each button maps to its event and selected window");
        if (index == 6u) {
            const auto smith = std::find(runtime.calls.begin(),
                                         runtime.calls.end(), "smith-write");
            const auto show = std::find(smith, runtime.calls.end(), "event");
            Check(smith != runtime.calls.end() && show != runtime.calls.end(),
                  "smith flag is set before the forging event");
        }
    }
    Runtime unknown;
    unknown.clicked_name = "sysToolBar/Other";
    Check(pal4::SystemToolbarOperations::OnButtonClick(
              0x5000u, 0x9000u, unknown) == 1u &&
              unknown.events.size() == 7u &&
              unknown.selections.size() == 7u &&
              unknown.compared_names.size() == 7u &&
              unknown.current_button == 0u,
          "unknown source still clears all selections");
}

void TestClose() {
    Runtime trade;
    Check(pal4::SystemToolbarOperations::Close(0x5000u, trade) == 0x12u &&
              trade.game_calls == 2u && trade.scene_mode_calls == 1u &&
              trade.calls.back() == "clear-trade",
          "mode-five scene clears trade item after hide");
    const auto hide = std::find(trade.calls.begin(), trade.calls.end(),
                                "hide");
    const auto game = std::find(trade.calls.begin(), trade.calls.end(),
                                "game");
    Check(hide != trade.calls.end() && game != trade.calls.end() && hide < game,
          "close hides toolbar before checking game state");

    Runtime other;
    other.scene_mode = 4;
    Check(pal4::SystemToolbarOperations::Close(0x5000u, other) == 0x34u &&
              other.game_calls == 3u && other.scene_mode_calls == 1u &&
              other.calls.back() == "initialize-game",
          "other scene mode initializes game state");
    Runtime empty;
    empty.words[0xb000u + 0x18cu] = 0u;
    Check(pal4::SystemToolbarOperations::Close(0x5000u, empty) == 0x34u &&
              empty.game_calls == 2u && empty.scene_mode_calls == 0u,
          "missing scene skips mode query and initializes game state");
}

void TestKeyInput() {
    constexpr std::array<std::uint32_t, 7> offsets{
        72u, 64u, 76u, 68u, 88u, 80u, 84u};
    for (std::uint32_t index = 0u; index < 7u; ++index) {
        Runtime runtime;
        runtime.words[0x9010u] = 59u + index;
        Check(pal4::SystemToolbarOperations::HandleKeyInput(
                  0x5028u, 0x9000u, runtime) == 1u &&
                  runtime.mouse_window == 0x8000u + offsets[index] &&
                  runtime.calls == std::vector<std::string>{
                      "read", "read", "mouse"},
              "keys 59..65 route to seven toolbar windows");
    }
    for (const auto key : {0u, 58u, 66u, 0xffffffffu}) {
        Runtime runtime;
        runtime.words[0x9010u] = key;
        Check(pal4::SystemToolbarOperations::HandleKeyInput(
                  0x5028u, 0x9000u, runtime) == 0u &&
                  runtime.mouse_window == 0u &&
                  runtime.calls == std::vector<std::string>{"read"},
              "unsupported key does not read or activate a window");
    }
}

struct ClickRuntime final : pal4::ISystemToolbarClickRuntime {
    std::uint32_t frame = 0x5000u;
    std::uint32_t events = 0u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(address == 0x8b07d8u, "composed toolbar name source");
        return 0x7000u;
    }
    std::uint32_t FrameManager() override { return 0x4000u; }
    std::uint32_t FindFrame(const std::uint32_t manager,
                            const std::uint32_t name) override {
        Check(manager == 0x4000u && name == 0x7000u,
              "composed toolbar lookup");
        return frame;
    }
    void ClearAllSelection(std::uint32_t) override {
        Check(false, "composed path must call reconstructed clear");
    }
    void Close(std::uint32_t) override {
        Check(false, "composed path must call reconstructed close");
    }
    void DispatchEvent(const std::uint32_t tree,
                       const std::uint32_t key,
                       const std::uint32_t first,
                       const std::uint32_t second) override {
        Check(tree == 0x4104u && key == 0x8a80acu &&
                  first == 1u && second == 0u,
              "composed click event");
        ++events;
    }
    void HandleMouseEvent(std::uint32_t) override {}
};

void TestComposedClickActions() {
    ClickRuntime click;
    Runtime operations;
    Check(pal4::SystemToolbarClickActions::HandleClick1(
              click, operations) == 1u &&
              click.events == 1u && operations.events.size() == 7u &&
              operations.selections.size() == 7u,
          "button 1 composes reconstructed clear-all operation");
    click = ClickRuntime{};
    operations = Runtime{};
    Check(pal4::SystemToolbarClickActions::HandleClick2(
              click, operations) &&
              operations.events.size() == 7u &&
              operations.calls.back() == "clear-trade",
          "button 2 composes reconstructed close operation");
    click.frame = 0u;
    operations = Runtime{};
    Check(pal4::SystemToolbarClickActions::HandleClick2(
              click, operations) && operations.calls.empty(),
          "missing toolbar does not invoke composed close");
}

}  // namespace

int main() {
    TestClearAllSelection();
    TestAllButtonNames();
    TestClose();
    TestKeyInput();
    TestComposedClickActions();
    return EXIT_SUCCESS;
}
