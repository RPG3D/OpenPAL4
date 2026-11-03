#include "pal4/menu_window.h"

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

struct Runtime final : pal4::IMenuWindowRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocations{0x8000u, 0x8100u, 0x8200u};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> destroyed_strings;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> released_subscriptions;
    std::vector<std::string> names;
    std::string prefix = "UI/";
    std::uint32_t registered_close = 0xffffffffu;

    void LoadLayout(std::uint32_t window,
                    std::string_view name) override {
        calls.emplace_back("layout");
        Check(window == 0x1000u && name == "menuWindow",
              "layout name");
    }
    void ConstructNavigation(std::uint32_t navigation) override {
        calls.emplace_back("navigation");
        Check(navigation == 0x1000u + 40u,
              "embedded navigation address");
    }
    std::uint8_t InitialStackByte() override {
        calls.emplace_back("stack_byte");
        return 0x5au;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("write_byte");
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write_word");
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read_word");
        return words.at(address);
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        Check(bytes == 24u && !allocations.empty(),
              "callback allocation size");
        const auto result = allocations.front();
        allocations.erase(allocations.begin());
        return result;
    }
    std::uint32_t ConstructCloseCallback(
        std::uint32_t storage, std::uint32_t function,
        std::uint32_t argument, std::uint32_t owner) override {
        calls.emplace_back("construct_close");
        Check(storage == 0x8000u && function == 0x4b4b30u &&
              argument == 0u && owner == 0x1000u,
              "close callback fields");
        return storage;
    }
    void RegisterCloseEvent(std::string_view name,
                            std::uint32_t callback) override {
        calls.emplace_back("register_close");
        Check(name == "close_popup_menu", "global event name");
        registered_close = callback;
    }
    void ReleaseCloseCallback(std::uint32_t callback) override {
        calls.emplace_back("release_close");
        Check(callback == 0x8000u, "global callback release");
    }
    std::string_view ReadPrefix(std::uint32_t address) override {
        calls.emplace_back("prefix");
        Check(address == 0x1000u + 20u,
              "layout prefix field");
        return prefix;
    }
    std::uint32_t FindWindow(std::string_view name) override {
        calls.emplace_back("find_window");
        names.emplace_back(name);
        return names.size() == 1u ? 0x7001u : 0x7002u;
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        calls.emplace_back("visible");
        Check(window == 0x7002u && !visible,
              "model button visibility");
    }
    void SetEnabled(std::uint32_t window, bool enabled) override {
        calls.emplace_back("enabled");
        Check(window == 0x7002u && !enabled,
              "model button enabled state");
    }
    void Subscribe(std::uint32_t window,
                   std::string_view event_name,
                   std::uint32_t callback) override {
        calls.emplace_back("subscribe");
        Check(window == 0x7000u, "root subscription target");
        if (event_name == "EventMouseClick")
            Check(callback == 0x8100u || callback == 0u,
                  "mouse callback address");
        else if (event_name == "EventKeyDown")
            Check(callback == 0x8200u || callback == 0u,
                  "key callback address");
        else
            Check(false, "unexpected event name");
        names.emplace_back(event_name);
    }
    void ReleaseSubscriptionCallback(
        std::uint32_t callback) override {
        calls.emplace_back("release_subscription");
        released_subscriptions.push_back(callback);
    }
    void DestroyString(std::uint32_t address) override {
        calls.emplace_back("destroy_string");
        destroyed_strings.push_back(address);
    }
    void Free(std::uint32_t pointer) override {
        calls.emplace_back("free");
        freed.push_back(pointer);
    }
    std::uint32_t DestructBase(std::uint32_t window) override {
        calls.emplace_back("destruct_base");
        Check(window == 0x1000u, "base destructor owner");
        return 0xdeadu;
    }
};

void CheckConstruct(bool allocations_succeed) {
    Runtime runtime;
    runtime.words[0x1000u + 36u] = 0x7000u;
    if (!allocations_succeed) runtime.allocations = {0u, 0u, 0u};
    Check(pal4::MenuWindow::Construct(0x1000u, runtime) == 0x1000u,
          "constructor return");
    Check(runtime.words.at(0x1000u) == pal4::MenuWindow::kVtable &&
          runtime.words.at(0x1000u + 40u) ==
              pal4::MenuWindow::kNavigationVtable &&
          runtime.bytes.at(0x1000u + 64u) == 0x5au &&
          runtime.words.at(0x1000u + 68u) == 0u &&
          runtime.words.at(0x1000u + 72u) == 0u &&
          runtime.words.at(0x1000u + 76u) == 0u,
          "constructor state and vtables");
    Check(runtime.registered_close ==
              (allocations_succeed ? 0x8000u : 0u) &&
          runtime.names == std::vector<std::string>{
              "UI/PanelMenu", "UI/BtnModel",
              "EventMouseClick", "EventKeyDown"} &&
          runtime.words.at(0x1000u + 80u) == 0x7001u &&
          runtime.words.at(0x1000u + 84u) == 0x7002u,
          "event and window names");
    Check(runtime.released_subscriptions ==
              (allocations_succeed
                   ? std::vector<std::uint32_t>{0x8100u, 0x8200u}
                   : std::vector<std::uint32_t>{}),
          "subscription callback lifetime");
    if (allocations_succeed) {
        Check(runtime.words.at(0x8100u) ==
                  pal4::MenuWindow::kEventCallbackVtable &&
              runtime.words.at(0x8108u) == 0x4b4c00u &&
              runtime.words.at(0x810cu) == 0u &&
              runtime.words.at(0x8110u) == 0x1000u &&
              runtime.words.at(0x8208u) == 0x484bd0u,
              "event callback record layout");
    }
    Check(runtime.calls[0] == "layout" &&
          runtime.calls[1] == "navigation" &&
          runtime.calls.back() ==
              (allocations_succeed ? "release_subscription" : "subscribe"),
          "constructor call order");
}

void CheckDestruct(bool populated) {
    Runtime runtime;
    const auto begin = populated ? 0x9000u : 0u;
    runtime.words[0x1000u + 68u] = begin;
    runtime.words[0x1000u + 72u] =
        populated ? begin + 328u : 0u;
    runtime.words[0x1000u + 76u] = 0xa000u;
    runtime.words[0x1000u + 48u] = populated ? 0xb000u : 0u;
    runtime.words[0x1000u + 52u] = 1u;
    runtime.words[0x1000u + 56u] = 2u;
    Check(pal4::MenuWindow::Destruct(0x1000u, runtime) == 0xdeadu,
          "destructor return");
    Check(runtime.destroyed_strings ==
              (populated
                   ? std::vector<std::uint32_t>{0x900cu, 0x90b0u}
                   : std::vector<std::uint32_t>{}) &&
          runtime.freed == std::vector<std::uint32_t>{
              begin, populated ? 0xb000u : 0u},
          "navigation-owned storage release");
    Check(runtime.words.at(0x1000u) ==
              pal4::MenuWindow::kVtable &&
          runtime.words.at(0x1000u + 40u) ==
              pal4::MenuWindow::kNavigationVtable &&
          runtime.words.at(0x1000u + 68u) == 0u &&
          runtime.words.at(0x1000u + 72u) == 0u &&
          runtime.words.at(0x1000u + 76u) == 0u &&
          runtime.words.at(0x1000u + 48u) == 0u &&
          runtime.words.at(0x1000u + 52u) == 0u &&
          runtime.words.at(0x1000u + 56u) == 0u &&
          runtime.calls.back() == "destruct_base",
          "destructor resets all owned fields");
}

}  // namespace

int main() {
    try {
        CheckConstruct(true);
        CheckConstruct(false);
        CheckDestruct(true);
        CheckDestruct(false);
        std::cout << "menu_window_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
