#include "pal4/property_window_callbacks.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyWindowCallbacksRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void InsertActiveMember(pal4::PropertyWindowState&,
                            std::uint32_t offset) override {
        Check(offset == 176u, "panel inserts UI object at +0xb0");
        calls.emplace_back("insert-member");
    }
    void SetImage(std::uint32_t window,
                  std::uint32_t image) override {
        Check(window == 0x9000u && image == 0xa004u,
              "panel image uses category-indexed cached image");
        calls.emplace_back("set-image");
    }
    void PopulateList(pal4::PropertyWindowState& state,
                      std::uint32_t category,
                      bool preserve_scroll) override {
        Check(category == 1u && state.Read(108u) == category &&
                  !preserve_scroll,
              "panel refresh populates category without scroll preservation");
        calls.emplace_back("populate-list");
    }
    void HandleSelection(pal4::PropertyWindowState&,
                         std::uint32_t event_data) override {
        Check(event_data == 0x500cu,
              "item selection receives event payload at +0xc");
        calls.emplace_back("selection");
    }
};

pal4::PropertyWindowState MakeState() {
    pal4::PropertyWindowState state;
    state.Write(104u, 0x9000u);
    state.Write(156u, 0xa004u);
    return state;
}

void TestCategoryAndPanel() {
    auto state = MakeState();
    Runtime runtime;
    runtime.words[0x5008u] = 0x6000u;
    runtime.words[0x6118u] = 1u;
    Check(pal4::PropertyWindowCallbacks::OnCategoryClick(
              state, 0x5000u, runtime) &&
              state.Read(108u) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "read:20488", "read:24856", "insert-member",
                  "set-image", "populate-list"},
          "category click reads button tag and refreshes in observed order");
    runtime.calls.clear();
    Check(pal4::PropertyWindowCallbacks::UpdatePanel(state, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "insert-member", "set-image", "populate-list"},
          "direct panel refresh uses existing category");
}

void TestItemClick() {
    auto state = MakeState();
    Runtime runtime;
    runtime.words[0x501cu] = 1u;
    Check(pal4::PropertyWindowCallbacks::OnItemClick(
              state, 0x5000u, runtime) &&
              runtime.calls ==
                  std::vector<std::string>{"read:20508"},
          "nonzero click marker skips item selection");
    runtime.calls.clear();
    runtime.words[0x501cu] = 0u;
    Check(pal4::PropertyWindowCallbacks::OnItemClick(
              state, 0x5000u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "read:20508", "selection"},
          "zero click marker dispatches payload to selection handler");
}

}  // namespace

int main() {
    TestCategoryAndPanel();
    TestItemClick();
    std::cout << "Property window callback tests passed\n";
}
