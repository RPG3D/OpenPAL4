#include "pal4/pal_console_show_dialog_command.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalConsoleShowDialogRuntime {
    std::vector<std::string> calls;
    std::string text;
    std::string output;
    std::uint32_t buffer_result = 0x3000u;
    bool fail_set = false;

    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x4000u;
    }
    std::uint32_t AddOutputLine(std::uint32_t console,
                                std::string_view format) override {
        Check(console == 0x4000u, "usage console");
        calls.emplace_back("output");
        output = format;
        return 0x1234u;
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::uint32_t CreateScenarioDialog(std::uint32_t manager) override {
        Check(manager == 0x1000u, "scenario manager");
        calls.emplace_back("dialog");
        return 0x2000u;
    }
    std::uint32_t CreateTextBuffer(std::string_view value) override {
        calls.emplace_back("buffer");
        text = value;
        return buffer_result;
    }
    void SetDialogText(std::uint32_t dialog, std::uint32_t buffer,
                       std::uint32_t editbox_value) override {
        Check(dialog == 0x2000u && buffer == buffer_result &&
                  editbox_value == 0x3dcccccdU,
              "scenario set-text arguments");
        calls.emplace_back("text");
        if (fail_set) throw std::runtime_error("set failed");
    }
    std::uint32_t ReleaseTextBuffer(std::uint32_t buffer) override {
        Check(buffer == 0x3000u, "temporary buffer handle");
        calls.emplace_back("release");
        return 0x3001u;
    }
};

void MissingInput() {
    Runtime runtime;
    Check(pal4::PalConsoleShowDialogCommand::Execute(nullptr, runtime) ==
              0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "console", "output"} &&
              runtime.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd",
          "missing input writes original GBK error");
}

void DialogTextLifetime() {
    Runtime runtime;
    Check(pal4::PalConsoleShowDialogCommand::Execute(
              "dialog-info ignored", runtime) == 0x3001u &&
              runtime.text == "dialog-info" &&
              runtime.calls == std::vector<std::string>{
                  "manager", "dialog", "buffer", "text", "release"},
          "first field, dialog creation and text cleanup order");

    Runtime empty;
    Check(pal4::PalConsoleShowDialogCommand::Execute("", empty) ==
              0x3001u && empty.text.empty(),
          "present empty field reaches dialog");
}

void AllocationFailureAndLongInput() {
    Runtime failed;
    failed.buffer_result = 0u;
    Check(pal4::PalConsoleShowDialogCommand::Execute("message", failed) ==
              0u &&
              failed.calls == std::vector<std::string>{
                  "manager", "dialog", "buffer", "text"},
          "failed allocation still sets dialog with null buffer");

    Runtime long_input;
    Check(pal4::PalConsoleShowDialogCommand::Execute(
              std::string(160u, 'x').c_str(), long_input) == 0x3001u &&
              long_input.text == std::string(128u, 'x'),
          "defined 128-byte truncation boundary");
}

void SetFailureReleasesBuffer() {
    Runtime runtime;
    runtime.fail_set = true;
    try {
        (void)pal4::PalConsoleShowDialogCommand::Execute("message", runtime);
        throw std::runtime_error("expected set failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "set failed",
              "original exception propagates");
    }
    Check(runtime.calls == std::vector<std::string>{
              "manager", "dialog", "buffer", "text", "release"},
          "temporary buffer released during unwind");
}

}  // namespace

int main() {
    try {
        MissingInput();
        DialogTextLifetime();
        AllocationFailureAndLongInput();
        SetFailureReleasesBuffer();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
