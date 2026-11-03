#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerMessageRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint8_t process_result = 0x7fu;
    bool remove_system = false;
    bool throw_process = false;
    bool change_result_on_escape = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x5198u, "system field address");
        events.emplace_back("read system");
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x5220u || address == 0x5221u,
              "message flag address");
        events.emplace_back(address == 0x5220u ? "read result" :
                                                    "read flag");
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == 0x5220u, "result field address");
        events.emplace_back(value == 0u ? "clear result" : "set result");
        bytes[address] = value;
    }
    std::uint8_t ProcessUiEvent(
        std::uint32_t system, std::uint32_t hwnd,
        std::uint32_t message, std::uint32_t wparam,
        std::uint32_t lparam) override {
        events.emplace_back("process");
        Check(system == 0x8800u && hwnd == 0x1234u &&
                  wparam == 0x89abcdefu && lparam == 0xfeed1234u &&
                  (message == 0x100u || message == 0x205u),
              "Win32 arguments forwarded without conversion");
        Check(bytes[0x5220u] == 0u, "result cleared before processing");
        if (remove_system) words[0x5198u] = 0u;
        if (throw_process) throw std::runtime_error("process failure");
        return process_result;
    }
    void SimulateKeyPressAndRelease(
        std::uint32_t manager, std::uint32_t key) override {
        events.emplace_back("escape");
        Check(manager == 0x5000u && key == 0x1bu,
              "context action sends Escape to same manager");
        if (change_result_on_escape) bytes[0x5220u] = 0x35u;
    }
};

std::uint8_t Handle(Runtime& runtime, std::uint32_t message) {
    return pal4::UiFrameManagerSingleton::HandleMessage(
        0x5000u, 0x1234u, message, 0x89abcdefu, 0xfeed1234u,
        runtime);
}

void CheckNoSystem() {
    Runtime runtime;
    runtime.bytes[0x5220u] = 0xffu;
    runtime.bytes[0x5221u] = 1u;
    Check(Handle(runtime, 0x100u) == 0u &&
              runtime.events == std::vector<std::string>{
                  "read system", "clear result", "read result"},
          "missing system clears result and skips dispatch");

    runtime.events.clear();
    Check(Handle(runtime, 0x205u) == 0u &&
              runtime.events == std::vector<std::string>{
                  "read system", "clear result", "read system",
                  "read result"},
          "context message with missing system does not read escape flag");
}

void CheckOrdinaryDispatch() {
    Runtime runtime;
    runtime.words[0x5198u] = 0x8800u;
    runtime.bytes[0x5220u] = 0xffu;
    runtime.bytes[0x5221u] = 1u;
    Check(Handle(runtime, 0x100u) == 0x7fu &&
              runtime.events == std::vector<std::string>{
                  "read system", "clear result", "process",
                  "set result", "read result"},
          "ordinary message dispatches and returns CEGUI byte");
}

void CheckContextBranches() {
    Runtime runtime;
    runtime.words[0x5198u] = 0x8800u;
    Check(Handle(runtime, 0x205u) == 0x7fu &&
              runtime.events == std::vector<std::string>{
                  "read system", "clear result", "process",
                  "set result", "read system", "read flag",
                  "read result"},
          "clear context flag skips simulated key");

    runtime.events.clear();
    runtime.bytes[0x5221u] = 1u;
    runtime.change_result_on_escape = true;
    Check(Handle(runtime, 0x205u) == 0x35u &&
              runtime.events == std::vector<std::string>{
                  "read system", "clear result", "process",
                  "set result", "read system", "read flag",
                  "escape", "read result"},
          "Escape follows dispatch and final result is re-read");

    runtime.events.clear();
    runtime.remove_system = true;
    Check(Handle(runtime, 0x205u) == 0x7fu &&
              runtime.events == std::vector<std::string>{
                  "read system", "clear result", "process",
                  "set result", "read system", "read result"},
          "context branch rechecks system after UI processing");
}

void CheckException() {
    Runtime runtime;
    runtime.words[0x5198u] = 0x8800u;
    runtime.bytes[0x5220u] = 0xffu;
    runtime.throw_process = true;
    bool threw = false;
    try {
        (void)Handle(runtime, 0x205u);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) == "process failure";
    }
    Check(threw && runtime.bytes[0x5220u] == 0u &&
              runtime.events == std::vector<std::string>{
                  "read system", "clear result", "process"},
          "processing exception leaves result cleared");
}

}  // namespace

int main() {
    try {
        CheckNoSystem();
        CheckOrdinaryDispatch();
        CheckContextBranches();
        CheckException();
        std::cout << "ui_frame_manager_message_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
