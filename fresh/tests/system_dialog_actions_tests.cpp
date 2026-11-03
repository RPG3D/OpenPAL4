#include "pal4/system_dialog_actions.h"
#include "pal4/system_dialog_visibility.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemDialogActionsRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x9000u + 16u, 0u}, {0x5000u + 40u, 0x6000u},
        {0x6000u + 1256u, 0x7000u}, {0x8000u, 0x8100u},
        {0x8100u + 24u, 0x8200u}};
    std::vector<std::string> calls;
    std::uint32_t hidden_frame = 0u;
    std::uint32_t scrolled = 0u;
    float scroll_position = 0.0f;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    float ReadFloat(const std::uint32_t address) override {
        Check(address == 0x7000u + 1216u,
              "scrollbar position field");
        calls.emplace_back("float");
        return 23.5f;
    }
    void UpdateVisibility(const std::uint32_t frame,
                          const std::uint8_t visible,
                          const std::uint32_t variable,
                          const std::uint32_t parent) override {
        Check(frame == 0x5000u && visible == 0u &&
                  variable == 0u && parent == 0u,
              "close visibility arguments");
        calls.emplace_back("hide");
        hidden_frame = frame;
    }
    std::uint32_t CallVirtual(const std::uint32_t object,
                              const std::uint32_t function) override {
        Check(object == 0x8000u && function == 0x8200u,
              "virtual slot six target");
        calls.emplace_back("virtual");
        return 0x1234u;
    }
    void SetScrollPosition(const std::uint32_t scrollbar,
                           const float position) override {
        Check(scrollbar == 0x7000u,
              "scrollbar target");
        calls.emplace_back("scroll");
        scrolled = scrollbar;
        scroll_position = position;
    }
};

void TestOkAndVirtual() {
    Runtime runtime;
    Check(pal4::SystemDialogActions::OnOkClick(
              0x5000u, 0xdeadu, runtime) == 1u &&
              runtime.hidden_frame == 0x5000u &&
              runtime.calls == std::vector<std::string>{"hide"},
          "OK ignores event and hides dialog");
    runtime.calls.clear();
    Check(pal4::SystemDialogActions::VirtualCall(
              0x8000u, runtime) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "read:32768", "read:33048", "virtual"},
          "virtual call reads vtable slot +24 and returns its result");
}

void TestKeys() {
    for (const auto key : {1u, 28u}) {
        Runtime runtime;
        runtime.words[0x9010u] = key;
        Check(pal4::SystemDialogActions::HandleKeyInput(
                  0x5000u, 0x9000u, runtime) == 1u &&
                  runtime.calls == std::vector<std::string>{
                      "read:36880", "hide"},
              "escape/enter closes dialog");
    }
    for (const auto [key, expected] : {
             std::pair{200u, 13.5f}, {208u, 33.5f}}) {
        Runtime runtime;
        runtime.words[0x9010u] = key;
        Check(pal4::SystemDialogActions::HandleKeyInput(
                  0x5000u, 0x9000u, runtime) == 1u &&
                  runtime.scrolled == 0x7000u &&
                  runtime.scroll_position == expected &&
                  runtime.calls == std::vector<std::string>{
                      "read:36880", "read:20520", "read:25832",
                      "float", "scroll"},
              "up/down adjust scrollbar position by ten");
    }
    for (const auto key : {0u, 199u, 201u, 207u, 209u}) {
        Runtime runtime;
        runtime.words[0x9010u] = key;
        Check(pal4::SystemDialogActions::HandleKeyInput(
                  0x5000u, 0x9000u, runtime) == 0u &&
                  runtime.calls == std::vector<std::string>{"read:36880"},
              "unsupported key has no dialog or scroll action");
    }
}

struct Visibility final : pal4::ISystemDialogVisibilityRuntime {
    std::vector<std::string> calls;

    std::uint32_t GameDatabase() override {
        Check(false, "hide must skip database");
        return 0u;
    }
    std::uint32_t TriggerVariable(std::uint32_t,
                                  std::string_view) override {
        Check(false, "hide must skip variable lookup");
        return 0u;
    }
    std::string_view TriggerText(std::uint32_t) override { return {}; }
    std::string ConvertToUtf8(std::string_view) override { return {}; }
    std::uint32_t CreateCeguiString(std::string_view) override { return 0u; }
    void SetText(std::uint32_t, std::uint32_t) override {}
    void DestroyCeguiString(std::uint32_t) override {}
    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(address == 0x5024u, "composed root lookup");
        calls.emplace_back("read-root");
        return 0x6000u;
    }
    void SetParent(const std::uint32_t frame,
                   const std::uint8_t visible,
                   const std::uint32_t parent) override {
        Check(frame == 0x5000u && visible == 0u && parent == 0u,
              "composed hide arguments");
        calls.emplace_back("parent");
    }
    void SetAlwaysOnTop(const std::uint32_t window,
                        const bool enabled) override {
        Check(window == 0x6000u && enabled,
              "composed root top state");
        calls.emplace_back("top");
    }
    void SetModalState(const std::uint32_t window,
                       const std::uint8_t visible) override {
        Check(window == 0x6000u && visible == 0u,
              "composed root modal state");
        calls.emplace_back("modal");
    }
};

void TestComposedHide() {
    Visibility visibility;
    Check(pal4::SystemDialogActions::OnOkClick(
              0x5000u, 0xdeadu, visibility) == 1u &&
              visibility.calls == std::vector<std::string>{
                  "parent", "read-root", "top", "read-root", "modal"},
          "OK composes reconstructed visibility update");
    Runtime runtime;
    visibility.calls.clear();
    runtime.words[0x9010u] = 28u;
    Check(pal4::SystemDialogActions::HandleKeyInput(
              0x5000u, 0x9000u, runtime, visibility) == 1u &&
              visibility.calls.size() == 5u &&
              runtime.calls == std::vector<std::string>{"read:36880"},
          "escape key composes reconstructed visibility update");
    visibility.calls.clear();
    runtime = Runtime{};
    runtime.words[0x9010u] = 200u;
    Check(pal4::SystemDialogActions::HandleKeyInput(
              0x5000u, 0x9000u, runtime, visibility) == 1u &&
              visibility.calls.empty() && runtime.scroll_position == 13.5f,
          "scroll key leaves visibility unchanged in composed path");
}

}  // namespace

int main() {
    TestOkAndVirtual();
    TestKeys();
    TestComposedHide();
    return EXIT_SUCCESS;
}
