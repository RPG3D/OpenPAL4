#include "pal4/movie_play_window_visibility.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MoviePlayWindowVisibilityRuntimeStub {
    std::vector<std::string> calls;
    std::uint8_t showing{};

    void SetParent(pal4::MoviePlayWindowState&,
                   std::uint8_t show, std::uint32_t parent) override {
        Check(parent == 0x6000u, "parent pointer is forwarded unchanged");
        showing = show;
        calls.emplace_back("parent");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    void InsertActive(std::uint32_t array,
                      std::uint32_t member) override {
        Check(array == 0x8134u && member == 0x2028u,
              "show inserts member +0x28 in manager active array +0x134");
        calls.emplace_back("insert");
    }
    void RemoveActive(std::uint32_t array,
                      std::uint32_t member) override {
        Check(array == 0x8134u && member == 0x2028u,
              "hide removes the same active member");
        calls.emplace_back("remove");
    }
    std::uint32_t MusicPlayer() override {
        calls.emplace_back("music-player");
        return 0x9000u;
    }
    void StopCurrentWithFade(std::uint32_t player,
                             float seconds) override {
        Check(player == 0x9000u && seconds == 1.0f,
              "show fades out current music over one second");
        calls.emplace_back("fade-out");
    }
    void FadeInBackground(std::uint32_t player,
                          std::string_view file, float seconds,
                          std::uint32_t flags) override {
        Check(player == 0x9000u && file == "P01-1.mp3" &&
                  seconds == 3.0f && flags == 0u,
              "hide fades in original background music");
        calls.emplace_back("fade-in");
    }
};

}  // namespace

int main() {
    pal4::MoviePlayWindowState state{};
    Runtime runtime;
    pal4::MoviePlayWindowVisibility::SetVisible(
        state, 0x2000u, 2u, 0x6000u, runtime);
    Check(runtime.showing == 2u &&
              runtime.calls == std::vector<std::string>{
                  "parent", "manager", "insert", "music-player",
                  "fade-out"},
          "nonzero show byte inserts before stopping current music");
    runtime.calls.clear();
    pal4::MoviePlayWindowVisibility::SetVisible(
        state, 0x2000u, 0u, 0x6000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "parent", "manager", "remove", "music-player",
              "fade-in"},
          "hide removes active member before restoring music");
    std::cout << "Movie play window visibility tests passed\n";
}
