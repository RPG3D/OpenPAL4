#include "pal4/load_window_lifecycle.h"

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

struct Runtime final : pal4::LoadWindowLifecycleRuntimeStub {
    bool fail_load{};
    bool fail_elements{};
    std::vector<std::string> calls;

    void LoadLayout(pal4::LoadWindowState& state,
                    std::uint32_t name,
                    std::uint32_t first,
                    std::uint32_t second) override {
        Check(name == 0x2000u && first == 0u && second == 0u,
              "dialog layout receives name and two zero flags");
        Check(state.Read(0u) == 0u && state.Read(0x28u) == 0u,
              "layout is loaded before derived vtables are written");
        calls.emplace_back("layout");
        if (fail_load) throw std::runtime_error("layout");
    }
    void InitializeListElements(pal4::LoadWindowState& state) override {
        Check(state.Read(0u) ==
                  pal4::LoadWindowLifecycle::kObjectVtable &&
                  state.Read(0x28u) ==
                      pal4::LoadWindowLifecycle::kMemberVtable,
              "list elements see the final object and member vtables");
        calls.emplace_back("elements");
        if (fail_elements) throw std::runtime_error("elements");
    }
    std::uint32_t DestroyDialog(
        pal4::LoadWindowState& state) override {
        Check(state.Read(0x28u) ==
                  pal4::LoadWindowLifecycle::kMemberVtable,
              "dialog cleanup follows derived vtable installation");
        calls.emplace_back("destroy");
        return 0x1234u;
    }
};

}  // namespace

int main() {
    pal4::LoadWindowState state{};
    Runtime runtime;
    Check(&pal4::LoadWindowLifecycle::Construct(
              state, 0x2000u, runtime) == &state &&
              state.Read(0u) ==
                  pal4::LoadWindowLifecycle::kObjectVtable &&
              state.Read(0x28u) ==
                  pal4::LoadWindowLifecycle::kMemberVtable &&
              runtime.calls == std::vector<std::string>{
                  "layout", "elements"},
          "constructor loads layout, installs vtables and initializes list");
    Check(pal4::LoadWindowLifecycle::Destroy(state, runtime) == 0x1234u &&
              runtime.calls.back() == "destroy",
          "destructor thunk forwards dialog-base result");

    state = {};
    runtime = Runtime{};
    runtime.fail_load = true;
    try {
        (void)pal4::LoadWindowLifecycle::Construct(
            state, 0x2000u, runtime);
        Check(false, "layout failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{"layout"},
              "layout failure does not destroy an unconstructed dialog");
    }

    state = {};
    runtime = Runtime{};
    runtime.fail_elements = true;
    try {
        (void)pal4::LoadWindowLifecycle::Construct(
            state, 0x2000u, runtime);
        Check(false, "element initialization failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "layout", "elements", "destroy"},
              "constructed dialog is destroyed during exception unwind");
    }
    std::cout << "Load window lifecycle tests passed\n";
}
