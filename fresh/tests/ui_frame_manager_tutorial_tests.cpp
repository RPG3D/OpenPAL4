#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct TutorialRuntime final : pal4::IUiFrameManagerTutorialRuntime {
    bool ready = true;
    bool flag = false;
    std::uint32_t frame = 0x7000u;
    std::uint32_t after_register = 0x7000u;
    std::vector<std::string> calls;
    unsigned init_count = 0;
    unsigned find_count = 0;

    std::uint32_t InitializeGlobalData() override {
        calls.emplace_back("init data");
        return 0x6000u + 0x100u * init_count++;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x6008u, "global data active field");
        calls.emplace_back("read active");
        return ready ? 1u : 0u;
    }
    bool GetDialogFlag(std::uint32_t data,
                       std::uint32_t key) override {
        Check(data == 0x6100u && key == 0x12345678u,
              "dialog flag receiver and full key");
        calls.emplace_back("get flag");
        return flag;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            const char* name) override {
        Check(manager == 0x5000u &&
                  std::string(name) == "TutorialWindow",
              "tutorial frame lookup");
        calls.emplace_back("find frame");
        return find_count++ == 0u ? frame : after_register;
    }
    std::uint32_t ManagerSingleton() override {
        calls.emplace_back("manager singleton");
        return 0x8000u;
    }
    void RegisterFrame(std::uint32_t manager,
                       const char* name) override {
        Check(manager == 0x8000u &&
                  std::string(name) == "TutorialWindow",
              "tutorial registration");
        calls.emplace_back("register frame");
    }
    void SetDialogFlagAndSave(std::uint32_t data,
                              std::uint32_t key) override {
        Check(data == 0x6200u && key == 0x12345678u,
              "save flag with refreshed subsystem and full key");
        calls.emplace_back("save flag");
    }
    void UpdateVisibility(std::uint32_t actual_frame,
                          std::uint32_t visible,
                          std::uint32_t key,
                          std::uint32_t zero) override {
        Check(actual_frame == 0x7000u && visible == 1u &&
                  key == 0x12345678u && zero == 0u,
              "tutorial visibility arguments");
        calls.emplace_back("show frame");
    }
};

void CheckTutorialBranches() {
    TutorialRuntime inactive;
    inactive.ready = false;
    pal4::UiFrameManagerSingleton::ShowTutorialDialog(
        0x5000u, 0x12345678u, inactive);
    Check(inactive.calls == (std::vector<std::string>{
              "init data", "read active"}),
          "inactive global data exits before flag lookup");

    TutorialRuntime already_seen;
    already_seen.flag = true;
    pal4::UiFrameManagerSingleton::ShowTutorialDialog(
        0x5000u, 0x12345678u, already_seen);
    Check(already_seen.calls == (std::vector<std::string>{
              "init data", "read active", "init data", "get flag"}),
          "existing flag exits before frame lookup");

    TutorialRuntime existing;
    pal4::UiFrameManagerSingleton::ShowTutorialDialog(
        0x5000u, 0x12345678u, existing);
    Check(existing.calls == (std::vector<std::string>{
              "init data", "read active", "init data", "get flag",
              "find frame", "init data", "save flag", "show frame"}),
          "existing tutorial saves before display without registration");

    TutorialRuntime registered;
    registered.frame = 0u;
    pal4::UiFrameManagerSingleton::ShowTutorialDialog(
        0x5000u, 0x12345678u, registered);
    Check(registered.calls == (std::vector<std::string>{
              "init data", "read active", "init data", "get flag",
              "find frame", "manager singleton", "register frame",
              "find frame", "init data", "save flag", "show frame"}),
          "missing tutorial registers, relooks up and then displays");

    TutorialRuntime registration_failed;
    registration_failed.frame = 0u;
    registration_failed.after_register = 0u;
    pal4::UiFrameManagerSingleton::ShowTutorialDialog(
        0x5000u, 0x12345678u, registration_failed);
    Check(registration_failed.calls.size() == 8u &&
              registration_failed.calls.back() == "find frame",
          "failed registration does not save tutorial flag");
}

struct TimerRuntime final : pal4::IUiFrameManagerTimerFlagRuntime {
    std::uint32_t frame = 0x7000u;
    unsigned reads = 0;
    std::uint32_t FindFrame(std::uint32_t manager,
                            const char* name) override {
        Check(manager == 0x5000u && std::string(name) == "portrait",
              "portrait lookup");
        return frame;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x70f6u, "portrait flag byte offset");
        ++reads;
        return 0xabu;
    }
};

void CheckTimerFlag() {
    TimerRuntime runtime;
    runtime.frame = 0u;
    Check(pal4::UiFrameManagerSingleton::GetPortraitTimerFlag(
              0x5000u, runtime) == 0u && runtime.reads == 0u,
          "missing portrait returns zero without read");
    runtime.frame = 0x7000u;
    Check(pal4::UiFrameManagerSingleton::GetPortraitTimerFlag(
              0x5000u, runtime) == 0xabu && runtime.reads == 1u,
          "present portrait returns full flag byte");
}

}  // namespace

int main() {
    try {
        CheckTutorialBranches();
        CheckTimerFlag();
        std::cout << "ui_frame_manager_tutorial_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
