#include "pal4/movie_preview_window_visibility.h"

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

struct Runtime final : pal4::MoviePreviewWindowVisibilityRuntimeStub {
    std::vector<std::string> calls;
    bool disabled{};

    void SetParent(pal4::MoviePreviewWindowState&,
                   std::uint8_t showing, std::uint32_t parent) override {
        Check(parent == 0x7000u && (showing == 0u || showing == 1u),
              "base parent receives visibility arguments");
        calls.emplace_back("parent");
    }
    void UpdateImages(pal4::MoviePreviewWindowState&) override {
        calls.emplace_back("images");
    }
    bool IsDisabled(std::uint32_t window, bool inherited) override {
        Check(window == 0x8000u && !inherited,
              "first image disabled state tested without inheritance");
        calls.emplace_back("disabled?");
        return disabled;
    }
    void UpdateStaticInfo(pal4::MoviePreviewWindowState&,
                          std::uint32_t window) override {
        Check(window == 0x8000u, "first image selected for static info");
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
    pal4::MoviePreviewWindowState state{};
    state.Write(0x2cu, 0x8000u);
    Runtime runtime;
    Check(pal4::MoviePreviewWindowVisibility::SetVisible(
              state, 0x6000u, 1u, 0x7000u, runtime) == 10u &&
              runtime.calls == std::vector<std::string>{
                  "parent", "images", "disabled?", "info", "manager", "insert"},
          "show refreshes images and selected description before insertion");
    runtime.calls.clear();
    runtime.disabled = true;
    Check(pal4::MoviePreviewWindowVisibility::SetVisible(
              state, 0x6000u, 1u, 0x7000u, runtime) == 10u &&
              runtime.calls == std::vector<std::string>{
                  "parent", "images", "disabled?", "manager", "insert"},
          "disabled first image skips description update");
    runtime.calls.clear();
    Check(pal4::MoviePreviewWindowVisibility::SetVisible(
              state, 0x6000u, 0u, 0x7000u, runtime) == 20u &&
              runtime.calls == std::vector<std::string>{
                  "parent", "manager", "remove"},
          "hide only removes active member");
    runtime.calls.clear();
    Check(pal4::MoviePreviewWindowVisibility::SetVisible(
              state, 0u, 0u, 0x7000u, runtime) == 20u &&
              runtime.calls == std::vector<std::string>{
                  "parent", "manager", "remove"},
          "null object hide forwards zero member");
}

}  // namespace

int main() {
    CheckVisibility();
    std::cout << "Movie preview window visibility tests passed\n";
}
