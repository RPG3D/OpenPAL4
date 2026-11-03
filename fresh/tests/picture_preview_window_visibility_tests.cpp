#include "pal4/picture_preview_window_visibility.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PicturePreviewWindowVisibilityRuntimeStub {
    std::vector<std::string> calls;
    bool disabled{};

    void SetParent(pal4::PicturePreviewWindowState&,
                   std::uint8_t showing, std::uint32_t parent) override {
        Check((showing == 0u || showing == 1u) && parent == 0x7000u,
              "picture preview forwards parent arguments");
        calls.emplace_back("parent");
    }
    void UpdateItemList(pal4::PicturePreviewWindowState&) override {
        calls.emplace_back("items");
    }
    bool IsDisabled(std::uint32_t window, bool inherited) override {
        Check(window == 0x8000u && !inherited,
              "first picture image queried without inheritance");
        calls.emplace_back("disabled?");
        return disabled;
    }
    void UpdateStaticInfo(pal4::PicturePreviewWindowState&,
                          std::uint32_t window) override {
        Check(window == 0x8000u, "first picture image selected");
        calls.emplace_back("info");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x9000u;
    }
    std::uint32_t InsertActive(std::uint32_t array,
                               std::uint32_t member) override {
        Check(array == 0x9134u && member == 0x6028u,
              "show inserts member into active array");
        calls.emplace_back("insert");
        return 10u;
    }
    std::uint32_t RemoveActive(std::uint32_t array,
                               std::uint32_t member) override {
        Check(array == 0x9134u &&
                  (member == 0x6028u || member == 0u),
              "hide removes member or null object");
        calls.emplace_back("remove");
        return 20u;
    }
};

void CheckVisibility() {
    pal4::PicturePreviewWindowState state{};
    state.Write(0x2cu, 0x8000u);
    Runtime runtime;
    Check(pal4::PicturePreviewWindowVisibility::SetVisible(
              state, 0x6000u, 1u, 0x7000u, runtime) == 10u &&
              runtime.calls == std::vector<std::string>{
                  "parent", "items", "disabled?", "info", "manager", "insert"},
          "show refreshes items and enabled first image before insertion");
    runtime.calls.clear();
    runtime.disabled = true;
    Check(pal4::PicturePreviewWindowVisibility::SetVisible(
              state, 0x6000u, 1u, 0x7000u, runtime) == 10u &&
              runtime.calls == std::vector<std::string>{
                  "parent", "items", "disabled?", "manager", "insert"},
          "disabled first image skips info update");
    runtime.calls.clear();
    Check(pal4::PicturePreviewWindowVisibility::SetVisible(
              state, 0x6000u, 0u, 0x7000u, runtime) == 20u &&
              runtime.calls == std::vector<std::string>{
                  "parent", "manager", "remove"},
          "hide only removes active member");
    runtime.calls.clear();
    Check(pal4::PicturePreviewWindowVisibility::SetVisible(
              state, 0u, 0u, 0x7000u, runtime) == 20u &&
              runtime.calls == std::vector<std::string>{
                  "parent", "manager", "remove"},
          "null object hide forwards zero member");
}

}  // namespace

int main() {
    CheckVisibility();
    std::cout << "Picture preview window visibility tests passed\n";
}
