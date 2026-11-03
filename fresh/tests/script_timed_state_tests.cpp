#include "pal4/script_timed_state.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptTimedStateRuntime {
    std::vector<std::string> calls;
    std::uint32_t callback = 0x8000u;
    std::uint32_t expected_callback = 0x8000u;
    std::uint32_t expected_value = 0x12345678u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x7004u, "callback record word one");
        calls.emplace_back("read-callback");
        return callback;
    }
    std::uint32_t EmptyString() override {
        calls.emplace_back("empty");
        return 0x9000u;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x1000u;
    }
    std::uint32_t InitializeState(
        std::uint32_t executor, std::uint32_t value,
        std::uint32_t text) override {
        Check(executor == 0x1000u && value == expected_value &&
                  text == expected_callback,
              "timed script initialization operands");
        calls.emplace_back("initialize-state");
        return 0xabcdefu;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(value == 0u, "terminated state words are zero");
        if (address == 0x103cu) calls.emplace_back("clear-enabled");
        else if (address == 0x1040u) calls.emplace_back("clear-elapsed");
        else if (address == 0x1044u) calls.emplace_back("clear-value");
        else Check(false, "unexpected timer field");
    }
    std::uint32_t TextLength(std::uint32_t text) override {
        Check(text == 0x9000u, "length of global empty string");
        calls.emplace_back("length");
        return 0u;
    }
    std::uint32_t InitializeText(
        std::uint32_t destination, std::uint32_t text,
        std::uint32_t length) override {
        Check(destination == 0x1048u && text == 0x9000u && length == 0u,
              "reset callback string buffer");
        calls.emplace_back("initialize-text");
        return 0x1050u;
    }
};

void Start() {
    Runtime named;
    Check(pal4::ScriptTimedState::Start(
              named.expected_value, 0x7000u, named) == 0xabcdefu,
          "named callback initialize result");
    Check(named.calls == std::vector<std::string>{
              "read-callback", "executor", "initialize-state"},
          "named callback order");

    Runtime empty;
    empty.callback = 0u;
    empty.expected_callback = 0x9000u;
    empty.expected_value = 0xffffffffu;
    Check(pal4::ScriptTimedState::Start(
              empty.expected_value, 0x7000u, empty) == 0xabcdefu,
          "empty callback initialize result");
    Check(empty.calls == std::vector<std::string>{
              "read-callback", "empty", "executor", "initialize-state"},
          "null callback falls back to global empty string");
}

void Terminate() {
    Runtime runtime;
    Check(pal4::ScriptTimedState::Terminate(runtime) == 0x1050u,
          "terminate forwards string initialization result");
    Check(runtime.calls == std::vector<std::string>{
              "executor", "empty", "clear-enabled", "length",
              "initialize-text", "clear-elapsed", "clear-value"},
          "timer reset field and text order");
}

}  // namespace

int main() {
    try {
        Start();
        Terminate();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
