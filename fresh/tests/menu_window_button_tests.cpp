#include "pal4/menu_window_button.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

using State = pal4::MenuButtonVisualState;
using Metric = pal4::MenuButtonMetric;

struct Runtime final : pal4::IMenuWindowButtonRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<State> text_states;
    std::vector<State> image_states;
    std::vector<Metric> read_metrics;
    std::vector<Metric> written_metrics;
    std::unordered_map<int, float> metric_values;
    std::uint32_t allocation = 0x9000u;
    std::uint32_t subscribed = 0xffffffffu;
    std::uint32_t navigation_button = 0u;
    std::string button_name;
    bool enabled_value = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    void AppendButtonRecord(std::uint32_t records,
                            std::uint32_t insertion,
                            std::uint32_t count,
                            std::uint32_t source) override {
        calls.emplace_back("append_record");
        Check(records == 0x1000u + 64u &&
              insertion == words.at(0x1000u + 72u) &&
              count == 1u && source == 0x2000u,
              "record insertion arguments");
    }
    std::uint32_t CreateWindow(std::string_view type,
                                std::string_view name) override {
        calls.emplace_back("create_window");
        Check(type == "OiramLook/Button", "button widget type");
        button_name = name;
        return 0x8000u;
    }
    void SetText(std::uint32_t window,
                 std::uint32_t string_address) override {
        calls.emplace_back("text");
        Check(window == 0x8000u && string_address == 0x200cu,
              "button record text");
    }
    void SetTextColour(std::uint32_t window, State state,
                       pal4::MenuButtonColour colour) override {
        calls.emplace_back("colour");
        Check(window == 0x8000u && colour.alpha == 1.0f,
              "text colour owner and alpha");
        const float component = state == State::Disabled
                                    ? 0.30000001f : 0.0f;
        Check(colour.red == component && colour.green == component &&
              colour.blue == component, "state text colour");
        text_states.push_back(state);
    }
    void CopyImage(std::uint32_t model, std::uint32_t button,
                   State state, std::string_view scope) override {
        calls.emplace_back("image");
        Check(model == 0x7000u && button == 0x8000u &&
              scope == "Self", "image clone context");
        image_states.push_back(state);
    }
    void AddChild(std::uint32_t parent,
                  std::uint32_t child) override {
        calls.emplace_back("add_child");
        Check(parent == 0x7100u && child == 0x8000u,
              "panel child attachment");
    }
    float GetMetric(std::uint32_t window, Metric metric,
                    std::uint32_t mode) override {
        calls.emplace_back("get_metric");
        Check(window == 0x7000u && mode == 1u,
              "model metrics mode");
        read_metrics.push_back(metric);
        switch (metric) {
        case Metric::X: return 0.25f;
        case Metric::Y: return 0.15f;
        case Metric::Width: return 0.3f;
        case Metric::Height: return 0.1f;
        }
        throw std::runtime_error("unknown metric");
    }
    void SetMetric(std::uint32_t window, Metric metric,
                   std::uint32_t mode, float value) override {
        calls.emplace_back("set_metric");
        Check(window == 0x8000u && mode == 1u,
              "new button metrics mode");
        written_metrics.push_back(metric);
        metric_values[static_cast<int>(metric)] = value;
    }
    void SetClippedByParent(std::uint32_t window,
                            bool clipped) override {
        calls.emplace_back("clip");
        Check(window == 0x8000u && !clipped,
              "button clip setting");
    }
    void SetEnabled(std::uint32_t window,
                    bool enabled) override {
        calls.emplace_back("enabled");
        Check(window == 0x8000u, "button enabled owner");
        enabled_value = enabled;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        Check(bytes == 24u, "click callback size");
        return allocation;
    }
    std::uint32_t ConstructClickCallback(
        std::uint32_t storage, std::uint32_t function,
        std::uint32_t argument, std::uint32_t owner) override {
        calls.emplace_back("construct_callback");
        Check(storage == 0x9000u && function == 0x4b4b40u &&
              argument == 0u && owner == 0x1000u,
              "click callback fields");
        return storage;
    }
    void SubscribeClicked(std::uint32_t button,
                          std::uint32_t callback) override {
        calls.emplace_back("subscribe");
        Check(button == 0x8000u, "clicked event owner");
        subscribed = callback;
    }
    void ReleaseClickCallback(std::uint32_t callback) override {
        calls.emplace_back("release");
        Check(callback == 0x9000u, "click callback release");
    }
    void AppendNavigationButton(std::uint32_t list,
                                std::uint32_t insertion,
                                std::uint32_t button) override {
        calls.emplace_back("navigation");
        Check(list == 0x1000u + 44u &&
              insertion == 0x6000u && button == 0x8000u,
              "navigation insertion arguments");
        navigation_button = button;
    }
};

void CheckCreate(std::uint32_t begin, std::uint32_t end,
                 bool navigation, bool enabled, bool callback) {
    Runtime runtime;
    runtime.words[0x1000u + 68u] = begin;
    runtime.words[0x1000u + 72u] = end;
    runtime.words[0x1000u + 80u] = 0x7100u;
    runtime.words[0x1000u + 84u] = 0x7000u;
    runtime.words[0x1000u + 52u] = 0x6000u;
    runtime.allocation = callback ? 0x9000u : 0u;
    pal4::MenuWindowButton::Create(0x1000u, 0x2000u,
                                  navigation, enabled, runtime);
    const auto index = begin == 0u ? 0u : (end - begin) / 164u;
    Check(runtime.button_name == "popItem_" +
                                     std::to_string(index) &&
          runtime.words.at(0x8000u + 280u) == index &&
          runtime.enabled_value == enabled,
          "name, owner and enabled flag");
    Check(runtime.text_states == std::vector<State>{
              State::Normal, State::Highlight,
              State::Pushed, State::Disabled} &&
          runtime.image_states == std::vector<State>{
              State::Normal, State::Highlight,
              State::Disabled, State::Pushed},
          "four text colours and copied images");
    Check(runtime.read_metrics == std::vector<Metric>{
              Metric::Y, Metric::Height, Metric::X,
              Metric::Width, Metric::Height} &&
          runtime.written_metrics == std::vector<Metric>{
              Metric::Y, Metric::X, Metric::Width, Metric::Height} &&
          runtime.metric_values.at(static_cast<int>(Metric::Y)) ==
              0.15f + 0.1f * static_cast<float>(index) &&
          runtime.metric_values.at(static_cast<int>(Metric::X)) == 0.25f &&
          runtime.metric_values.at(static_cast<int>(Metric::Width)) == 0.3f &&
          runtime.metric_values.at(static_cast<int>(Metric::Height)) == 0.1f,
          "model-derived geometry");
    Check(runtime.subscribed == (callback ? 0x9000u : 0u) &&
          runtime.navigation_button == (navigation ? 0x8000u : 0u) &&
          runtime.calls.front() == "read",
          "callback and navigation gates");
    const auto has_release = [&]() {
        for (const auto& call : runtime.calls)
            if (call == "release") return true;
        return false;
    };
    Check(has_release() == callback, "callback ownership release");
}

}  // namespace

int main() {
    try {
        CheckCreate(0u, 0u, false, false, true);
        CheckCreate(0x5000u, 0x5000u + 328u,
                    true, true, true);
        CheckCreate(0x5000u, 0x5000u + 164u,
                    false, true, false);
        std::cout << "menu_window_button_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
