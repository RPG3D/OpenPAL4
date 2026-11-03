#include "pal4/picture_view_window_visibility.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PictureViewWindowVisibilityRuntimeStub {
    std::vector<std::string> calls;
    std::uint8_t showing{};

    void SetParent(pal4::PictureViewWindowState& state,
                   std::uint8_t show,
                   std::uint32_t parent) override {
        Check(parent == 0x6000u && state.Read(0x3cu) == 0u,
              "dialog parent is set before member flag changes");
        showing = show;
        calls.emplace_back("parent");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t InsertActive(std::uint32_t array,
                               std::uint32_t member) override {
        Check(array == 0x8134u && member == 0x2028u,
              "show inserts member +0x28 into active array +0x134");
        calls.emplace_back("insert");
        return 0x1111u;
    }
    std::uint32_t RemoveActive(std::uint32_t array,
                               std::uint32_t member) override {
        Check(array == 0x8134u && member == 0x2028u,
              "hide removes same member from active array");
        calls.emplace_back("remove");
        return 0x2222u;
    }
};

}  // namespace

int main() {
    pal4::PictureViewWindowState state{};
    Runtime runtime;
    Check(pal4::PictureViewWindowVisibility::SetVisible(
              state, 0x2000u, 2u, 0x6000u, runtime) == 0x1111u &&
              runtime.showing == 2u && state.Read(0x3cu) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "parent", "manager", "insert"},
          "show sets member flag before registering active window");
    state.Write(0x3cu, 0u);
    runtime.calls.clear();
    Check(pal4::PictureViewWindowVisibility::SetVisible(
              state, 0x2000u, 0u, 0x6000u, runtime) == 0x2222u &&
              state.Read(0x3cu) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "parent", "manager", "remove"},
          "hide also sets member flag then unregisters active window");
    std::cout << "Picture view window visibility tests passed\n";
}
