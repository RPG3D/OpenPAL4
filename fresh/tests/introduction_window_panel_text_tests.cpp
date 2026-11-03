#include "pal4/introduction_window_panel_text.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IntroductionWindowPanelRuntimeStub {
    pal4::IntroductionWindowState* state{};
    std::int32_t group_id = 1;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, bool>> visible;
    std::vector<std::pair<std::uint32_t, bool>> enabled;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> assigned_ids;
    std::vector<std::string> built_text;
    std::vector<std::uint32_t> subwindow_slots;
    std::vector<std::string> calls;
    std::uint32_t database_calls{};
    std::uint32_t destroyed_strings{};
    std::uint32_t selected_window{};
    std::uint32_t inserted_member{};
    std::uint32_t clicked_window = 0x8800u;
    std::uint32_t text_window{};
    bool npos_equals_length{};
    bool fail_set_text{};

    void SetVisible(std::uint32_t window, bool value) override {
        visible.emplace_back(window, value);
        calls.emplace_back(value ? "visible" : "hidden");
    }
    void SetEnabled(std::uint32_t window, bool value) override {
        enabled.emplace_back(window, value);
        calls.emplace_back(value ? "enabled" : "disabled");
    }
    std::int32_t WindowUserId(std::uint32_t window) override {
        Check(window == clicked_window,
              "main selection reads clicked window ID at +0x118");
        calls.emplace_back("group");
        return group_id;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::uint32_t SubWindowAt(
        const pal4::IntroductionWindowState& object,
        std::uint32_t address) override {
        subwindow_slots.push_back(address);
        return object.Read(address - 0x1000u);
    }
    std::uint32_t GameDatabase() override {
        ++database_calls;
        calls.emplace_back("database");
        return 0x2000u;
    }
    bool HasTrigger(std::uint32_t member,
                    std::uint32_t trigger) override {
        Check(member == 0x2034u,
              "trigger collection is GameDB +0x34");
        calls.emplace_back("has-trigger");
        return trigger != 12u;
    }
    std::uint32_t TriggerRecord(std::uint32_t member,
                                 std::uint32_t trigger) override {
        Check(member == 0x2034u && (trigger == 11u || trigger == 13u),
              "record lookup reuses GameDB +0x34 and trigger ID");
        calls.emplace_back("record");
        return 0x4000u + trigger;
    }
    std::string_view TriggerText(std::uint32_t record) override {
        Check(record == 0x400bu || record == 0x400du,
              "trigger text comes from found record");
        calls.emplace_back("text");
        return record == 0x400bu ? std::string_view{"TEXT"} :
                                   std::string_view{};
    }
    std::string_view TriggerDescription(std::uint32_t record) override {
        Check(record == 0x400bu,
              "description comes from the trigger record at +0x18");
        calls.emplace_back("description");
        return "DETAIL";
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.emplace_back("utf8");
        if (text == "DETAIL") return std::string("DETAIL\0TAIL", 11);
        return text.empty() ? std::string{} : std::string("TEXT\0TAIL", 9);
    }
    std::uint32_t CeguiNpos() override {
        calls.emplace_back("npos");
        return npos_equals_length ? 4u : 0xffffffffu;
    }
    std::uint32_t BuildCeguiString(
        std::string_view utf8, std::uint32_t length) override {
        Check(length == utf8.size(),
              "CEGUI builder uses byte count before first NUL");
        built_text.emplace_back(utf8);
        calls.emplace_back("build");
        return 0x5000u + static_cast<std::uint32_t>(built_text.size());
    }
    void SetText(std::uint32_t window,
                 std::uint32_t text) override {
        Check((window == 0x9800u || window == 0xa000u ||
               window == 0x8000u) &&
                  text >= 0x5001u,
              "subwindow receives temporary CEGUI string");
        text_window = window;
        calls.emplace_back("set-text");
        if (fail_set_text) throw std::runtime_error("set-text");
    }
    void DestroyCeguiString(std::uint32_t text) override {
        Check(text >= 0x5001u,
              "temporary string is destroyed after use");
        ++destroyed_strings;
        calls.emplace_back("destroy-string");
    }
    void SetWindowUserId(std::uint32_t window,
                         std::uint32_t id) override {
        assigned_ids.emplace_back(window, id);
        calls.emplace_back("assign-id");
    }
    void SelectFirstSub(pal4::IntroductionWindowState& object,
                        std::uint32_t window) override {
        Check(object.Read(0x3cu) == 0x777u,
              "first sub selection precedes group field update");
        selected_window = window;
        calls.emplace_back("select-first");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x9000u;
    }
    void InsertActive(std::uint32_t collection,
                      std::uint32_t member) override {
        Check(collection == 0x9134u && state != nullptr &&
                  state->Read(0x3cu) ==
                      static_cast<std::uint32_t>(group_id),
              "active insertion follows selected-group write");
        inserted_member = member;
        calls.emplace_back("insert");
    }
};

pal4::IntroductionWindowState MakeState() {
    pal4::IntroductionWindowState state{};
    state.Write(0x80u, 0x8000u);
    state.Write(0xacu, 0xac00u);
    state.Write(0x88u, 0x8800u);
    state.Write(0x8cu, 0x8c00u);
    state.Write(0x90u, 0x9000u);
    state.Write(0x94u, 0x9400u);
    state.Write(0x98u, 0x9800u);
    state.Write(0x9cu, 0x9c00u);
    state.Write(0xa0u, 0xa000u);
    state.Write(0xa4u, 0xa400u);
    state.Write(0xa8u, 0xa800u);
    state.Write(0x3cu, 0x777u);
    return state;
}

void SeedGroup(Runtime& runtime) {
    runtime.words[0x1054u] = 0x3000u;
    runtime.words[0x1058u] = 0x300cu;
    runtime.words[0x3000u] = 11u;
    runtime.words[0x3004u] = 12u;
    runtime.words[0x3008u] = 13u;
}

void CheckMainGroup() {
    auto state = MakeState();
    Runtime runtime;
    runtime.state = &state;
    SeedGroup(runtime);
    pal4::IntroductionWindowPanelText::ShowMainGroup(
        state, 0x1000u, 0x8800u, runtime);
    Check(runtime.visible ==
              std::vector<std::pair<std::uint32_t, bool>>{
                  {0xac00u, true}, {0x9800u, false},
                  {0x9c00u, false}, {0xa000u, false},
                  {0xa400u, false}, {0xa800u, false},
                  {0x9800u, true}, {0xa000u, true}} &&
              runtime.enabled ==
                  std::vector<std::pair<std::uint32_t, bool>>{
                      {0x8800u, true}, {0x8c00u, true},
                      {0x9000u, true}, {0x9400u, true},
                      {0x8800u, false}} &&
              runtime.database_calls == 5u &&
              runtime.subwindow_slots ==
                  std::vector<std::uint32_t>{0x1098u, 0x10a0u} &&
              runtime.built_text ==
                  std::vector<std::string>{"TEXT", ""} &&
              runtime.destroyed_strings == 2u &&
              runtime.assigned_ids ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x9800u, 11u}, {0xa000u, 13u}} &&
              runtime.selected_window == 0x9800u &&
              runtime.inserted_member == 0x10b0u &&
              state.Read(0x3cu) == 1u,
          "main group shows valid triggers, selects first and inserts member");
}

void CheckEmptyAndOutOfRange() {
    auto state = MakeState();
    Runtime runtime;
    runtime.state = &state;
    runtime.group_id = 4;
    pal4::IntroductionWindowPanelText::ShowMainGroup(
        state, 0x1000u, 0x8800u, runtime);
    Check(runtime.database_calls == 0u &&
              runtime.inserted_member == 0u &&
              state.Read(0x3cu) == 0x777u,
          "group four returns after clearing controls");

    runtime = Runtime{};
    runtime.state = &state;
    runtime.words[0x1054u] = 0u;
    pal4::IntroductionWindowPanelText::ShowMainGroup(
        state, 0x1000u, 0x8800u, runtime);
    Check(runtime.database_calls == 0u &&
              runtime.selected_window == 0u &&
              runtime.inserted_member == 0x10b0u &&
              state.Read(0x3cu) == 1u,
          "empty collection still records group and active member");

    state.Write(0x3cu, 0x777u);
    runtime = Runtime{};
    runtime.state = &state;
    runtime.group_id = -1;
    runtime.words[0x1034u] = 0u;
    pal4::IntroductionWindowPanelText::ShowMainGroup(
        state, 0x1000u, 0x8800u, runtime);
    Check(state.Read(0x3cu) == 0xffffffffu &&
              runtime.inserted_member == 0x10b0u,
          "signed comparison admits negative group IDs as observed");
}

void CheckEncodingFailures() {
    auto state = MakeState();
    Runtime runtime;
    runtime.state = &state;
    runtime.npos_equals_length = true;
    SeedGroup(runtime);
    try {
        pal4::IntroductionWindowPanelText::ShowMainGroup(
            state, 0x1000u, 0x8800u, runtime);
        Check(false, "CEGUI npos length must throw");
    } catch (const std::length_error&) {
        Check(runtime.built_text.empty() &&
                  runtime.inserted_member == 0u &&
                  state.Read(0x3cu) == 0x777u,
              "npos throws before CEGUI string creation and active insertion");
    }

    runtime = Runtime{};
    runtime.state = &state;
    runtime.fail_set_text = true;
    SeedGroup(runtime);
    try {
        pal4::IntroductionWindowPanelText::ShowMainGroup(
            state, 0x1000u, 0x8800u, runtime);
        Check(false, "CEGUI setText exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.built_text.size() == 1u &&
                  runtime.destroyed_strings == 1u &&
                  runtime.inserted_member == 0u,
              "setText failure destroys temporary string and stops insertion");
    }
}

void CheckSubSelection() {
    auto state = MakeState();
    Runtime runtime;
    runtime.clicked_window = 0x9800u;
    runtime.group_id = 11;
    pal4::IntroductionWindowPanelText::ShowSubSelection(
        state, 0x9800u, runtime);
    Check(runtime.enabled ==
              std::vector<std::pair<std::uint32_t, bool>>{
                  {0x9800u, true}, {0x9c00u, true},
                  {0xa000u, true}, {0xa400u, true},
                  {0xa800u, true}, {0x9800u, false}} &&
              runtime.database_calls == 2u &&
              runtime.built_text == std::vector<std::string>{"DETAIL"} &&
              runtime.text_window == 0x8000u &&
              runtime.destroyed_strings == 1u &&
              state.Read(0x3cu) == 0x777u,
          "sub selection enables five controls and sets description");

    runtime = Runtime{};
    runtime.clicked_window = 0x9800u;
    runtime.group_id = 12;
    pal4::IntroductionWindowPanelText::ShowSubSelection(
        state, 0x9800u, runtime);
    Check(runtime.database_calls == 1u &&
              runtime.built_text.empty() &&
              runtime.text_window == 0u,
          "missing trigger leaves information text untouched");

    runtime = Runtime{};
    runtime.clicked_window = 0x9800u;
    runtime.group_id = 11;
    runtime.fail_set_text = true;
    try {
        pal4::IntroductionWindowPanelText::ShowSubSelection(
            state, 0x9800u, runtime);
        Check(false, "description setText must propagate failure");
    } catch (const std::runtime_error&) {
        Check(runtime.destroyed_strings == 1u,
              "description temporary string is destroyed on failure");
    }
}

void CheckEventWrappers() {
    auto state = MakeState();
    Runtime runtime;
    runtime.state = &state;
    runtime.words[0x7008u] = 0x8800u;
    runtime.words[0x1054u] = 0u;
    Check(pal4::IntroductionWindowPanelText::OnMainButtonEvent(
              state, 0x1000u, 0x7000u, runtime) == 1u &&
              runtime.inserted_member == 0x10b0u &&
              state.Read(0x3cu) == 1u,
          "main event reads window from event +8 and returns true");

    runtime = Runtime{};
    runtime.clicked_window = 0x9800u;
    runtime.group_id = 12;
    runtime.words[0x7008u] = 0x9800u;
    Check(pal4::IntroductionWindowPanelText::OnSubButtonEvent(
              state, 0x7000u, runtime) == 1u &&
              runtime.database_calls == 1u,
          "sub event forwards window and returns true");
}

}  // namespace

int main() {
    CheckMainGroup();
    CheckEmptyAndOutOfRange();
    CheckEncodingFailures();
    CheckSubSelection();
    CheckEventWrappers();
    std::cout << "Introduction window panel text tests passed\n";
}
