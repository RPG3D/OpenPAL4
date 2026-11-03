#include "pal4/system_setting_navigation.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

constexpr std::uint32_t kFrame = 0x4000u;
constexpr std::uint32_t kMember = 0x4040u;
constexpr std::uint32_t kEvent = 0x5000u;
constexpr std::uint32_t kFirst = 0x10000u;
constexpr std::uint32_t kDisabled = 0x10100u;
constexpr std::uint32_t kLast = 0x10200u;
constexpr std::uint32_t kFirstWindow = 0x20000u;
constexpr std::uint32_t kDisabledWindow = 0x21000u;
constexpr std::uint32_t kLastWindow = 0x22000u;
constexpr std::uint32_t kFirstSlider = 0x30000u;
constexpr std::uint32_t kSecondSlider = 0x31000u;
constexpr std::uint32_t kFirstChild = 0x32000u;
constexpr std::uint32_t kSecondChild = 0x33000u;

struct Runtime final : pal4::ISystemSettingNavigationRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, float> floats;
    std::vector<std::uint32_t> redrawn;
    std::vector<std::pair<std::uint32_t, float>> slider_values;
    std::vector<std::string> calls;
    std::uint32_t removed_member = 0u;
    std::uint32_t shown_frame = 0u;
    std::uint32_t mouse_window = 0u;

    Runtime() {
        words[kMember + 4u] = kFrame;
        words[kMember + 12u] = 0x6000u;
        words[0x6000u] = kFirst;
        words[kMember + 24u] = 0u;
        words[kMember + 28u] = kFirst;
        words[kFrame + 496u] = kFirstSlider;
        words[kFrame + 500u] = kSecondSlider;
        words[kFirstSlider + 1212u] = kFirstChild;
        words[kSecondSlider + 1212u] = kSecondChild;
        floats[kFirstSlider + 1200u] = 0.5f;
        floats[kSecondSlider + 1200u] = 0.5f;
        words[kFirst] = kFirstWindow;
        words[kDisabled] = kDisabledWindow;
        words[kLast] = kLastWindow;
        for (std::uint32_t direction : {4u, 8u, 12u, 16u}) {
            words[kFirst + direction] = kDisabled;
            words[kDisabled + direction] = kLast;
            words[kLast + direction] = 0u;
        }
        bytes[kDisabledWindow + 1202u] = 1u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read-byte");
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("write-byte");
        bytes[address] = value;
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read-float");
        return floats.at(address);
    }
    void RequestRedraw(std::uint32_t window) override {
        calls.emplace_back("redraw");
        redrawn.push_back(window);
    }
    void SetSliderValue(std::uint32_t slider, float value) override {
        calls.emplace_back("set-slider");
        slider_values.emplace_back(slider, value);
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    void RemoveActive(std::uint32_t collection,
                      std::uint32_t member) override {
        calls.emplace_back("remove");
        Check(collection == 0x7000u + 308u, "active collection offset");
        removed_member = member;
    }
    void ShowSystemFrame(std::uint32_t frame) override {
        calls.emplace_back("show-system");
        shown_frame = frame;
    }
    void HandleMouseEvent(std::uint32_t window) override {
        calls.emplace_back("mouse");
        mouse_window = window;
    }
};

std::uint8_t Run(Runtime& runtime, std::uint32_t code) {
    runtime.words[kEvent + 16u] = code;
    return pal4::SystemSettingNavigation::HandleKey(
        kMember, kEvent, runtime);
}

void CheckHighlight(const Runtime& runtime,
                    std::uint32_t old_window,
                    std::uint32_t new_window) {
    Check(runtime.bytes.at(old_window + 1201u) == 0u &&
          runtime.bytes.at(new_window + 1201u) == 1u &&
          runtime.redrawn == std::vector<std::uint32_t>{
              old_window, new_window},
          "old highlight cleared before new highlight");
}

}  // namespace

int main() {
    Runtime back;
    Check(Run(back, 1u) == 1u &&
          back.removed_member == kMember && back.shown_frame == kFrame &&
          back.calls == std::vector<std::string>{
              "read", "manager", "remove", "read", "show-system"},
          "back key removes settings member before showing system frame");

    Runtime at_top;
    at_top.words[kMember + 24u] = kFirstSlider;
    Check(Run(at_top, 200u) == 1u && at_top.redrawn.empty() &&
          at_top.words.at(kMember + 24u) == kFirstSlider,
          "up from first slider stays there");

    Runtime up_slider;
    up_slider.words[kMember + 24u] = kSecondSlider;
    Check(Run(up_slider, 200u) == 1u &&
          up_slider.words.at(kMember + 24u) == kFirstSlider,
          "up from second slider reaches first");
    CheckHighlight(up_slider, kSecondChild, kFirstChild);

    Runtime up_record;
    Check(Run(up_record, 200u) == 1u &&
          up_record.words.at(kMember + 28u) == kLast,
          "up follows record +12 and skips disabled node");
    CheckHighlight(up_record, kFirstWindow, kLastWindow);

    Runtime up_boundary;
    up_boundary.words[kMember + 28u] = kLast;
    Check(Run(up_boundary, 200u) == 1u &&
          up_boundary.words.at(kMember + 28u) == 0u &&
          up_boundary.words.at(kMember + 24u) == kSecondSlider,
          "up from record boundary enters second slider");
    CheckHighlight(up_boundary, kLastWindow, kSecondChild);

    Runtime up_no_record;
    up_no_record.words[kMember + 28u] = 0u;
    Check(Run(up_no_record, 200u) == 1u &&
          up_no_record.redrawn.empty(),
          "up with no slider or record is a no-op");

    Runtime down_slider;
    down_slider.words[kMember + 24u] = kFirstSlider;
    Check(Run(down_slider, 208u) == 1u &&
          down_slider.words.at(kMember + 24u) == kSecondSlider,
          "down from first slider reaches second");
    CheckHighlight(down_slider, kFirstChild, kSecondChild);

    Runtime down_into_records;
    down_into_records.words[kMember + 24u] = kSecondSlider;
    down_into_records.bytes[kFirstWindow + 1202u] = 1u;
    Check(Run(down_into_records, 208u) == 1u &&
          down_into_records.words.at(kMember + 24u) == 0u &&
          down_into_records.words.at(kMember + 28u) == kLast,
          "down from second slider skips disabled first records");
    CheckHighlight(down_into_records, kSecondChild, kLastWindow);

    Runtime all_disabled;
    all_disabled.words[kMember + 24u] = kSecondSlider;
    all_disabled.bytes[kFirstWindow + 1202u] = 1u;
    all_disabled.bytes[kLastWindow + 1202u] = 1u;
    Check(Run(all_disabled, 208u) == 1u &&
          all_disabled.words.at(kMember + 24u) == 0u &&
          all_disabled.words.at(kMember + 28u) == kFirst &&
          all_disabled.redrawn ==
              std::vector<std::uint32_t>{kSecondChild},
          "all-disabled list keeps initial record pointer without highlight");

    Runtime no_first_record;
    no_first_record.words[kMember + 24u] = kSecondSlider;
    no_first_record.words[0x6000u] = 0u;
    Check(Run(no_first_record, 208u) == 1u &&
          no_first_record.words.at(kMember + 28u) == 0u &&
          no_first_record.redrawn ==
              std::vector<std::uint32_t>{kSecondChild},
          "empty record collection clears slider highlight only");

    Runtime down_record;
    Check(Run(down_record, 208u) == 1u &&
          down_record.words.at(kMember + 28u) == kLast,
          "down follows record +16 and skips disabled node");
    CheckHighlight(down_record, kFirstWindow, kLastWindow);

    for (std::uint32_t code : {203u, 205u}) {
        Runtime slider;
        slider.words[kMember + 24u] = kFirstSlider;
        Check(Run(slider, code) == 1u &&
              slider.slider_values ==
                  std::vector<std::pair<std::uint32_t, float>>{
                      {kFirstSlider, code == 203u ? 0.6f : 0.4f}} &&
              slider.redrawn.empty(),
              "left/right adjust active slider by 0.1");
        Runtime record;
        Check(Run(record, code) == 1u &&
              record.words.at(kMember + 28u) == kLast,
              "left/right follow +4/+8 and skip disabled node");
        CheckHighlight(record, kFirstWindow, kLastWindow);
    }

    Runtime activate;
    Check(Run(activate, 28u) == 1u &&
          activate.mouse_window == kFirstWindow &&
          activate.calls == std::vector<std::string>{
              "read", "read", "read", "mouse"},
          "confirm activates current record window");
    Runtime no_activate;
    no_activate.words[kMember + 28u] = 0u;
    Check(Run(no_activate, 28u) == 1u && no_activate.mouse_window == 0u,
          "confirm without record does nothing");

    Runtime unknown;
    Check(Run(unknown, 999u) == 0u &&
          unknown.calls == std::vector<std::string>{"read"},
          "unsupported key returns false");

    Runtime reset_record;
    reset_record.words[kMember + 16u] = 0x6008u;
    reset_record.words[0x6004u] = kLast;
    Check(pal4::SystemSettingNavigation::ResetHighlights(
          kMember, reset_record) == 1u &&
          reset_record.redrawn == std::vector<std::uint32_t>{
              kFirstWindow, kLastWindow, kFirstWindow} &&
          reset_record.bytes.at(kFirstWindow + 1201u) == 1u &&
          reset_record.bytes.at(kLastWindow + 1201u) == 0u,
          "record reset clears collection then highlights current record");

    Runtime reset_slider;
    reset_slider.words[kMember + 16u] = 0x6008u;
    reset_slider.words[0x6004u] = kLast;
    reset_slider.words[kMember + 24u] = kSecondSlider;
    Check(pal4::SystemSettingNavigation::ResetHighlights(
          kMember, reset_slider) == 1u &&
          reset_slider.redrawn == std::vector<std::uint32_t>{
              kFirstWindow, kLastWindow, kSecondChild} &&
          reset_slider.bytes.at(kSecondChild + 1201u) == 1u,
          "slider focus takes precedence over stored record");

    Runtime reset_none;
    reset_none.words[kMember + 16u] = 0x6004u;
    reset_none.words[kMember + 28u] = 0u;
    Check(pal4::SystemSettingNavigation::ResetHighlights(
          kMember, reset_none) == 1u &&
          reset_none.redrawn == std::vector<std::uint32_t>{kFirstWindow} &&
          reset_none.bytes.at(kFirstWindow + 1201u) == 0u,
          "no focus leaves every record unhighlighted");
}
