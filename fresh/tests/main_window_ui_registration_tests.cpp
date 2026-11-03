#include "pal4/main_window_ui_registration.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MainWindowUiRuntimeStub {
    std::uint32_t allocation = 0x4000u;
    bool replace_callback{};
    bool throw_on_insert{};
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::uint32_t released{};

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 8u, "UI callback allocation is eight bytes");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t WindowNameAddress() override {
        calls.emplace_back("window-name");
        return 0x5000u;
    }
    std::uint32_t ProcessEventsAddress() override {
        calls.emplace_back("process-events");
        return 0x6000u;
    }
    std::uint32_t CloseMainAddress() override {
        calls.emplace_back("close-main");
        return 0x6100u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes.emplace_back(address, value);
        calls.emplace_back("write");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    void InsertOrReplace(std::uint32_t tree,
                         std::string_view command,
                         std::uint32_t& callback) override {
        Check(tree == 0x7104u &&
                  (command == "show_logo_ui" ||
                   command == "show_main_ui" ||
                   command == "close_main_ui") &&
                  callback == allocation,
              "command is inserted in UI manager tree +0x104");
        calls.emplace_back(std::string{command});
        if (replace_callback) callback = 0x4100u;
        if (throw_on_insert) throw std::runtime_error("insert");
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flags) override {
        Check(flags == 1u, "callback is released with deleting flag one");
        released = callback;
        calls.emplace_back("delete");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::MainWindowUiRegistration::RegisterLogo(runtime) ==
              pal4::MainWindowUiRegistration::kLogoResultAddress &&
              runtime.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x4004u, 0x5000u},
                      {0x4000u,
                       pal4::MainWindowUiRegistration::kLogoCallbackVtable}}) &&
              runtime.calls ==
                  (std::vector<std::string>{
                      "window-name", "allocate", "write", "write",
                      "manager", "show_logo_ui", "delete"}) &&
              runtime.released == 0x4000u,
          "logo registration stores MainWindow name and callback vtable");
    runtime = Runtime{};
    Check(pal4::MainWindowUiRegistration::RegisterMainThunk(runtime) ==
              pal4::MainWindowUiRegistration::kMainResultAddress &&
              runtime.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x4004u, 0x6000u},
                      {0x4000u,
                       pal4::MainWindowUiRegistration::kMainCallbackVtable}}) &&
              runtime.calls[0] == "process-events" &&
              runtime.calls[5] == "show_main_ui",
          "main thunk registers process-events callback");
    runtime = Runtime{};
    Check(pal4::MainWindowUiRegistration::RegisterCloseThunk(runtime) ==
              pal4::MainWindowUiRegistration::kCloseResultAddress &&
              runtime.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x4004u, 0x6100u},
                      {0x4000u,
                       pal4::MainWindowUiRegistration::kMainCallbackVtable}}) &&
              runtime.calls[0] == "close-main" &&
              runtime.calls[5] == "close_main_ui" &&
              runtime.released == 0x4000u,
          "close thunk registers close_main_ui callback");
    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(pal4::MainWindowUiRegistration::RegisterLogoThunk(runtime) ==
              pal4::MainWindowUiRegistration::kLogoResultAddress &&
              runtime.writes.empty() && runtime.released == 0u &&
              runtime.calls.back() == "show_logo_ui",
          "null allocation still registers a null callback");
    runtime = Runtime{};
    runtime.replace_callback = true;
    (void)pal4::MainWindowUiRegistration::RegisterMain(runtime);
    Check(runtime.released == 0x4100u,
          "tree replacement updates local pointer before release");
    runtime = Runtime{};
    runtime.throw_on_insert = true;
    try {
        (void)pal4::MainWindowUiRegistration::RegisterMain(runtime);
        Check(false, "insertion exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.released == 0x4000u &&
                  runtime.calls.back() == "delete",
              "insertion exception releases local callback");
    }
    std::cout << "MainWindow UI registration tests passed\n";
}
