#include "pal4/magic_window_details.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MagicWindowDetailsRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, bool>> selected;
    std::vector<std::pair<std::uint32_t, std::string>> texts;
    std::map<std::uint32_t, std::uint32_t> keys;
    bool visible{true};
    std::optional<pal4::MagicWindowDetailText> magic_detail;
    std::optional<pal4::MagicWindowDetailText> stunt_detail;
    std::uint32_t looked_up_key{};

    bool IsVisible(std::uint32_t window) override {
        calls.push_back("visible:" + std::to_string(window));
        return visible;
    }
    void SetSelected(std::uint32_t window, bool value) override {
        selected.emplace_back(window, value);
        calls.push_back("selected:" + std::to_string(window));
    }
    std::uint32_t ButtonKey(std::uint32_t window) override {
        calls.push_back("button-key:" + std::to_string(window));
        return keys.at(window);
    }
    std::optional<pal4::MagicWindowDetailText> FindMagicDetail(
        std::uint32_t key) override {
        looked_up_key = key;
        calls.push_back("magic-lookup");
        return magic_detail;
    }
    std::optional<pal4::MagicWindowDetailText> FindStuntDetail(
        std::uint32_t key) override {
        looked_up_key = key;
        calls.push_back("stunt-lookup");
        return stunt_detail;
    }
    void SetText(std::uint32_t window, std::string_view text) override {
        texts.emplace_back(window, text);
        calls.push_back("text:" + std::to_string(window));
    }
};

pal4::MagicWindowPanelState Panel() {
    pal4::MagicWindowPanelState panel;
    for (std::uint32_t offset = 0; offset < 0x170u; offset += 4u)
        panel.At(offset) = 0x1000u + offset;
    return panel;
}

void TestNegativeIndex() {
    Runtime runtime;
    const auto panel = Panel();
    pal4::MagicWindowDetails::UpdateMagic(-1, panel, runtime);
    pal4::MagicWindowDetails::UpdateStunt(-1, panel, runtime);
    Check(runtime.calls.empty(), "negative sentinel performs no work");
}

void TestMagicDetails() {
    const auto panel = Panel();
    for (const auto index : {0, 5}) {
        Runtime runtime;
        const auto button = panel.At(0xf0u + 4u * static_cast<std::uint32_t>(index));
        runtime.keys[button] = 44u;
        runtime.magic_detail = pal4::MagicWindowDetailText{"title", "description"};
        pal4::MagicWindowDetails::UpdateMagic(index, panel, runtime);
        Check(runtime.selected.size() == 7u, "six clear operations and selection");
        for (std::uint32_t row = 0; row < 6u; ++row)
            Check(runtime.selected[row] ==
                      std::pair<std::uint32_t, bool>{panel.At(0xc0u + 4u * row), false},
                  "magic selection reset order");
        Check(runtime.selected.back() ==
                  std::pair<std::uint32_t, bool>{panel.At(0xc0u + 4u * index), true} &&
                  runtime.looked_up_key == 44u &&
                  runtime.calls[7] == "button-key:" + std::to_string(button),
              "selected magic row uses action-button record key");
        Check(runtime.texts ==
                  (std::vector<std::pair<std::uint32_t, std::string>>{
                      {panel.At(0x88u), "title"}, {panel.At(0x8cu), "description"}}),
              "magic title and description controls");
    }

    Runtime missing;
    missing.keys[panel.At(0xf4u)] = 99u;
    pal4::MagicWindowDetails::UpdateMagic(1, panel, missing);
    Check(missing.selected.size() == 7u && missing.looked_up_key == 99u &&
              missing.texts.empty(), "missing magic record leaves info unchanged");
}

void TestStuntDetails() {
    const auto panel = Panel();
    Runtime hidden;
    hidden.visible = false;
    pal4::MagicWindowDetails::UpdateStunt(3, panel, hidden);
    Check(hidden.calls == std::vector<std::string>{"visible:4372"},
          "hidden stunt row exits before selection and lookup");

    for (const auto index : {0, 6}) {
        Runtime runtime;
        const auto button = panel.At(0x108u + 4u * static_cast<std::uint32_t>(index));
        runtime.keys[button] = 77u;
        runtime.stunt_detail = pal4::MagicWindowDetailText{"stunt title", "stunt description"};
        pal4::MagicWindowDetails::UpdateStunt(index, panel, runtime);
        Check(runtime.calls.front() == "visible:" + std::to_string(button) &&
                  runtime.selected.size() == 8u,
              "visibility checked before seven selection resets");
        for (std::uint32_t row = 0; row < 7u; ++row)
            Check(runtime.selected[row] ==
                      std::pair<std::uint32_t, bool>{panel.At(0x108u + 4u * row), false},
                  "stunt selection reset order");
        Check(runtime.selected.back() ==
                  std::pair<std::uint32_t, bool>{button, true} &&
                  runtime.looked_up_key == 77u &&
                  runtime.texts ==
                      (std::vector<std::pair<std::uint32_t, std::string>>{
                          {panel.At(0x88u), "stunt title"},
                          {panel.At(0x8cu), "stunt description"}}),
              "visible stunt detail binding");
    }

    Runtime missing;
    missing.keys[panel.At(0x108u)] = 88u;
    pal4::MagicWindowDetails::UpdateStunt(0, panel, missing);
    Check(missing.selected.size() == 8u && missing.looked_up_key == 88u &&
              missing.texts.empty(), "missing stunt record leaves info unchanged");
}

}  // namespace

int main() {
    TestNegativeIndex();
    TestMagicDetails();
    TestStuntDetails();
}
