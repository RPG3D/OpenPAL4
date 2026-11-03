#include "pal4/ol_info_visibility.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::OlInfoVisibilityRuntimeStub {
    std::vector<std::uint32_t> ids;
    std::vector<std::string> created;
    std::vector<std::pair<std::uint32_t, std::string>> texts;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::vector<std::string> calls;
    std::string source;
    std::int32_t progress{};
    std::uint32_t progress_calls{};
    std::uint32_t fail_text_window{};
    bool embedded_nul{};

    void SetParent(pal4::OlInfoState&, std::uint8_t showing,
                   std::uint32_t parent) override {
        Check(showing == 0u || showing == 2u,
              "dialog receives original show byte");
        Check(parent == 0x6000u, "dialog receives original parent");
        calls.emplace_back("parent");
    }
    std::string_view TriggerText(std::uint32_t id) override {
        Check((id >= 219u && id <= 226u) || id == 0xe3u,
              "only observed GameDB trigger IDs are read");
        ids.push_back(id);
        calls.emplace_back("trigger");
        source = id == 221u ? "" : "Text" + std::to_string(id);
        return source;
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.emplace_back("utf8");
        if (embedded_nul) return std::string(text) + std::string("\0tail", 5);
        return "U:" + std::string(text);
    }
    std::uint32_t CreateCeguiString(std::string_view text) override {
        calls.emplace_back("create");
        created.emplace_back(text);
        return 0x9000u + static_cast<std::uint32_t>(created.size());
    }
    void SetText(std::uint32_t window,
                 std::uint32_t text) override {
        Check((window >= 0x3001u && window <= 0x3008u) ||
                  window == 0x4000u,
              "text targets one of eight info rows or description");
        calls.emplace_back("set-text");
        texts.emplace_back(window, created.at(text - 0x9001u));
        if (window == fail_text_window) throw std::runtime_error("set-text");
    }
    void DestroyCeguiString(std::uint32_t text) override {
        calls.emplace_back("destroy");
        destroyed.push_back(text);
    }
    void SetWindowVisible(std::uint32_t window,
                          bool visible) override {
        Check(window >= 0x3001u && window <= 0x3008u,
              "visibility only changes StaticInfo rows");
        calls.emplace_back(visible ? "show" : "hide");
        visibility.emplace_back(window, visible);
    }
    std::int32_t ProgressCount() override {
        calls.emplace_back("count");
        ++progress_calls;
        return progress;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t InsertActive(std::uint32_t array,
                               std::uint32_t member) override {
        Check(array == 0x8134u && member == 0x2028u,
              "show inserts member +0x28 in manager array +0x134");
        calls.emplace_back("insert");
        return 0x1111u;
    }
    std::uint32_t RemoveActive(std::uint32_t array,
                               std::uint32_t member) override {
        Check(array == 0x8134u && member == 0x2028u,
              "hide removes same active member");
        calls.emplace_back("remove");
        return 0x2222u;
    }
};

pal4::OlInfoState MakeState() {
    pal4::OlInfoState state{};
    for (std::uint32_t index = 0u; index < 8u; ++index)
        state.Write(0x2cu + index * 4u, 0x3001u + index);
    state.Write(0x4cu, 0x4000u);
    return state;
}

void CheckRows(const Runtime& runtime, std::size_t shown) {
    Check(runtime.ids.size() == 9u && runtime.ids.back() == 0xe3u &&
              runtime.created.size() == 9u &&
              runtime.texts.size() == 9u &&
              runtime.destroyed.size() == 9u &&
              runtime.progress_calls == 2u &&
              runtime.visibility.size() == 8u + shown,
          "all eight rows and description refresh on every visibility call");
    for (std::uint32_t index = 0u; index < 8u; ++index) {
        Check(runtime.ids[index] == 219u + index &&
                  runtime.texts[index].first == 0x3001u + index &&
                  runtime.visibility[index] ==
                      std::pair<std::uint32_t, bool>{0x3001u + index, false},
              "row ID, slot and initial hide follow sequential order");
    }
    for (std::size_t index = 0u; index < shown; ++index)
        Check(runtime.visibility[8u + index] ==
                  std::pair<std::uint32_t, bool>{
                      0x3001u + static_cast<std::uint32_t>(index), true},
              "visible rows begin at StaticInfo1");
    Check(runtime.texts[2].second == "U:" &&
              runtime.texts[8].first == 0x4000u &&
              runtime.texts[8].second == "U:Text227",
          "missing row text falls back to empty and description uses 0xe3");
}

}  // namespace

int main() {
    auto state = MakeState();
    Runtime runtime;
    runtime.progress = 1;
    Check(pal4::OlInfoVisibility::SetVisible(
              state, 0x2000u, 2u, 0x6000u, runtime) == 0x1111u,
          "show forwards active-list insertion result");
    CheckRows(runtime, 4u);
    Check(runtime.calls.front() == "parent" &&
              runtime.calls.back() == "insert" &&
              runtime.calls[runtime.calls.size() - 2u] == "manager",
          "parent runs first and active-list insertion runs last");

    runtime = Runtime{};
    runtime.progress = 2;
    Check(pal4::OlInfoVisibility::SetVisible(
              state, 0x2000u, 0u, 0x6000u, runtime) == 0x2222u,
          "hide forwards active-list removal result");
    CheckRows(runtime, 8u);
    Check(runtime.calls.back() == "remove",
          "hide still refreshes text and shows eight rows before removal");

    for (const auto count : {0, -1}) {
        runtime = Runtime{};
        runtime.progress = count;
        (void)pal4::OlInfoVisibility::SetVisible(
            state, 0x2000u, 0u, 0x6000u, runtime);
        CheckRows(runtime, 0u);
    }

    runtime = Runtime{};
    runtime.fail_text_window = 0x3003u;
    try {
        (void)pal4::OlInfoVisibility::SetVisible(
            state, 0x2000u, 2u, 0x6000u, runtime);
        Check(false, "row setText failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.destroyed.size() == 3u &&
                  runtime.visibility.size() == 2u &&
                  runtime.progress_calls == 0u,
              "failing CEGUI text is destroyed before aborting row refresh");
    }

    runtime = Runtime{};
    runtime.fail_text_window = 0x4000u;
    try {
        (void)pal4::OlInfoVisibility::SetVisible(
            state, 0x2000u, 2u, 0x6000u, runtime);
        Check(false, "description setText failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.destroyed.size() == 9u &&
                  runtime.calls.back() == "destroy",
              "description exception destroys its temporary CEGUI string");
    }

    runtime = Runtime{};
    runtime.embedded_nul = true;
    (void)pal4::OlInfoVisibility::SetVisible(
        state, 0x2000u, 0u, 0x6000u, runtime);
    Check(runtime.created[0] == "Text219" &&
              runtime.created[2].empty(),
          "converted UTF-8 text stops at its first NUL byte");
    std::cout << "olInfo visibility tests passed\n";
}
