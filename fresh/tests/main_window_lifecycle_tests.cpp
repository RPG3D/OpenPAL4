#include "pal4/main_window_lifecycle.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MainWindowLifecycleRuntimeStub {
    bool fail_member{};
    bool fail_buttons{};
    std::vector<std::string> calls;

    void LoadLayout(pal4::MainWindowState&, std::uint32_t name,
                    std::uint32_t first, std::uint32_t second) override {
        Check(name == 0x2000u && first == 0u && second == 0u,
              "MainWindow layout receives name and two zero flags");
        calls.emplace_back("layout");
    }
    void ConstructMember(pal4::MainWindowState&,
                         std::uint32_t offset) override {
        Check(offset == 0x28u, "MainWindow member begins at +0x28");
        calls.emplace_back("member");
        if (fail_member) throw std::runtime_error("member");
    }
    void CleanupMember(pal4::MainWindowState&,
                       std::uint32_t offset) override {
        Check(offset == 0x28u, "member unwind targets +0x28");
        calls.emplace_back("cleanup-member");
    }
    void DestroyDialog(pal4::MainWindowState&) override {
        calls.emplace_back("destroy-dialog");
    }
    void InitializeButtons(pal4::MainWindowState& state) override {
        calls.emplace_back("buttons");
        Check(state.Read(0u) == pal4::MainWindowLifecycle::kObjectVtable &&
                  state.Read(0x28u) ==
                      pal4::MainWindowLifecycle::kMemberVtable &&
                  state.bytes[0x68u] == 1u,
              "buttons see final vtables and flag");
        for (std::uint32_t offset = 0x40u;
             offset <= 0x60u; offset += 4u)
            Check(state.Read(offset) == 0u,
                  "all nine control pointers are cleared");
        if (fail_buttons) throw std::runtime_error("buttons");
    }
    void DestroyTimerFrame(pal4::MainWindowState&) override {
        calls.emplace_back("timer-frame");
    }
    void FreeObject(std::uint32_t address) override {
        Check(address == 0x6000u, "deleting destructor frees object address");
        calls.emplace_back("free");
    }
};

}  // namespace

int main() {
    pal4::MainWindowState state{};
    Runtime runtime;
    Check(&pal4::MainWindowLifecycle::Construct(state, 0x2000u, runtime) ==
              &state &&
              runtime.calls == (std::vector<std::string>{
                  "layout", "member", "buttons"}),
          "constructor initializes member and buttons after layout");
    Check(pal4::MainWindowLifecycle::Destroy(
              state, 0x6000u, 2u, runtime) == 0x6000u &&
              runtime.calls.back() == "timer-frame",
          "nondeleting destructor runs timer-frame cleanup only");
    Check(pal4::MainWindowLifecycle::Destroy(
              state, 0x6000u, 3u, runtime) == 0x6000u &&
              runtime.calls[runtime.calls.size() - 2u] == "timer-frame" &&
              runtime.calls.back() == "free",
          "deleting destructor frees after timer-frame cleanup");

    runtime = Runtime{};
    runtime.fail_member = true;
    try {
        (void)pal4::MainWindowLifecycle::Construct(state, 0x2000u, runtime);
        Check(false, "member constructor failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == (std::vector<std::string>{
                  "layout", "member", "destroy-dialog"}),
              "member failure destroys only completed dialog base");
    }
    runtime = Runtime{};
    runtime.fail_buttons = true;
    try {
        (void)pal4::MainWindowLifecycle::Construct(state, 0x2000u, runtime);
        Check(false, "button initializer failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == (std::vector<std::string>{
                  "layout", "member", "buttons",
                  "cleanup-member", "destroy-dialog"}),
              "button failure unwinds member then dialog base");
    }
    std::cout << "MainWindow lifecycle tests passed\n";
}
