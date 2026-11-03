#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerInitializeRuntime {
    std::vector<std::string> events;
    std::string fail_at;
    std::uint32_t pk_calls = 0u;

    void MaybeFail(const std::string& step) {
        if (fail_at == step) throw std::runtime_error(step);
    }
    std::uint32_t InitializeLogger() override {
        events.emplace_back("logger");
        MaybeFail("logger");
        return 0x4000u;
    }
    void LogCursorLoading(std::uint32_t logger,
                          const char* file,
                          std::uint32_t line,
                          const char* message) override {
        Check(logger == 0x4000u && line == 110u &&
                  std::string(file) ==
                      "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\PAL4Game\\UiFrame\\uiFrameManager.cpp" &&
                  std::string(message) ==
                      "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiCursorManager load cursor resource...\n\n\n",
              "cursor log evidence");
        events.emplace_back("log");
        MaybeFail("log");
    }
    std::uint32_t PkManager() override {
        events.emplace_back("pk-manager");
        return 0x5000u + pk_calls++;
    }
    void ProcessFile3(std::uint32_t manager) override {
        Check(manager == 0x5000u, "first PK manager");
        events.emplace_back("process-3");
        MaybeFail("process-3");
    }
    void LoadCursor(std::uint32_t manager,
                    const char* name) override {
        Check(manager == 0x1000u,
              "cursor loading owner");
        events.emplace_back(name);
        MaybeFail(name);
    }
    std::uint32_t ProcessFile4(std::uint32_t manager) override {
        Check(manager == 0x5001u,
              "second PK manager");
        events.emplace_back("process-4");
        return 0x1234u;
    }
};

void CheckInitialize() {
    Runtime runtime;
    Check(pal4::UiFrameManagerSingleton::Initialize(
              0x1000u, runtime) == 0x1234u &&
              runtime.pk_calls == 2u &&
              runtime.events ==
                  std::vector<std::string>{
                      "logger", "log", "pk-manager",
                      "process-3", "default.cur", "none.ico",
                      "pickup.ani", "talk.ani", "pk-manager",
                      "process-4"},
          "cursor initialization order and final return");
}

void CheckFailure() {
    Runtime runtime;
    runtime.fail_at = "pickup.ani";
    bool threw = false;
    try {
        (void)pal4::UiFrameManagerSingleton::Initialize(
            0x1000u, runtime);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) == "pickup.ani";
    }
    Check(threw && runtime.pk_calls == 1u &&
              runtime.events.back() == "pickup.ani",
          "cursor load failure stops later PK processing");
}

}  // namespace

int main() {
    try {
        CheckInitialize();
        CheckFailure();
        std::cout << "ui_frame_manager_initialize_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
