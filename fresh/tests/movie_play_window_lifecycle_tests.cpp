#include "pal4/movie_play_window_lifecycle.h"

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

struct Runtime final : pal4::MoviePlayWindowLifecycleRuntimeStub {
    std::vector<std::string> calls;
    bool fail_path{};
    bool fail_buttons{};

    void LoadLayout(pal4::MoviePlayWindowState&,
                    std::uint32_t name, std::uint32_t first,
                    std::uint32_t second) override {
        Check(name == 0x2000u && first == 0u && second == 0u,
              "layout receives name and two zero flags");
        calls.emplace_back("layout");
    }
    void ConstructVideoPath(pal4::MoviePlayWindowState& state,
                            std::uint32_t offset) override {
        Check(offset == 0x38u &&
                  state.Read(0x28u) ==
                      pal4::MoviePlayWindowLifecycle::kBaseMemberVtable,
              "path member follows base member vtable");
        calls.emplace_back("path");
        if (fail_path) throw std::runtime_error("path");
        state.Write(0x38u, 0x3000u);
    }
    void CleanupVideoPath(pal4::MoviePlayWindowState& state,
                          std::uint32_t offset) override {
        Check(offset == 0x38u && state.Read(0x38u) == 0x3000u,
              "cleanup targets constructed path at +0x38");
        calls.emplace_back("cleanup-path");
    }
    void InitializeButtons(pal4::MoviePlayWindowState& state) override {
        Check(state.Read(0u) ==
                  pal4::MoviePlayWindowLifecycle::kObjectVtable &&
                  state.Read(0x28u) ==
                      pal4::MoviePlayWindowLifecycle::kMemberVtable &&
                  state.Read(0x38u) == 0x3000u,
              "buttons see final object and member vtables");
        calls.emplace_back("buttons");
        if (fail_buttons) throw std::runtime_error("buttons");
    }
    void DestroyDialog(pal4::MoviePlayWindowState&) override {
        calls.emplace_back("destroy-dialog");
    }
};

}  // namespace

int main() {
    pal4::MoviePlayWindowState state{};
    Runtime runtime;
    Check(&pal4::MoviePlayWindowLifecycle::Construct(
              state, 0x2000u, runtime) == &state &&
              runtime.calls == std::vector<std::string>{
                  "layout", "path", "buttons"},
          "constructor loads layout, constructs path and initializes buttons");
    pal4::MoviePlayWindowLifecycle::Destroy(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "layout", "path", "buttons", "cleanup-path",
              "destroy-dialog"},
          "destructor cleans path before dialog base");

    state = {};
    runtime = Runtime{};
    runtime.fail_path = true;
    try {
        (void)pal4::MoviePlayWindowLifecycle::Construct(
            state, 0x2000u, runtime);
        Check(false, "path exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "layout", "path", "destroy-dialog"},
              "path failure destroys only completed dialog base");
    }

    state = {};
    runtime = Runtime{};
    runtime.fail_buttons = true;
    try {
        (void)pal4::MoviePlayWindowLifecycle::Construct(
            state, 0x2000u, runtime);
        Check(false, "button exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "layout", "path", "buttons", "cleanup-path",
                  "destroy-dialog"},
              "button failure unwinds path before dialog base");
    }
    std::cout << "Movie play window lifecycle tests passed\n";
}
