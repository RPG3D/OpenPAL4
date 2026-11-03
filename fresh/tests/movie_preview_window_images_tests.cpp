#include "pal4/movie_preview_window_images.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MoviePreviewWindowImagesRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t unlock_mask{(1u << 2u) | (1u << 3u)};
    bool click_task{true};
    std::uint8_t dispatch_result{7u};

    std::uint32_t UnlockMask() override {
        calls.emplace_back("unlock-mask");
        return unlock_mask;
    }
    bool HasTriggerTask(std::uint32_t index) override {
        calls.emplace_back("has:" + std::to_string(index));
        return index >= 1u && index <= 3u ||
               index == 102u && click_task;
    }
    std::uint32_t TriggerTaskAddress(std::uint32_t index) override {
        calls.emplace_back("task:" + std::to_string(index));
        return 0x9000u + index * 0x10u;
    }
    std::uint32_t TriggerTaskId(std::uint32_t task) override {
        calls.emplace_back("id");
        return 100u + (task - 0x9000u) / 0x10u;
    }
    std::string TriggerTaskImageName(std::uint32_t task) override {
        calls.emplace_back("image-name");
        return "raw" + std::to_string((task - 0x9000u) / 0x10u);
    }
    std::string ConvertToUtf8(std::string_view raw) override {
        calls.emplace_back("utf8");
        return std::string(raw) + std::string("\0ignored", 8u);
    }
    void SetEnabled(std::uint32_t window, bool enabled) override {
        calls.emplace_back("enabled:" + std::to_string(window) +
                           (enabled ? "+" : "-"));
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(visible, "unlocked image made visible");
        calls.emplace_back("visible:" + std::to_string(window));
    }
    void SetImage(std::uint32_t window, std::string_view imageset,
                  std::string_view image) override {
        Check(imageset == "biaoti30" &&
                  (image == "raw2" || image == "raw3"),
              "unlocked movie image uses title Imageset and UTF-8 name");
        calls.emplace_back("set-image:" + std::to_string(window));
    }
    void SetImagePointer(std::uint32_t window,
                         std::uint32_t image) override {
        Check(image == 0xd000u, "locked image uses saved disabled pointer");
        calls.emplace_back("disabled-image:" + std::to_string(window));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    void Hide(pal4::MoviePreviewWindowState&, std::uint32_t first,
              std::uint32_t second) override {
        Check(first == 0u && second == 0u,
              "movie preview hidden before playback");
        calls.emplace_back("hide");
    }
    std::uint8_t Dispatch(std::string_view command,
                          std::uint32_t first,
                          std::uint32_t second) override {
        Check(command == "start_play_movie" && first == 1u &&
                  second == 0x9660u,
              "play command carries selected GameDB task address");
        calls.emplace_back("dispatch");
        return dispatch_result;
    }
};

pal4::MoviePreviewWindowState State() {
    pal4::MoviePreviewWindowState state{};
    for (std::uint32_t i = 0u; i < 13u; ++i)
        state.Write(0x2cu + i * 4u, 0x2100u + i * 0x100u);
    state.Write(0x6cu, 0xd000u);
    return state;
}

void CheckImageRefresh() {
    auto state = State();
    Runtime runtime;
    pal4::MoviePreviewWindowImages::Update(state, runtime);
    Check(runtime.calls.front() == "unlock-mask" &&
              runtime.words.at(0x2418u) == 102u &&
              runtime.words.at(0x2518u) == 103u,
          "unlocked entries store GameDB task IDs in image fields");
    Check(runtime.calls[1] == "has:0" &&
              runtime.calls[2] == "disabled-image:8448" &&
              runtime.calls[3] == "enabled:8448-" &&
              runtime.calls[4] == "has:1" &&
              runtime.calls[5] == "task:1" &&
              runtime.calls[6] == "disabled-image:8704" &&
              runtime.calls[7] == "enabled:8704-",
          "missing and locked entries share disabled-image fallback");
    Check(runtime.calls[8] == "has:2" &&
              runtime.calls[9] == "task:2" &&
              runtime.calls[10] == "enabled:8960+" &&
              runtime.calls[11] == "visible:8960" &&
              runtime.calls[12] == "image-name" &&
              runtime.calls[13] == "utf8" &&
              runtime.calls[14] == "set-image:8960" &&
              runtime.calls[15] == "id" &&
              runtime.calls[16] == "write:9240",
          "unlocked image enabled, shown, assigned and tagged in order");
    Check(runtime.calls.back() == "enabled:11520-",
          "last missing image is disabled");
}

void CheckClickAndFallback() {
    auto state = State();
    Runtime runtime;
    runtime.words[0x2418u] = 102u;
    Check(pal4::MoviePreviewWindowImages::HandleItemClick(
              state, 0x2300u, runtime) == 7u &&
              runtime.calls == std::vector<std::string>{
                  "read:9240", "has:102", "task:102", "hide", "dispatch"},
          "click hides preview then dispatches playback task");
    runtime.calls.clear();
    runtime.click_task = false;
    Check(pal4::MoviePreviewWindowImages::HandleItemClick(
              state, 0x2300u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:9240", "has:102"},
          "missing clicked task returns zero without hiding");
    runtime.calls.clear();
    pal4::MoviePreviewWindowImages::SetImageAndDisable(
        state, 0x2300u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "disabled-image:8960", "enabled:8960-"},
          "fallback sets saved image pointer before disabling");
}

}  // namespace

int main() {
    CheckImageRefresh();
    CheckClickAndFallback();
    std::cout << "Movie preview window image tests passed\n";
}
