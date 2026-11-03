#include "pal4/save_window_lifecycle.h"

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

struct Runtime final : pal4::SaveWindowLifecycleRuntimeStub {
    std::vector<std::string> calls;
    bool fail_components{};

    void LoadLayout(pal4::SaveWindowState& state, std::uint32_t name,
                    std::uint32_t first, std::uint32_t second) override {
        Check(name == 0x7000u && first == 0u && second == 0u,
              "save layout receives name and zero flags");
        calls.emplace_back("load");
        state.Write(0x24u, 0x5000u);
    }
    void InitializeComponents(pal4::SaveWindowState& state) override {
        Check(state.Read(0u) == 0x843798u &&
                  state.Read(0x28u) == 0x843780u,
              "final vtables installed before component lookup");
        calls.emplace_back("components");
        if (fail_components) throw std::runtime_error("components");
    }
    void DestroyDialogBase(pal4::SaveWindowState&) override {
        calls.emplace_back("destroy-base");
    }
};

}  // namespace

int main() {
    pal4::SaveWindowState state{};
    Runtime runtime;
    Check(&pal4::SaveWindowLifecycle::Construct(state, 0x7000u, runtime) ==
              &state && state.Read(0x24u) == 0x5000u &&
              state.Read(0u) == 0x843798u &&
              state.Read(0x28u) == 0x843780u &&
              runtime.calls == std::vector<std::string>{"load", "components"},
          "save constructor installs vtables and components");
    Check(pal4::SaveWindowLifecycle::ReturnTrue(0u) == 1u &&
              pal4::SaveWindowLifecycle::ReturnTrue(0xffffffffu) == 1u,
          "0x44ce10 callback ignores its argument");
    state = {};
    runtime = Runtime{};
    runtime.fail_components = true;
    try {
        (void)pal4::SaveWindowLifecycle::Construct(state, 0x7000u, runtime);
        Check(false, "component failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "load", "components", "destroy-base"},
              "failed components unwind dialog base");
    }
    std::cout << "Save window lifecycle tests passed\n";
}
