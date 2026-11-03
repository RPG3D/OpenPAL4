#include "pal4/smith_window.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowSelectionPanelRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::uint8_t shown = 0u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0x90u, 0x3000u},
        {object + 0x94u, 0x3004u},
        {object + 0x98u, 0x3008u},
        {object + 0x7cu, 0x3010u},
        {object + 0x44u, 0x3014u},
    };
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::uint32_t mouse_window = 0u;
    std::uint32_t inserted_array = 0u;
    std::uint32_t inserted_record = 0u;
    std::uint8_t insert_result = 0x5au;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == object + 0x81u, "shown flag address");
        calls.emplace_back("read-byte");
        return shown;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == object + 0x81u && value <= 1u,
              "shown flag write");
        shown = value;
        calls.emplace_back("write-byte");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-word");
        return words.at(address);
    }
    void SetVisible(std::uint32_t window,
                    bool visible) override {
        visibility.emplace_back(window, visible);
        calls.emplace_back("visible");
    }
    void SetAlwaysOnTop(std::uint32_t window,
                        bool enabled) override {
        Check(window == 0x3014u, "topmost control");
        calls.emplace_back(enabled ? "topmost-on" : "topmost");
    }
    void HandleMouseEvent(std::uint32_t window) override {
        mouse_window = window;
        calls.emplace_back("mouse");
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    std::uint8_t InsertFrameRecord(std::uint32_t array,
                                   std::uint32_t record) override {
        inserted_array = array;
        inserted_record = record;
        calls.emplace_back("insert");
        return insert_result;
    }
};

void CheckFirstShow() {
    Runtime runtime;
    Check(pal4::SmithWindow::ShowSelectionPanel(
              Runtime::object, runtime) == 0x5au &&
              runtime.shown == 1u,
          "first show forwards insertion result and sets flag");
    Check(runtime.visibility == (std::vector<
              std::pair<std::uint32_t, bool>>{
                  {0x3000u, true}, {0x3004u, true},
                  {0x3008u, true}, {0x3010u, true}}) &&
              runtime.mouse_window == 0x3010u &&
              runtime.inserted_array == 0x5134u &&
              runtime.inserted_record == Runtime::object + 0xf0u,
          "controls, mouse registration and frame record");
    Check(runtime.calls.front() == "read-byte" &&
              runtime.calls.at(8) == "write-byte" &&
              runtime.calls.at(runtime.calls.size() - 3u) == "mouse" &&
              runtime.calls.back() == "insert",
          "show order");

    const auto calls_before = runtime.calls.size();
    Check(pal4::SmithWindow::ShowSelectionPanel(
              Runtime::object, runtime) == 1u &&
              runtime.calls.size() == calls_before + 1u,
          "already shown returns flag without side effects");
}

void CheckNonBooleanExistingFlag() {
    Runtime runtime;
    runtime.shown = 0x80u;
    Check(pal4::SmithWindow::ShowSelectionPanel(
              Runtime::object, runtime) == 0x80u &&
              runtime.visibility.empty(),
          "existing byte is returned unchanged");
}

void CheckHideAndRepeat() {
    Runtime runtime;
    runtime.shown = 0x80u;
    pal4::SmithWindow::HideSelectionPanel(Runtime::object, runtime);
    Check(runtime.shown == 0u &&
              runtime.visibility == (std::vector<
                  std::pair<std::uint32_t, bool>>{
                      {0x3000u, false}, {0x3004u, false},
                      {0x3008u, false}, {0x3010u, false}}) &&
              runtime.calls.at(runtime.calls.size() - 2u) ==
                  "topmost-on" &&
              runtime.calls.back() == "write-byte",
          "hide clears all controls then restores topmost and flag");
    const auto calls_before = runtime.calls.size();
    pal4::SmithWindow::HideSelectionPanel(Runtime::object, runtime);
    Check(runtime.calls.size() == calls_before + 1u,
          "already hidden has no side effects");
}

}  // namespace

int main() {
    try {
        CheckFirstShow();
        CheckNonBooleanExistingFlag();
        CheckHideAndRepeat();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
