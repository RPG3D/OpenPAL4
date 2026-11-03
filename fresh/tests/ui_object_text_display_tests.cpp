#include "pal4/ui_object_text_display.h"
#include "pal4/ui_object_visibility.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
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

struct Runtime final : pal4::UiObjectTextRuntimeStub {
    std::vector<char> events;
    bool empty_text{};
    bool fail_set_text{};
    bool npos_equals_length{};
    bool embedded_nul{};
    int destroy_count{};

    std::uint32_t EventWindow(std::uint32_t address) override {
        Check(address == 0xa008u,
              "click wrapper reads source window from event arguments +8");
        events.push_back('W');
        return 0x8800u;
    }

    void SetEnabled(std::uint32_t window, bool enabled) override {
        Check((window == 0x4800u && enabled) ||
                  (window == 0x4c00u && enabled) ||
                  (window == 0x8800u && !enabled),
              "Cast/Website are enabled and clicked window disabled");
        events.push_back(enabled ? 'E' : 'e');
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window == 0x5000u && visible,
              "PanelInfo window becomes visible");
        events.push_back('V');
    }
    std::uint32_t WindowUserId(std::uint32_t window) override {
        Check(window == 0x8800u,
              "clicked window supplies user ID at +0x118");
        events.push_back('I');
        return 18u;
    }
    std::uint32_t GameDatabase() override {
        events.push_back('D');
        return 0x1000u;
    }
    std::uint32_t TriggerRecord(std::uint32_t member,
                                 std::uint32_t id) override {
        Check(member == 0x1034u && id == 18u,
              "trigger lookup uses GameDB +0x34 and window ID");
        events.push_back('R');
        return 0x2000u;
    }
    std::string_view TriggerText(std::uint32_t record) override {
        Check(record == 0x2000u,
              "text lookup receives trigger record");
        events.push_back('T');
        return empty_text ? std::string_view{} : std::string_view{"TEXT"};
    }
    std::string ConvertToUtf8(std::string_view text) override {
        Check(text == (empty_text ? "" : "TEXT"),
              "UTF-8 conversion receives trigger text or empty fallback");
        events.push_back('C');
        return embedded_nul ? std::string("TEXT\0TAIL", 9) :
                              std::string(text);
    }
    std::uint32_t CeguiNpos() override {
        events.push_back('N');
        return npos_equals_length ? (empty_text ? 0u : 4u) : 0xffffffffu;
    }
    std::uint32_t BuildCeguiString(
        std::string_view utf8, std::uint32_t length) override {
        Check(utf8 == (empty_text ? "" : "TEXT") &&
                  length == utf8.size(),
              "CEGUI string builder receives UTF-8 bytes and length");
        events.push_back('B');
        return 0x3000u;
    }
    void SetText(std::uint32_t window,
                 std::uint32_t text) override {
        Check(window == 0x4000u && text == 0x3000u,
              "EditInfo window receives temporary CEGUI string");
        events.push_back('S');
        if (fail_set_text) throw std::runtime_error("setText");
    }
    void DestroyCeguiString(std::uint32_t address) override {
        Check(address == 0x3000u,
              "temporary CEGUI string destructor receives same object");
        ++destroy_count;
        events.push_back('X');
    }
    std::uint32_t FrameManager() override {
        events.push_back('M');
        return 0x9000u;
    }
    int InsertActive(std::uint32_t collection,
                      std::uint32_t member) override {
        Check(collection == 0x9134u && member == 0x7054u,
              "active array receives manager +0x134 and UI object +0x54");
        events.push_back('A');
        return -9;
    }
};

pal4::UiObjectState MakeState() {
    pal4::UiObjectState state{};
    state.Write(0x40u, 0x4000u);
    state.Write(0x48u, 0x4800u);
    state.Write(0x4cu, 0x4c00u);
    state.Write(0x50u, 0x5000u);
    return state;
}

void CheckTextFlow() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::UiObjectTextDisplay::SetTextAndVisibility(
              state, 0x7000u, 0x8800u, runtime) == -9 &&
              runtime.events == std::vector<char>{
                  'E', 'E', 'e', 'V', 'I', 'D', 'R', 'T', 'C', 'N',
                  'B', 'S', 'X', 'M', 'A'} &&
              runtime.destroy_count == 1,
          "text flow uses GameDB and CEGUI before active-array insertion");
    runtime.events.clear();
    runtime.empty_text = true;
    Check(pal4::UiObjectTextDisplay::SetTextAndVisibility(
              state, 0x7000u, 0x8800u, runtime) == -9 &&
              runtime.destroy_count == 2,
          "missing trigger text uses empty conversion fallback");
    runtime.events.clear();
    runtime.empty_text = false;
    runtime.embedded_nul = true;
    Check(pal4::UiObjectTextDisplay::SetTextAndVisibility(
              state, 0x7000u, 0x8800u, runtime) == -9 &&
              runtime.destroy_count == 3,
          "UTF-8 byte count stops at first NUL like original strlen");
}

void CheckClickWrapper() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::UiObjectTextDisplay::HandleClick(
              state, 0x7000u, 0xa000u, runtime) &&
              runtime.events == std::vector<char>{
                  'W', 'E', 'E', 'e', 'V', 'I', 'D', 'R', 'T', 'C',
                  'N', 'B', 'S', 'X', 'M', 'A'},
          "click wrapper forwards event window and returns success");
}

void CheckVisibilityIntegration() {
    struct VisibilityRuntime final : pal4::UiObjectVisibilityRuntimeStub {
        Runtime& text;
        explicit VisibilityRuntime(Runtime& runtime) : text(runtime) {}
        void SetParent(pal4::UiObjectState&, std::uint8_t,
                       std::uint32_t) override {}
        void SetVisible(std::uint32_t window, bool visible) override {
            if (visible) text.SetVisible(window, visible);
        }
        void SetEnabled(std::uint32_t window, bool enabled) override {
            if (enabled) text.SetEnabled(window, enabled);
        }
        std::uint32_t FrameManager() override { return 0x9000u; }
        void InsertActive(std::uint32_t, std::uint32_t) override {}
        int SetTextAndVisibility(pal4::UiObjectState& state,
                                  std::uint32_t) override {
            return pal4::UiObjectTextDisplay::SetTextAndVisibility(
                state, 0x7000u, 0x8800u, text);
        }
    };
    auto state = MakeState();
    Runtime text;
    VisibilityRuntime visibility(text);
    Check(pal4::UiObjectVisibility::SetVisibility(
              state, 0x7000u, 1u, 0u, visibility) == -9 &&
              text.destroy_count == 1,
          "show path can execute reconstructed text/display handler");
}

void CheckErrorPaths() {
    auto state = MakeState();
    Runtime runtime;
    runtime.npos_equals_length = true;
    bool length_error = false;
    try {
        (void)pal4::UiObjectTextDisplay::SetTextAndVisibility(
            state, 0x7000u, 0x8800u, runtime);
    } catch (const std::length_error& error) {
        length_error = std::string_view(error.what()) ==
            "Length for utf8 encoded string can not be 'npos'";
    }
    Check(length_error && runtime.events == std::vector<char>{
              'E', 'E', 'e', 'V', 'I', 'D', 'R', 'T', 'C', 'N'} &&
              runtime.destroy_count == 0,
          "CEGUI npos length throws before string creation");

    runtime = Runtime{};
    runtime.fail_set_text = true;
    bool set_text_error = false;
    try {
        (void)pal4::UiObjectTextDisplay::SetTextAndVisibility(
            state, 0x7000u, 0x8800u, runtime);
    } catch (const std::runtime_error&) {
        set_text_error = true;
    }
    Check(set_text_error && runtime.events == std::vector<char>{
              'E', 'E', 'e', 'V', 'I', 'D', 'R', 'T', 'C', 'N',
              'B', 'S', 'X'} && runtime.destroy_count == 1,
          "setText failure destroys CEGUI string and skips active insertion");
}

}  // namespace

int main() {
    CheckTextFlow();
    CheckClickWrapper();
    CheckErrorPaths();
    CheckVisibilityIntegration();
    std::cout << "UI object text display tests passed\n";
}
