#include "pal4/ol_info_lifecycle.h"

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

struct Runtime final : pal4::OlInfoLifecycleRuntimeStub {
    std::vector<std::string> calls;
    bool fail_panel{};

    void LoadLayout(pal4::OlInfoState&, std::uint32_t name,
                    std::uint32_t first,
                    std::uint32_t second) override {
        Check(name == 0x2000u && first == 0u && second == 0u,
              "olInfo layout receives name and two zero flags");
        calls.emplace_back("layout");
    }
    void InitializePanel(pal4::OlInfoState& state) override {
        Check(state.Read(0u) == pal4::OlInfoLifecycle::kObjectVtable &&
                  state.Read(0x28u) ==
                      pal4::OlInfoLifecycle::kMemberVtable,
              "panel sees final object and member vtables");
        calls.emplace_back("panel");
        if (fail_panel) throw std::runtime_error("panel");
    }
    void DestroyDialog(pal4::OlInfoState&) override {
        calls.emplace_back("destroy-dialog");
    }
};

}  // namespace

int main() {
    pal4::OlInfoState state{};
    Runtime runtime;
    Check(&pal4::OlInfoLifecycle::Construct(state, 0x2000u, runtime) ==
                  &state &&
              runtime.calls == std::vector<std::string>{
                  "layout", "panel"},
          "olInfo constructor initializes panel after layout and vtables");
    runtime = Runtime{};
    runtime.fail_panel = true;
    try {
        (void)pal4::OlInfoLifecycle::Construct(state, 0x2000u, runtime);
        Check(false, "panel exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "layout", "panel", "destroy-dialog"},
              "panel failure destroys completed dialog base");
    }
    std::cout << "olInfo lifecycle tests passed\n";
}
