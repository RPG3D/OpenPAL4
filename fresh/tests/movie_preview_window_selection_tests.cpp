#include "pal4/movie_preview_window_selection.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
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

struct Runtime final : pal4::MoviePreviewWindowSelectionRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    bool disabled{};
    bool has_task{true};
    std::u32string text;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    bool IsDisabled(std::uint32_t window, bool inherited) override {
        Check(window != 0u && !inherited, "selected window disabled query");
        calls.emplace_back("disabled?");
        return disabled;
    }
    void RemoveChild(std::uint32_t parent,
                     std::uint32_t child) override {
        Check(parent == 0x7000u && child == 0x5000u,
              "mask removed from old parent");
        calls.emplace_back("remove-child");
    }
    void AddChild(std::uint32_t parent,
                  std::uint32_t child) override {
        Check(parent != 0u && child == 0x5000u,
              "mask added to selected image");
        calls.emplace_back("add-child");
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window == 0x5000u, "mask visibility target");
        calls.emplace_back(visible ? "visible+" : "visible-");
    }
    void SetWidth(std::uint32_t window, std::uint32_t mode,
                  float value) override {
        Check(window == 0x5000u && mode == 0u && value == 1.0f,
              "mask width fills image");
        calls.emplace_back("width");
    }
    void SetHeight(std::uint32_t window, std::uint32_t mode,
                   float value) override {
        Check(window == 0x5000u && mode == 0u && value == 1.0f,
              "mask height fills image");
        calls.emplace_back("height");
    }
    void SetX(std::uint32_t window, std::uint32_t mode,
              float value) override {
        Check(window == 0x5000u && mode == 0u && value == 0.0f,
              "mask X position reset");
        calls.emplace_back("x");
    }
    void SetY(std::uint32_t window, std::uint32_t mode,
              float value) override {
        Check(window == 0x5000u && mode == 0u && value == 0.0f,
              "mask Y position reset");
        calls.emplace_back("y");
    }
    bool HasTriggerTask(std::uint32_t index) override {
        Check(index == 5u, "task index read from selected image");
        calls.emplace_back("has-task");
        return has_task;
    }
    std::string TriggerTaskText(std::uint32_t index) override {
        Check(index == 5u, "task text index");
        calls.emplace_back("task-text");
        return "raw";
    }
    std::string ConvertToUtf8(std::string_view raw) override {
        Check(raw == "raw", "raw task text converted");
        calls.emplace_back("utf8");
        return "A\xE4\xB8\xAD\xF0\x9F\x98\x80";
    }
    void SetText(std::uint32_t window,
                 std::u32string_view value) override {
        Check(window == 0x6000u, "static info text target");
        calls.emplace_back("set-text");
        text = value;
    }
    void HandleItemClick(pal4::MoviePreviewWindowState&,
                         std::uint32_t selected) override {
        Check(selected == 0x2300u, "mouse click forwards selected image");
        calls.emplace_back("item-click");
    }
};

pal4::MoviePreviewWindowState State() {
    pal4::MoviePreviewWindowState state{};
    for (std::uint32_t i = 0u; i < 13u; ++i)
        state.Write(0x2cu + i * 4u, 0x2100u + i * 0x100u);
    state.Write(0x60u, 0x6000u);
    state.Write(0x68u, 0x5000u);
    return state;
}

void CheckSelection() {
    auto state = State();
    Runtime runtime;
    runtime.words[0x5044u] = 0x7000u;
    runtime.words[0x2418u] = 5u;
    pal4::MoviePreviewWindowSelection::UpdateStaticInfo(
        state, 0x2300u, runtime);
    Check(runtime.words.at(0x5118u) == 2u &&
              runtime.text == U"A\u4e2d\U0001f600" &&
              runtime.calls == std::vector<std::string>{
                  "disabled?", "read:20548", "remove-child", "add-child",
                  "visible+", "width", "height", "x", "y",
                  "write:20760:0", "write:20760:2", "read:9240",
                  "has-task", "task-text", "utf8", "set-text"},
          "selection reparents mask and decodes UTF-8 description");
    runtime.calls.clear();
    pal4::MoviePreviewWindowSelection::UpdateStaticInfo(state, 0u, runtime);
    Check(runtime.calls == std::vector<std::string>{"visible-"},
          "null selection only hides mask");
    runtime.calls.clear();
    runtime.disabled = true;
    pal4::MoviePreviewWindowSelection::UpdateStaticInfo(
        state, 0x2300u, runtime);
    Check(runtime.calls == std::vector<std::string>{"disabled?"},
          "disabled selection leaves mask and text unchanged");
}

void CheckMissingTaskAndUnmatched() {
    auto state = State();
    Runtime runtime;
    runtime.words[0x5044u] = 0u;
    runtime.words[0x9918u] = 5u;
    runtime.has_task = false;
    pal4::MoviePreviewWindowSelection::UpdateStaticInfo(
        state, 0x9800u, runtime);
    Check(runtime.words.at(0x5118u) == 0u &&
              runtime.calls.back() == "has-task" && runtime.text.empty(),
          "unmatched image uses index zero and missing task skips text");
}

void CheckMouseWrappersAndDecoder() {
    auto state = State();
    Runtime runtime;
    runtime.words[0x8008u] = 0u;
    runtime.words[0x801cu] = 1u;
    Check(pal4::MoviePreviewWindowSelection::OnMouseMove(
              state, 0x8000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "read:32776", "visible-"},
          "mouse move passes event window to static-info update");
    runtime.calls.clear();
    Check(pal4::MoviePreviewWindowSelection::OnMouseClick(
              state, 0x8000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"read:32796"},
          "nonzero mouse button skips item click");
    runtime.calls.clear();
    runtime.words[0x801cu] = 0u;
    runtime.words[0x8008u] = 0x2300u;
    Check(pal4::MoviePreviewWindowSelection::OnMouseClick(
              state, 0x8000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "read:32796", "read:32776", "item-click"},
          "zero mouse button forwards selected image");
    Check(pal4::MoviePreviewWindowSelection::DecodeUtf8(
              "abc\0tail") == U"abc" &&
              pal4::MoviePreviewWindowSelection::DecodeUtf8(
                  "\xC3\xA9") == U"\u00e9",
          "decoder supports NUL termination and two-byte UTF-8");
    try {
        (void)pal4::MoviePreviewWindowSelection::DecodeUtf8("\xF0\x9F");
        Check(false, "truncated multibyte sequence must fail");
    } catch (const std::length_error&) {
    }
}

}  // namespace

int main() {
    CheckSelection();
    CheckMissingTaskAndUnmatched();
    CheckMouseWrappersAndDecoder();
    std::cout << "Movie preview window selection tests passed\n";
}
