#include "pal4/menu_window_actions.h"

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

struct Runtime final : pal4::IMenuWindowActionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t processed = 0u;
    float panel_height = 0.0f;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void HideAndCleanup(std::uint32_t menu) override {
        calls.emplace_back("hide");
        Check(menu == 0x1000u, "hide owner");
    }
    std::uint32_t CallVirtualSlot4(std::uint32_t menu,
                                   std::uint32_t first,
                                   std::uint32_t second) override {
        calls.emplace_back("virtual_4");
        Check(menu == 0x1000u && first == 0u && second == 0u,
              "close virtual arguments");
        return 0xabcdu;
    }
    void CopyWindowEventArgs(std::uint32_t event) override {
        calls.emplace_back("copy_args");
        Check(event == 0x3000u, "event copy source");
    }
    void DestroyWindowEventArgs() override {
        calls.emplace_back("destroy_args");
    }
    void RemoveCurrentHandler(std::uint32_t navigation) override {
        calls.emplace_back("remove_handler");
        Check(navigation == 0x1000u + 40u,
              "registered navigation handler");
    }
    void ProcessButtonData(std::uint32_t record) override {
        calls.emplace_back("process_record");
        processed = record;
    }
    float GetYPosition(std::uint32_t window,
                       std::uint32_t mode) override {
        calls.emplace_back("get_y");
        Check(window == 0x7000u && mode == 1u,
              "model Y metric");
        return 0.1f;
    }
    float GetHeight(std::uint32_t window,
                    std::uint32_t mode) override {
        calls.emplace_back("get_height");
        Check(window == 0x7000u && mode == 1u,
              "model height metric");
        return 0.2f;
    }
    void SetHeight(std::uint32_t window,
                   std::uint32_t mode, float height) override {
        calls.emplace_back("set_height");
        Check(window == 0x7100u && mode == 1u,
              "panel height target");
        panel_height = height;
    }
};

void CheckCloseAndSimpleClick() {
    Runtime runtime;
    Check(pal4::MenuWindowActions::Close(0x1000u, runtime) ==
              0xabcdu && runtime.calls ==
                  std::vector<std::string>{"hide", "virtual_4"},
          "close order and return");
    runtime.calls.clear();
    Check(pal4::MenuWindowActions::HandleClick(0x1000u,
                                               runtime) == 1u &&
          runtime.calls ==
              std::vector<std::string>{"hide", "virtual_4"},
          "simple click closes");
}

void CheckClickEvent(bool should_close) {
    Runtime runtime;
    runtime.words[0x3000u + 28u] = should_close ? 0u : 1u;
    Check(pal4::MenuWindowActions::HandleClickEvent(
              0x1000u, 0x3000u, runtime) == 1u,
          "click event return");
    Check(runtime.calls ==
              (should_close
                   ? std::vector<std::string>{
                         "read", "hide", "virtual_4"}
                   : std::vector<std::string>{"read"}),
          "click event gate");
}

void CheckButtonClick(bool records_present,
                      std::uint32_t index, bool valid) {
    Runtime runtime;
    runtime.words[0x3000u + 8u] = 0x8000u;
    runtime.words[0x8000u + 280u] = index;
    runtime.words[0x1000u + 68u] =
        records_present ? 0x9000u : 0u;
    if (records_present)
        runtime.words[0x1000u + 72u] = 0x9148u;
    Check(pal4::MenuWindowActions::HandleButtonClick(
              0x1000u, 0x3000u, runtime) == 1u,
          "button click return");
    const bool should_process = records_present && valid;
    Check(runtime.processed ==
              (should_process ? 0x9000u + index * 164u : 0u) &&
          runtime.calls.front() == "copy_args" &&
          runtime.calls.back() == "destroy_args",
          "record lookup and event lifetime");
    if (should_process) {
        const auto end = runtime.calls.size();
        Check(runtime.calls[end - 5u] == "remove_handler" &&
              runtime.calls[end - 4u] == "hide" &&
              runtime.calls[end - 3u] == "virtual_4" &&
              runtime.calls[end - 2u] == "process_record",
              "remove, close, process order");
    } else {
        for (const auto& call : runtime.calls)
            Check(call != "remove_handler" && call != "hide" &&
                  call != "process_record", "invalid index gate");
    }
}

void CheckAdjustHeight(bool records_present) {
    Runtime runtime;
    runtime.words[0x1000u + 84u] = 0x7000u;
    runtime.words[0x1000u + 80u] = 0x7100u;
    runtime.words[0x1000u + 68u] =
        records_present ? 0x9000u : 0u;
    if (records_present)
        runtime.words[0x1000u + 72u] = 0x9148u;
    pal4::MenuWindowActions::AdjustHeight(0x1000u, runtime);
    Check(runtime.panel_height ==
              (records_present ? 0.6f : 0.2f) &&
          runtime.calls.back() == "set_height",
          "height is twice model Y plus row heights");
}

}  // namespace

int main() {
    try {
        CheckCloseAndSimpleClick();
        CheckClickEvent(false);
        CheckClickEvent(true);
        CheckButtonClick(false, 0u, false);
        CheckButtonClick(true, 2u, false);
        CheckButtonClick(true, 1u, true);
        CheckAdjustHeight(false);
        CheckAdjustHeight(true);
        std::cout << "menu_window_actions_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
