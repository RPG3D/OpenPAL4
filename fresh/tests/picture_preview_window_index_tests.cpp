#include "pal4/picture_preview_window_selection.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PicturePreviewWindowSelectionRuntimeStub {
    std::vector<std::uint32_t> selected;
    void UpdateStaticInfo(pal4::PicturePreviewWindowState&,
                          std::uint32_t window) override {
        selected.push_back(window);
    }
};

}  // namespace

int main() {
    pal4::PicturePreviewWindowState state{};
    for (std::uint32_t i = 0u; i < 10u; ++i)
        state.Write(0x2cu + i * 4u, 0x2100u + i * 0x100u);
    state.Write(0x54u, 0x6000u);
    Runtime runtime;
    pal4::PicturePreviewWindowSelection::UpdateStaticInfoByIndex(
        state, -1, runtime);
    pal4::PicturePreviewWindowSelection::UpdateStaticInfoByIndex(
        state, 11, runtime);
    Check(runtime.selected.empty(), "negative and above-ten indices ignored");
    pal4::PicturePreviewWindowSelection::UpdateStaticInfoByIndex(
        state, 0, runtime);
    pal4::PicturePreviewWindowSelection::UpdateStaticInfoByIndex(
        state, 9, runtime);
    pal4::PicturePreviewWindowSelection::UpdateStaticInfoByIndex(
        state, 10, runtime);
    Check(runtime.selected ==
              std::vector<std::uint32_t>{0x2100u, 0x2a00u, 0x6000u},
          "index ten intentionally reaches StaticInfo slot at +0x54");
    std::cout << "Picture preview index tests passed\n";
}
