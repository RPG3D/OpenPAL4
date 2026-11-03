#include "pal4/system_dialog_visibility.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemDialogVisibilityRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5028u, 0x7000u}, {0x5024u, 0x6000u}};
    std::vector<std::string> calls;
    std::string source = "Original";
    std::string utf8 = "UTF-8 text";
    std::string received_utf8;
    bool fail_text = false;
    bool fail_convert = false;

    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        return 0x8000u;
    }
    std::uint32_t TriggerVariable(const std::uint32_t collection,
                                  const std::string_view name) override {
        Check(collection == 0x8034u && name == "dialog_message",
              "trigger-variable collection and name");
        calls.emplace_back("variable");
        return 0x9000u;
    }
    std::string_view TriggerText(const std::uint32_t variable) override {
        Check(variable == 0x9000u, "trigger-variable pointer");
        calls.emplace_back("source");
        return source;
    }
    std::string ConvertToUtf8(const std::string_view text) override {
        Check(text == source, "conversion receives source copy");
        calls.emplace_back("utf8");
        if (fail_convert) throw std::runtime_error("convert");
        return utf8;
    }
    std::uint32_t CreateCeguiString(const std::string_view text) override {
        received_utf8 = text;
        calls.emplace_back("cegui");
        return 0xa000u;
    }
    void SetText(const std::uint32_t window,
                 const std::uint32_t cegui_string) override {
        Check(window == 0x7000u && cegui_string == 0xa000u,
              "text target is dialog +40 window");
        calls.emplace_back("set-text");
        if (fail_text) throw std::runtime_error("set-text");
    }
    void DestroyCeguiString(const std::uint32_t cegui_string) override {
        Check(cegui_string == 0xa000u, "temporary CEGUI string");
        calls.emplace_back("destroy-cegui");
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void SetParent(const std::uint32_t frame,
                   const std::uint8_t visible,
                   const std::uint32_t parent) override {
        Check(frame == 0x5000u && visible <= 2u && parent == 0xb000u,
              "parent and visibility arguments");
        calls.emplace_back("parent");
    }
    void SetAlwaysOnTop(const std::uint32_t window,
                        const bool enabled) override {
        Check(window == 0x6000u && enabled, "root always-on-top");
        calls.emplace_back("top");
    }
    void SetModalState(const std::uint32_t window,
                       const std::uint8_t visible) override {
        Check(window == 0x6000u && visible <= 2u,
              "root modal state");
        calls.emplace_back("modal");
    }
};

void TestShow() {
    Runtime runtime;
    pal4::SystemDialogVisibility::Update(
        0x5000u, 2u, "dialog_message", 0xb000u, runtime);
    Check(runtime.received_utf8 == "UTF-8 text" &&
              runtime.calls == std::vector<std::string>{
                  "database", "variable", "source", "utf8", "cegui",
                  "read", "set-text", "destroy-cegui", "parent",
                  "read", "top", "read", "modal"},
          "show updates text then parent and root modal state");
    runtime = Runtime{};
    runtime.utf8 = std::string("first\0second", 12u);
    pal4::SystemDialogVisibility::Update(
        0x5000u, 1u, "dialog_message", 0xb000u, runtime);
    Check(runtime.received_utf8 == "first",
          "CEGUI encoding stops at first null byte");
}

void TestHide() {
    Runtime runtime;
    pal4::SystemDialogVisibility::Update(
        0x5000u, 0u, "unused", 0xb000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "parent", "read", "top", "read", "modal"},
          "hide skips database and text but updates parent/modal state");
}

void TestFailures() {
    Runtime convert;
    convert.fail_convert = true;
    bool caught = false;
    try {
        pal4::SystemDialogVisibility::Update(
            0x5000u, 1u, "dialog_message", 0xb000u, convert);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && convert.calls.back() == "utf8",
          "conversion failure does not enter CEGUI or parent updates");

    Runtime text;
    text.fail_text = true;
    caught = false;
    try {
        pal4::SystemDialogVisibility::Update(
            0x5000u, 1u, "dialog_message", 0xb000u, text);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && text.calls[text.calls.size() - 2u] == "set-text" &&
              text.calls.back() == "destroy-cegui",
          "text failure releases temporary CEGUI string before propagating");
}

}  // namespace

int main() {
    TestShow();
    TestHide();
    TestFailures();
    return EXIT_SUCCESS;
}
