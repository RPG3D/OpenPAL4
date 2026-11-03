#include "pal4/picture_preview_window_lifecycle.h"

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

struct Runtime final : pal4::PicturePreviewWindowLifecycleRuntimeStub {
    std::vector<std::string> calls;
    bool fail_layout{};
    bool fail_elements{};

    void LoadLayout(pal4::PicturePreviewWindowState&,
                    std::uint32_t name, std::uint32_t first,
                    std::uint32_t second) override {
        Check(name == 0x9000u && first == 0u && second == 0u,
              "picture preview layout receives name and zero flags");
        calls.emplace_back("layout");
        if (fail_layout) throw std::runtime_error("layout");
    }
    void InitializeComponents(
        pal4::PicturePreviewWindowState& state) override {
        Check(state.Read(0u) == 0x843760u &&
                  state.Read(0x28u) == 0x843748u,
              "picture components see final vtables");
        calls.emplace_back("components");
        if (fail_elements) throw std::runtime_error("components");
    }
    void DestroyDialogBase(
        pal4::PicturePreviewWindowState&) override {
        calls.emplace_back("destroy-base");
    }
};

void CheckConstruction() {
    pal4::PicturePreviewWindowState state{};
    Runtime runtime;
    Check(state.bytes.size() == 0x64u &&
              &pal4::PicturePreviewWindowLifecycle::Construct(
                  state, 0x9000u, runtime) == &state &&
              runtime.calls == std::vector<std::string>{
                  "layout", "components"},
          "0x64-byte picture preview initializes layout then components");
    runtime = Runtime{};
    runtime.fail_layout = true;
    try {
        (void)pal4::PicturePreviewWindowLifecycle::Construct(
            state, 0x9000u, runtime);
        Check(false, "layout failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{"layout"},
              "failed base layout has no derived cleanup");
    }
    runtime = Runtime{};
    runtime.fail_elements = true;
    try {
        (void)pal4::PicturePreviewWindowLifecycle::Construct(
            state, 0x9000u, runtime);
        Check(false, "element failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "layout", "components", "destroy-base"},
              "failed elements destroy initialized dialog base");
    }
}

}  // namespace

int main() {
    CheckConstruction();
    std::cout << "Picture preview window lifecycle tests passed\n";
}
