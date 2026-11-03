#include "pal4/role_selector_window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Subscription {
    std::uint32_t window;
    pal4::RoleSelectorEvent event;
    std::uint32_t callback;
};

struct Runtime final : pal4::IRoleSelectorWindowRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::vector<std::string> static_names;
    std::vector<std::string> found_names;
    std::vector<std::uint32_t> vectors;
    std::vector<std::uint32_t> mouse_buttons;
    std::vector<std::uint32_t> released_callbacks;
    std::vector<Subscription> subscriptions;
    std::uint32_t allocation_count = 0u;
    std::uint32_t failed_allocation = 0u;
    std::uint32_t copied_prefixes = 0u;
    std::uint32_t released_prefixes = 0u;
    std::uint32_t static_cleanups = 0u;
    std::uint32_t top_window = 0u;
    bool top_enabled = false;
    std::uint32_t throw_on_find = 0u;
    std::uint32_t find_count = 0u;
    std::uint32_t throw_on_subscribe = 0u;
    std::uint32_t subscribe_count = 0u;
    bool throw_on_navigation = false;

    void LoadLayout(std::uint32_t object,
                    const char* layout) override {
        calls.emplace_back("load");
        Check(object == 0x1000u &&
              std::string(layout) == "RoleSelectorWindow",
              "layout and object");
        words[object + 36u] = 0x5000u;
    }
    void ConstructNavigation(std::uint32_t navigation) override {
        calls.emplace_back("navigation");
        Check(navigation == 0x1028u, "navigation address");
        if (throw_on_navigation)
            throw std::runtime_error("navigation failed");
    }
    void DestructNavigation(std::uint32_t navigation) override {
        calls.emplace_back("destruct-navigation");
        Check(navigation == 0x1028u, "navigation cleanup address");
    }
    void DestructBase(std::uint32_t object) override {
        calls.emplace_back("destruct-base");
        Check(object == 0x1000u, "base cleanup object");
    }
    void ConstructVector2(std::uint32_t address) override {
        calls.emplace_back("vector");
        vectors.push_back(address);
        words[address] = 0u;
        words[address + 4u] = 0u;
    }
    void InitializeCloseNameBuffer() override {
        calls.emplace_back("init-close");
    }
    std::string ReadLayoutPrefix(std::uint32_t object) override {
        calls.emplace_back("layout-prefix");
        Check(object == 0x1000u, "layout prefix object");
        return "RS/";
    }
    std::uint32_t FindWindow(std::uint32_t object,
                             const std::string& name) override {
        calls.emplace_back("find");
        ++find_count;
        if (find_count == throw_on_find)
            throw std::runtime_error("find failed");
        Check(object == 0x1000u, "find owner");
        found_names.push_back(name);
        return name == "RS/BtnClose" ? 0x9000u : 0x9100u;
    }
    std::array<std::uint32_t, 4> CopyPrefixArguments(
        std::uint32_t source) override {
        calls.emplace_back("copy-prefix");
        Check(source == 0x1014u, "prefix copy source");
        ++copied_prefixes;
        return {0u, 0x8000u, 0u, 0u};
    }
    std::array<std::uint32_t, 2> GetPosition(
        std::uint32_t window) override {
        calls.emplace_back("position");
        return {window + 1u, window + 2u};
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        Check(bytes == 24u, "callback size");
        ++allocation_count;
        return allocation_count == failed_allocation
            ? 0u : 0x2000u + allocation_count * 0x100u;
    }
    void Subscribe(std::uint32_t window,
                   pal4::RoleSelectorEvent event,
                   std::uint32_t callback) override {
        calls.emplace_back("subscribe");
        ++subscribe_count;
        if (subscribe_count == throw_on_subscribe)
            throw std::runtime_error("subscribe failed");
        subscriptions.push_back({window, event, callback});
    }
    void ReleaseSubscriptionCallback(
        std::uint32_t callback) override {
        calls.emplace_back("release-callback");
        released_callbacks.push_back(callback);
    }
    void SetupMouseHandlers(std::uint32_t navigation,
                            std::uint32_t button) override {
        calls.emplace_back("mouse");
        Check(navigation == 0x1028u, "mouse navigation");
        mouse_buttons.push_back(button);
    }
    void LayoutButtons(std::uint32_t object) override {
        calls.emplace_back("layout-buttons");
        Check(object == 0x1000u, "layout buttons owner");
    }
    void SetAlwaysOnTop(std::uint32_t window,
                        bool enabled) override {
        calls.emplace_back("top");
        top_window = window;
        top_enabled = enabled;
    }
    void CleanupCloseNameBuffer() override {
        calls.emplace_back("cleanup-close");
    }
    std::string ReadPrefix(std::uint32_t pointer) override {
        calls.emplace_back("read-prefix");
        Check(pointer == 0x8000u, "role prefix pointer");
        return "RS/";
    }
    std::string ConvertToUtf8(
        const std::string& name) override {
        calls.emplace_back("convert");
        return name;
    }
    std::uint32_t GetWindow(const std::string& name) override {
        calls.emplace_back("get-window");
        const auto index = static_cast<std::uint32_t>(static_names.size());
        static_names.push_back(name);
        return 0xa000u + (index / 9u) * 0x1000u +
            (index % 9u) * 0x100u;
    }
    float GetWidth(std::uint32_t window) override {
        calls.emplace_back("width");
        return static_cast<float>(window & 0xfffu);
    }
    std::uint32_t CleanupFormatBuffer() override {
        calls.emplace_back("cleanup-static");
        ++static_cleanups;
        return 0xabcdu;
    }
    std::uint32_t ReleasePrefix(std::uint32_t pointer) override {
        calls.emplace_back("release-prefix");
        Check(pointer == 0x8000u, "released role prefix");
        ++released_prefixes;
        return 0xfeedu;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void WriteFloat(std::uint32_t address,
                    float value) override {
        floats[address] = value;
    }
};

void CheckConstruct(std::uint32_t failed_allocation) {
    Runtime runtime;
    runtime.failed_allocation = failed_allocation;
    Check(pal4::RoleSelectorWindow::Construct(
              0x1000u, 0xdeadbeefu, runtime) == 0x1000u,
          "constructor return");
    Check(runtime.words.at(0x1000u) ==
              pal4::RoleSelectorWindow::kVtable &&
          runtime.words.at(0x1028u) ==
              pal4::RoleSelectorWindow::kNavigationVtable &&
          runtime.words.at(0x1040u) == 0u &&
          runtime.words.at(0x1044u) == 0u,
          "vtable and navigation fields");
    Check(runtime.vectors.size() == 8u,
          "eight vector constructors");
    for (std::uint32_t index = 0u; index < 4u; ++index)
        Check(runtime.vectors[index] == 0x1118u + index * 8u &&
              runtime.vectors[index + 4u] == 0x1138u + index * 8u,
              "panel and button vector layout");
    Check(runtime.found_names == std::vector<std::string>{
              "RS/BtnClose", "RS/PanelClose"} &&
          runtime.words.at(0x115cu) == 0x9000u &&
          runtime.words.at(0x1158u) == 0x9100u,
          "close controls and prefix");

    Check(runtime.static_names.size() == 36u &&
          runtime.copied_prefixes == 4u &&
          runtime.released_prefixes == 4u &&
          runtime.static_cleanups == 4u,
          "four composed role records and prefix ownership");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto child = 0x1048u + index * 52u;
        const auto panel = 0xa000u + index * 0x1000u + 0x700u;
        const auto button = 0xa000u + index * 0x1000u + 0x600u;
        Check(runtime.static_names[index * 9u] ==
                  "RS/StaticHp" + std::to_string(index) &&
              runtime.static_names[index * 9u + 8u] ==
                  "RS/ImgRole" + std::to_string(index) &&
              runtime.words.at(child) == panel &&
              runtime.words.at(child + 28u) == button &&
              runtime.words.at(button + 280u) == index,
              "role subobject names, panel, button and index");
        Check(runtime.words.at(0x1118u + index * 8u) ==
                  panel + 1u &&
              runtime.words.at(0x111cu + index * 8u) ==
                  panel + 2u &&
              runtime.words.at(0x1138u + index * 8u) ==
                  button + 1u &&
              runtime.words.at(0x113cu + index * 8u) ==
                  button + 2u,
              "panel and button positions");
        Check(runtime.mouse_buttons[index] == button,
              "mouse handler target");
    }

    Check(runtime.allocation_count == 6u &&
          runtime.subscriptions.size() == 6u &&
          runtime.subscriptions[0].window == 0x9000u &&
          runtime.subscriptions[0].event ==
              pal4::RoleSelectorEvent::Clicked &&
          runtime.subscriptions[5].window == 0x5000u &&
          runtime.subscriptions[5].event ==
              pal4::RoleSelectorEvent::KeyDown,
          "close, four role clicks and root key subscriptions");
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        const auto callback = runtime.subscriptions[index].callback;
        if (index + 1u == failed_allocation) {
            Check(callback == 0u, "failed callback allocation");
            continue;
        }
        Check(callback == 0x2100u + index * 0x100u &&
              runtime.words.at(callback) ==
                  pal4::RoleSelectorWindow::kCallbackVtable &&
              runtime.words.at(callback + 8u) ==
                  (index == 0u ? 0x4b6890u :
                   index == 5u ? 0x4b6e10u : 0x4b6840u) &&
              runtime.words.at(callback + 12u) == 0u &&
              runtime.words.at(callback + 16u) == 0x1000u,
              "callback record layout and function");
    }
    Check(runtime.released_callbacks.size() ==
              (failed_allocation == 0u ? 6u : 5u) &&
          runtime.top_window == 0x5000u && runtime.top_enabled &&
          runtime.calls.front() == "load" &&
          runtime.calls.back() == "cleanup-close",
          "callback release, topmost root and final cleanup");
}

void CheckException(bool navigation_failure,
                    bool find_failure,
                    bool subscribe_failure) {
    Runtime runtime;
    runtime.throw_on_navigation = navigation_failure;
    runtime.throw_on_find = find_failure ? 1u : 0u;
    runtime.throw_on_subscribe = subscribe_failure ? 1u : 0u;
    bool caught = false;
    try {
        (void)pal4::RoleSelectorWindow::Construct(
            0x1000u, 0u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "destruct-base",
          "construction failure propagates after base cleanup");
    if (navigation_failure) {
        Check(runtime.calls == std::vector<std::string>{
                  "load", "navigation", "destruct-base"},
              "navigation failure cleans base only");
        return;
    }
    Check(runtime.calls[runtime.calls.size() - 3u] ==
              "cleanup-close" &&
          runtime.calls[runtime.calls.size() - 2u] ==
              "destruct-navigation",
          "format, navigation and base cleanup order");
    if (subscribe_failure)
        Check(runtime.released_callbacks ==
                  std::vector<std::uint32_t>{0x2100u} &&
              runtime.subscribe_count == 1u,
              "failed subscription releases callback reference");
    if (find_failure)
        Check(runtime.released_callbacks.empty() &&
              runtime.find_count == 1u,
              "failed lookup has no callback to release");
}

}  // namespace

int main() {
    try {
        CheckConstruct(0u);
        CheckConstruct(1u);
        CheckConstruct(3u);
        CheckConstruct(6u);
        CheckException(true, false, false);
        CheckException(false, true, false);
        CheckException(false, false, true);
        std::cout << "role_selector_window_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
