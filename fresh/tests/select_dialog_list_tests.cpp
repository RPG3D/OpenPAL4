#include "pal4/select_dialog.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Position {
    std::uint32_t window;
    float x;
    float y;
};

struct Runtime final : pal4::ISelectDialogListRuntime {
    static constexpr std::uint32_t kObject = 0x1000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::string> strings;
    std::unordered_map<std::uint32_t, std::u32string> texts;
    std::vector<std::string> events;
    std::vector<std::uint32_t> active;
    std::vector<Position> positions;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> children;
    std::vector<std::pair<std::uint32_t, float>> widths;
    std::vector<std::pair<std::uint32_t, float>> heights;
    std::uint32_t created = 0u;

    Runtime() {
        words[kObject + 36u] = 0x2000u;
        words[kObject + 48u] = 0x2100u;
        words[kObject + 52u] = 0x2110u;
        words[kObject + 72u] = 0x3000u;
        words[kObject + 76u] = 0x3000u;
        words[kObject + 88u] = 0u;
        words[kObject + 92u] = 0u;
        words[kObject + 108u] = 0x4100u;
        words[kObject + 116u] = std::bit_cast<std::uint32_t>(12.5f);
        words[kObject + 120u] = std::bit_cast<std::uint32_t>(20.0f);
        words[kObject + 132u] = 0x5000u;
        words[kObject + 140u] = 0x5100u;
        words[kObject + 144u] = 0x5200u;
        words[kObject + 152u] = 0x5300u;
    }

    void CleanupCallback(std::uint32_t object) override {
        Check(object == kObject, "cleanup owner");
        events.emplace_back("cleanup");
    }
    void ClearNavigation(std::uint32_t navigation) override {
        Check(navigation == kObject + 44u, "navigation address");
        words[kObject + 52u] = words.at(kObject + 48u);
        events.emplace_back("clear-navigation");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::string ReadCString(std::uint32_t address) override {
        events.emplace_back("read-string");
        return strings.at(address);
    }
    std::string ConvertToUtf8(
        const std::string& source) override {
        events.emplace_back("convert");
        return source;
    }
    std::uint32_t CreateButton(std::uint32_t object) override {
        Check(object == kObject, "button owner");
        events.emplace_back("create");
        const auto button = 0x6000u + created++ * 0x100u;
        words[button + 32u] = button + 0x40u;
        words[button + 0x40u] = button + 0x80u;
        return button;
    }
    void SetText(std::uint32_t window,
                 const std::u32string& text) override {
        events.emplace_back("text");
        texts[window] = text;
    }
    void SetupMouseHandlers(std::uint32_t navigation,
                            std::uint32_t window) override {
        Check(navigation == kObject + 40u && window != 0u,
              "mouse handler arguments");
        events.emplace_back("mouse");
    }
    void AppendActiveButton(std::uint32_t collection,
                            std::uint32_t storage,
                            std::uint32_t button) override {
        Check(collection == kObject + 100u &&
                  storage == 0x4100u,
              "active collection arguments");
        events.emplace_back("append");
        active.push_back(button);
    }
    float GetWidth(std::uint32_t window) override {
        Check(window == 0x5100u, "template width source");
        events.emplace_back("get-width");
        return 150.0f;
    }
    float GetHeight(std::uint32_t window) override {
        Check(window == 0x5100u, "template height source");
        events.emplace_back("get-height");
        return 18.0f;
    }
    void AddChild(std::uint32_t parent,
                  std::uint32_t child) override {
        events.emplace_back("add-child");
        children.emplace_back(parent, child);
    }
    void SetPosition(std::uint32_t window,
                     float x, float y) override {
        events.emplace_back("position");
        positions.push_back({window, x, y});
    }
    void SetWidth(std::uint32_t window, float width) override {
        events.emplace_back("width");
        widths.emplace_back(window, width);
    }
    void SetHeight(std::uint32_t window, float height) override {
        events.emplace_back("height");
        heights.emplace_back(window, height);
    }
    std::uint32_t DesktopWindow() override {
        events.emplace_back("desktop");
        return 0x5400u;
    }
    void SetAlwaysOnTop(std::uint32_t window,
                        bool enabled) override {
        Check(window == 0x2000u && enabled, "topmost root");
        events.emplace_back("topmost");
    }
    void SetModalState(std::uint32_t window,
                       bool enabled) override {
        Check(window == 0x2000u && enabled, "modal root");
        events.emplace_back("modal");
    }
    void SetPositionWithMetrics(std::uint32_t window,
                                std::uint32_t mode,
                                float x, float y) override {
        Check(window == 0x2000u && mode == 0u &&
                  std::bit_cast<std::uint32_t>(x) == 0x3eb33333u &&
                  std::bit_cast<std::uint32_t>(y) == 0x3e99999au,
              "root metric position");
        events.emplace_back("metric-position");
    }
    std::uint32_t RefreshNavigation(
        std::uint32_t navigation) override {
        Check(navigation == kObject + 40u,
              "refresh navigation address");
        events.emplace_back("refresh");
        return 0xcafe1234u;
    }
};

void CheckEmptyList() {
    Runtime runtime;
    Check(pal4::SelectDialog::InitializeList(
              Runtime::kObject, U"Question", 9u,
              runtime) == 0xcafe1234u,
          "empty list returns navigation result");
    Check(runtime.words.at(Runtime::kObject + 64u) == 9u &&
              runtime.words.at(Runtime::kObject + 156u) ==
                  0xffffffffu &&
              runtime.words.at(Runtime::kObject + 52u) ==
                  0x2100u &&
              runtime.words.at(Runtime::kObject + 60u) == 0u,
          "empty list resets state");
    Check(runtime.texts.at(0x5000u) == U"Question" &&
              runtime.active.empty() && runtime.created == 0u,
          "empty list sets question without buttons");
    Check(runtime.positions.size() == 1u &&
              runtime.positions[0].window == 0x5200u &&
              runtime.positions[0].x == 12.5f &&
              runtime.positions[0].y == 20.0f,
          "empty list footer uses index zero");
    Check(runtime.children ==
              std::vector<std::pair<std::uint32_t,
                                    std::uint32_t>>{{0x5400u,
                                                     0x2000u}} &&
              runtime.events.front() == "cleanup" &&
              runtime.events[1] == "clear-navigation" &&
              runtime.events.back() == "refresh",
          "empty list shows root after reset");
}

void CheckCreateAndReuse() {
    Runtime runtime;
    runtime.words[Runtime::kObject + 76u] = 0x3030u;
    runtime.words[0x3004u] = 0x7000u;
    runtime.words[0x3014u] = 0u;
    runtime.words[0x3024u] = 0x7100u;
    runtime.strings[0x7000u] = "First";
    runtime.strings[0x7100u] = "\xe4\xb8\xad\xe6\x96\x87";
    runtime.words[Runtime::kObject + 88u] = 0x8000u;
    runtime.words[Runtime::kObject + 92u] = 0x8008u;
    runtime.words[0x8000u] = 0x9000u;
    runtime.words[0x8004u] = 0x9100u;
    runtime.words[0x9000u + 32u] = 0x9200u;
    runtime.words[0x9200u] = 0x9300u;
    runtime.words[0x9100u + 32u] = 0x9400u;
    runtime.words[0x9400u] = 0x9500u;

    Check(pal4::SelectDialog::InitializeList(
              Runtime::kObject, U"Pick", 4u,
              runtime) == 0xcafe1234u,
          "populated list returns navigation result");
    Check(runtime.active ==
              std::vector<std::uint32_t>{0x9000u, 0x9100u,
                                         0x6000u} &&
              runtime.words.at(Runtime::kObject + 92u) ==
                  0x8000u &&
              runtime.words.at(0x8000u) == 0x9100u &&
              runtime.created == 1u,
          "available buttons are shifted out before creation");
    Check(runtime.texts.at(0x9300u) == U"First" &&
              runtime.texts.at(0x9500u).empty() &&
              runtime.texts.at(0x6080u) == U"\u4e2d\u6587" &&
              runtime.words.at(0x9300u + 280u) == 0u &&
              runtime.words.at(0x9500u + 280u) == 1u &&
              runtime.words.at(0x6080u + 280u) == 2u,
          "option labels and selection indices");
    Check(runtime.positions.size() == 4u &&
              runtime.positions[0].y == 20.0f &&
              runtime.positions[1].y == 38.0f &&
              runtime.positions[2].y == 56.0f &&
              runtime.positions[3].window == 0x5200u &&
              runtime.positions[3].y == 74.0f,
          "footer follows completed option count (IDA4B9B83/4B9D68)");
    Check(runtime.widths.size() == 3u &&
              runtime.heights.size() == 3u &&
              runtime.widths[0].second == 150.0f &&
              runtime.heights[2].second == 18.0f &&
              runtime.children.size() == 4u &&
              runtime.children[0].first == 0x5300u &&
              runtime.children.back() ==
                  std::pair<std::uint32_t,
                            std::uint32_t>{0x5400u, 0x2000u},
          "button geometry and root attachment");
}

}  // namespace

int main() {
    try {
        CheckEmptyList();
        CheckCreateAndReuse();
        std::cout << "select_dialog_list_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
