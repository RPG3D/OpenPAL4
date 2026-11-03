#include "pal4/movie_preview_window_lifecycle.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MoviePreviewWindowLifecycleRuntimeStub {
    std::vector<std::string> calls;
    bool fail_layout{};
    bool fail_components{};

    void LoadLayout(pal4::MoviePreviewWindowState&,
                    std::uint32_t name, std::uint32_t first,
                    std::uint32_t second) override {
        Check(name == 0x9000u && first == 0u && second == 0u,
              "constructor passes name and zero layout flags");
        calls.emplace_back("layout");
        if (fail_layout) throw std::runtime_error("layout");
    }
    void InitializeComponents(
        pal4::MoviePreviewWindowState& state) override {
        Check(state.Read(0u) == 0x843728u &&
                  state.Read(0x28u) == 0x843710u,
              "components see final object and member vtables");
        calls.emplace_back("components");
        if (fail_components) throw std::runtime_error("components");
    }
    void DestroyDialogBase(
        pal4::MoviePreviewWindowState&) override {
        calls.emplace_back("dialog-base");
    }
    void DestroyWindow(
        pal4::MoviePreviewWindowState&) override {
        calls.emplace_back("destroy-window");
    }
    void FreeMemory(std::uint32_t address) override {
        Check(address == 0x6000u, "deleting destructor frees object address");
        calls.emplace_back("free");
    }
};

void CheckLifecycle() {
    pal4::MoviePreviewWindowState state{};
    Runtime runtime;
    Check(&pal4::MoviePreviewWindowLifecycle::Construct(
              state, 0x9000u, runtime) == &state &&
              runtime.calls == std::vector<std::string>{"layout", "components"},
          "constructor initializes layout before components");
    runtime.calls.clear();
    Check(pal4::MoviePreviewWindowLifecycle::Destroy(
              state, 0x6000u, 0u, runtime) == 0x6000u &&
              runtime.calls == std::vector<std::string>{"destroy-window"},
          "nondeleting destructor does not free storage");
    runtime.calls.clear();
    Check(pal4::MoviePreviewWindowLifecycle::Destroy(
              state, 0x6000u, 3u, runtime) == 0x6000u &&
              runtime.calls == std::vector<std::string>{
                  "destroy-window", "free"},
          "deleting destructor checks flag bit zero");
}

void CheckFailures() {
    pal4::MoviePreviewWindowState state{};
    Runtime runtime;
    runtime.fail_layout = true;
    try {
        (void)pal4::MoviePreviewWindowLifecycle::Construct(
            state, 0x9000u, runtime);
        Check(false, "layout failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{"layout"},
              "failed base layout has no dialog cleanup");
    }
    runtime = Runtime{};
    runtime.fail_components = true;
    try {
        (void)pal4::MoviePreviewWindowLifecycle::Construct(
            state, 0x9000u, runtime);
        Check(false, "component failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "layout", "components", "dialog-base"},
              "component failure destroys initialized dialog base");
    }
}

}  // namespace

int main() {
    CheckLifecycle();
    CheckFailures();
    std::cout << "Movie preview window lifecycle tests passed\n";
}
