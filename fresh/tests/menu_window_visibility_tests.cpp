#include "pal4/menu_window_visibility.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IMenuWindowVisibilityRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, bool> disabled;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> cleaned;
    std::vector<std::uint32_t> removed;
    std::vector<std::uint32_t> destroyed;
    pal4::MenuWindowPoint positioned{};
    bool modal = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    void AdjustHeight(std::uint32_t menu) override {
        calls.emplace_back("adjust_height");
        Check(menu == 0x1000u, "height owner");
    }
    void SetParent(std::uint32_t menu,
                   std::uint32_t first,
                   std::uint32_t second) override {
        calls.emplace_back("set_parent");
        Check(menu == 0x1000u && first == 1u && second == 0u,
              "parent arguments");
    }
    void SetAlwaysOnTop(std::uint32_t window,
                        bool on_top) override {
        calls.emplace_back("always_on_top");
        Check(window == 0x7000u && on_top,
              "root topmost state");
    }
    pal4::MenuWindowPoint ScreenToWindow(
        std::uint32_t root,
        pal4::MenuWindowPoint screen) override {
        calls.emplace_back("screen_to_window");
        Check(root == 0x7000u && screen.x == 500.0f &&
              screen.y == 250.0f, "screen coordinate forwarding");
        return {400.0f, 200.0f};
    }
    void SetPosition(std::uint32_t window,
                     pal4::MenuWindowPoint point) override {
        calls.emplace_back("position");
        Check(window == 0x7100u, "panel position owner");
        positioned = point;
    }
    bool IsDisabled(std::uint32_t window,
                    bool inherited) override {
        calls.emplace_back("is_disabled");
        Check(!inherited, "disabled query flag");
        return disabled.at(window);
    }
    void SelectNavigation(std::uint32_t navigation,
                          std::uint32_t virtual_slot) override {
        calls.emplace_back("select_navigation");
        Check(navigation == 0x1000u + 40u &&
              virtual_slot == 12u, "navigation virtual slot");
    }
    void SetModalState(std::uint32_t root,
                       bool value) override {
        calls.emplace_back("modal");
        Check(root == 0x7000u, "modal root");
        modal = value;
    }
    void CleanupButtonRecord(std::uint32_t record) override {
        calls.emplace_back("cleanup_record");
        cleaned.push_back(record);
    }
    void RemoveChild(std::uint32_t parent,
                     std::uint32_t child) override {
        calls.emplace_back("remove_child");
        Check(parent == 0x7100u &&
              child == words.at(words.at(parent + 32u)),
              "first child removal");
        removed.push_back(child);
        const auto first = words.at(parent + 32u);
        words[first] = words[first + 4u];
        words[parent + 36u] -= 4u;
    }
    void DestroyWindow(std::uint32_t window) override {
        calls.emplace_back("destroy_window");
        destroyed.push_back(window);
    }
    void AddChild(std::uint32_t parent,
                  std::uint32_t child) override {
        calls.emplace_back("add_child");
        Check(parent == 0x7100u && child == 0x7200u,
              "model child restored");
    }
};

void CheckShow(bool populated) {
    Runtime runtime;
    runtime.words[0x1000u + 36u] = 0x7000u;
    runtime.words[0x1000u + 80u] = 0x7100u;
    runtime.words[0x1000u + 48u] = populated ? 0x8000u : 0u;
    runtime.words[0x1000u + 52u] = populated ? 0x800cu : 0u;
    if (populated) {
        runtime.words[0x8000u] = 0x9000u;
        runtime.words[0x8004u] = 0x9100u;
        runtime.words[0x8008u] = 0x9200u;
        runtime.disabled[0x9000u] = true;
        runtime.disabled[0x9100u] = false;
        runtime.disabled[0x9200u] = true;
    }
    pal4::MenuWindowVisibility::Show(0x1000u,
                                     {500.0f, 250.0f}, runtime);
    Check(runtime.words.at(0x1000u + 60u) ==
              (populated ? 2u : 0u) &&
          runtime.positioned.x == 400.0f &&
          runtime.positioned.y == 200.0f && runtime.modal,
          "show count, position and modal state");
    Check(runtime.calls[0] == "adjust_height" &&
          runtime.calls[1] == "set_parent" &&
          runtime.calls.back() == "modal" &&
          runtime.calls[runtime.calls.size() - 2u] ==
              "select_navigation", "show call order");
}

void CheckHide(bool populated) {
    Runtime runtime;
    runtime.words[0x1000u + 36u] = 0x7000u;
    runtime.words[0x1000u + 48u] = 0x8000u;
    runtime.words[0x1000u + 52u] = 0x8008u;
    runtime.words[0x1000u + 60u] = 2u;
    runtime.words[0x1000u + 68u] = populated ? 0x9000u : 0u;
    runtime.words[0x1000u + 72u] = populated ? 0x9148u : 0u;
    runtime.words[0x1000u + 80u] = 0x7100u;
    runtime.words[0x1000u + 84u] = 0x7200u;
    runtime.words[0x7100u + 32u] = populated ? 0xa000u : 0u;
    if (populated) {
        runtime.words[0x7100u + 36u] = 0xa008u;
        runtime.words[0xa000u] = 0x7200u;
        runtime.words[0xa004u] = 0x7300u;
    }
    pal4::MenuWindowVisibility::HideAndCleanup(0x1000u,
                                               runtime);
    Check(runtime.words.at(0x1000u + 52u) == 0x8000u &&
          runtime.words.at(0x1000u + 60u) == 0u &&
          runtime.words.at(0x1000u + 72u) ==
              (populated ? 0x9000u : 0u) &&
          !runtime.modal, "hide state reset");
    Check(runtime.cleaned ==
              (populated
                   ? std::vector<std::uint32_t>{0x9000u, 0x90a4u}
                   : std::vector<std::uint32_t>{}) &&
          runtime.removed ==
              (populated
                   ? std::vector<std::uint32_t>{0x7200u, 0x7300u}
                   : std::vector<std::uint32_t>{}) &&
          runtime.destroyed ==
              (populated
                   ? std::vector<std::uint32_t>{0x7300u}
                   : std::vector<std::uint32_t>{}),
          "records and children cleanup");
    Check(runtime.calls[runtime.calls.size() - 3u] == "add_child" &&
          runtime.calls[runtime.calls.size() - 2u] == "read" &&
          runtime.calls.back() == "modal", "model restored before modal off");
}

}  // namespace

int main() {
    try {
        CheckShow(false);
        CheckShow(true);
        CheckHide(false);
        CheckHide(true);
        std::cout << "menu_window_visibility_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
