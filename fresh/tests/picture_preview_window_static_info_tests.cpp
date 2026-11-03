#include "pal4/picture_preview_window_selection.h"

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

struct Runtime final : pal4::PicturePreviewWindowSelectionRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::u32string text;
    bool disabled{};
    bool has_task{true};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(value));
    }
    bool IsDisabled(std::uint32_t window, bool inherited) override {
        Check(window != 0u && !inherited, "disabled query excludes parents");
        calls.emplace_back("disabled?");
        return disabled;
    }
    void RemoveChild(std::uint32_t parent,
                     std::uint32_t child) override {
        Check(parent == 0x7000u && child == 0x5000u,
              "mask removed from old parent");
        calls.emplace_back("remove");
    }
    void AddChild(std::uint32_t parent,
                  std::uint32_t child) override {
        Check(parent == 0x2a00u && child == 0x5000u,
              "mask attached to tenth image");
        calls.emplace_back("add");
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
              "mask X reset");
        calls.emplace_back("x");
    }
    void SetY(std::uint32_t window, std::uint32_t mode,
              float value) override {
        Check(window == 0x5000u && mode == 0u && value == 0.0f,
              "mask Y reset");
        calls.emplace_back("y");
    }
    bool HasTriggerTask(std::uint32_t index) override {
        Check(index == 5u, "task index read from image");
        calls.emplace_back("has-task");
        return has_task;
    }
    std::string TriggerTaskText(std::uint32_t index) override {
        Check(index == 5u, "task text index");
        calls.emplace_back("task-text");
        return "raw";
    }
    std::string ConvertToUtf8(std::string_view raw) override {
        Check(raw == "raw", "task text converted");
        calls.emplace_back("utf8");
        return "A\xE4\xB8\xAD\xF0\x9F\x98\x80";
    }
    void SetText(std::uint32_t window,
                 std::u32string_view value) override {
        Check(window == 0x6000u, "StaticInfo control receives text");
        calls.emplace_back("set-text");
        text = value;
    }
};

pal4::PicturePreviewWindowState State() {
    pal4::PicturePreviewWindowState state{};
    for (std::uint32_t i = 0u; i < 10u; ++i)
        state.Write(0x2cu + i * 4u, 0x2100u + i * 0x100u);
    state.Write(0x54u, 0x6000u);
    state.Write(0x5cu, 0x5000u);
    return state;
}

void CheckSelection() {
    auto state = State();
    Runtime runtime;
    runtime.words[0x5044u] = 0x7000u;
    runtime.words[0x2b18u] = 5u;
    pal4::PicturePreviewWindowSelection::UpdateStaticInfo(
        state, 0x2a00u, runtime);
    Check(runtime.words.at(0x5118u) == 9u &&
              runtime.text == U"A\u4e2d\U0001f600" &&
              runtime.calls == std::vector<std::string>{
                  "disabled?", "read:20548", "remove", "add", "visible+",
                  "width", "height", "x", "y", "write:0", "write:9",
                  "read:11032", "has-task", "task-text", "utf8", "set-text"},
          "tenth image selection updates mask and converted description");
}

void CheckGates() {
    auto state = State();
    Runtime runtime;
    pal4::PicturePreviewWindowSelection::UpdateStaticInfo(state, 0u, runtime);
    Check(runtime.calls == std::vector<std::string>{"visible-"},
          "null image only hides mask");
    runtime.calls.clear();
    runtime.disabled = true;
    pal4::PicturePreviewWindowSelection::UpdateStaticInfo(
        state, 0x2a00u, runtime);
    Check(runtime.calls == std::vector<std::string>{"disabled?"},
          "disabled image does not move mask");
    runtime.calls.clear();
    runtime.disabled = false;
    runtime.has_task = false;
    runtime.words[0x5044u] = 0u;
    runtime.words[0x2b18u] = 5u;
    pal4::PicturePreviewWindowSelection::UpdateStaticInfo(
        state, 0x2a00u, runtime);
    Check(runtime.words.at(0x5118u) == 9u &&
              runtime.calls.back() == "has-task" && runtime.text.empty(),
          "missing task leaves mask selected without text update");
    runtime.calls.clear();
    runtime.has_task = true;
    runtime.words.erase(0x2b18u);
    try {
        pal4::PicturePreviewWindowSelection::UpdateStaticInfo(
            state, 0x2a00u, runtime);
        Check(false, "missing image field must propagate");
    } catch (const std::out_of_range&) {
        Check(runtime.calls.back() == "read:11032" &&
                  runtime.text.empty(), "field failure stops before text");
    }
}

}  // namespace

int main() {
    CheckSelection();
    CheckGates();
    std::cout << "Picture preview static info tests passed\n";
}
