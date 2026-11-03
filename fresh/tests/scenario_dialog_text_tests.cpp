#include "pal4/scenario_dialog.h"

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

struct Runtime final : pal4::IScenarioTextRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::string input;
    std::string converted;
    std::string displayed;
    bool fail_set_text = false;

    Runtime() {
        words[0x1038u] = 0x2000u;
        words[0x3004u] = 0x4000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x2878u && value == 0x55u,
              "editbox field at +0x878");
        calls.emplace_back("write");
        words[address] = value;
    }
    std::string ReadCString(std::uint32_t address) override {
        Check(address == 0x4000u, "source data pointer");
        calls.emplace_back("cstring");
        return input;
    }
    std::string ConvertToUtf8(
        const std::string& text) override {
        calls.emplace_back("convert");
        converted = text;
        return text + "!";
    }
    void SetUtf8Text(std::uint32_t window,
                     const std::string& text) override {
        Check(window == 0x2000u, "text window");
        calls.emplace_back("set-text");
        displayed = text;
        if (fail_set_text) throw std::runtime_error("set-text");
    }
    void Show(std::uint32_t object, std::uint32_t visible,
              std::uint32_t parent) override {
        Check(object == 0x1000u && visible == 1u && parent == 0u,
              "show virtual arguments");
        calls.emplace_back("show");
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    std::uint32_t InsertHandler(
        std::uint32_t array, std::uint32_t handler) override {
        Check(array == 0x5134u && handler == 0x1028u,
              "manager array and embedded handler");
        calls.emplace_back("insert");
        return 0x9090u;
    }
};

void CheckText(bool empty) {
    Runtime runtime;
    runtime.input = "A\xc3\xa9";
    if (empty) runtime.words[0x3004u] = 0u;
    Check(pal4::ScenarioDialog::SetText(
              0x1000u, 0x3000u, 0x55u, runtime) ==
              0x9090u, "handler insert return");
    Check(runtime.words.at(0x2878u) == 0x55u &&
              runtime.converted == (empty ? "" : "A\xc3\xa9") &&
              runtime.displayed ==
                  (empty ? "!" : "A\xc3\xa9!"),
          "editbox value and converted text");
    Check(runtime.calls == (empty
              ? std::vector<std::string>{
                    "read", "write", "read", "convert",
                    "set-text", "show", "manager", "insert"}
              : std::vector<std::string>{
                    "read", "write", "read", "cstring",
                    "convert", "set-text", "show", "manager",
                    "insert"}),
          "text, show and registration order");
}

void CheckSetTextFailure() {
    Runtime runtime;
    runtime.fail_set_text = true;
    bool threw = false;
    try {
        (void)pal4::ScenarioDialog::SetText(
            0x1000u, 0x3000u, 0x55u, runtime);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) == "set-text";
    }
    Check(threw && runtime.calls.back() == "set-text",
          "failed text assignment does not show or register");
}

}  // namespace

int main() {
    try {
        CheckText(false);
        CheckText(true);
        CheckSetTextFailure();
        std::cout << "scenario_dialog_text_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
