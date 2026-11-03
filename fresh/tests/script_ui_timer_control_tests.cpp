#include "pal4/script_ui_timer_control.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptUiTimerControlRuntime {
    std::vector<std::string> calls;
    std::uint32_t frame = 0x5000u;
    std::uint32_t enabled = 1u;
    std::uint32_t manager_count = 0u;

    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x4000u + 0x100u * manager_count++;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x4000u && name == "portrait",
              "portrait frame lookup");
        calls.emplace_back("find");
        return frame;
    }
    void ClearTimerAndText(std::uint32_t value) override {
        Check(value == 0x5000u, "clear timer frame");
        calls.emplace_back("clear");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x50e8u) {
            calls.emplace_back("enabled");
            return enabled;
        }
        Check(address == 0x42a0u, "timer queue field on second manager");
        calls.emplace_back("queue");
        return 0x6000u;
    }
    std::uint32_t RemoveTask(std::uint32_t queue,
                             std::uint32_t identifier) override {
        Check(queue == 0x6000u && identifier == 0x50d0u,
              "pause queue and frame identifier");
        calls.emplace_back("remove");
        return 0x12345678u;
    }
    void CreateTask(std::uint32_t queue,
                    std::uint32_t identifier,
                    std::uint32_t period_bits,
                    std::uint32_t context,
                    std::uint32_t callback,
                    std::uint32_t option) override {
        Check(queue == 0x6000u && identifier == 0x50d0u &&
                  period_bits == 0x3f800000u && context == 0x5000u &&
                  callback == 0x47caa0u && option == 0u,
              "resume task operands");
        calls.emplace_back("create");
    }
};

void Clear() {
    Runtime found;
    pal4::ScriptUiTimerControl::Clear(found);
    Check(found.calls == std::vector<std::string>{
              "manager", "find", "clear"},
          "clear calls frame text reset");
    Runtime missing;
    missing.frame = 0u;
    pal4::ScriptUiTimerControl::Clear(missing);
    Check(missing.calls == std::vector<std::string>{"manager", "find"},
          "clear skips absent frame");
}

void Pause() {
    Runtime found;
    Check(pal4::ScriptUiTimerControl::Pause(found) == 0x12345678u &&
              found.calls == std::vector<std::string>{
                  "manager", "find", "manager", "queue", "remove"},
          "pause return and second manager acquisition");
    Runtime missing;
    missing.frame = 0u;
    Check(pal4::ScriptUiTimerControl::Pause(missing) == 0u &&
              missing.calls == std::vector<std::string>{"manager", "find"},
          "pause returns zero without second manager on miss");
}

void Resume() {
    Runtime found;
    pal4::ScriptUiTimerControl::Resume(found);
    Check(found.calls == std::vector<std::string>{
              "manager", "find", "enabled", "manager", "queue", "create"},
          "resume checks frame state before task creation");
    Runtime disabled;
    disabled.enabled = 0u;
    pal4::ScriptUiTimerControl::Resume(disabled);
    Check(disabled.calls == std::vector<std::string>{
              "manager", "find", "enabled"},
          "disabled frame skips queue access");
    Runtime missing;
    missing.frame = 0u;
    pal4::ScriptUiTimerControl::Resume(missing);
    Check(missing.calls == std::vector<std::string>{"manager", "find"},
          "missing frame skips state read");
}

}  // namespace

int main() {
    try {
        Clear();
        Pause();
        Resume();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
