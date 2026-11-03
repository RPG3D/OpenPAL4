#include "pal4/system_setting_frame.h"
#include "pal4/system_setting_interface.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct InterfaceRuntime final : pal4::ISystemSettingInterfaceRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::string, std::uint32_t> windows;
    std::map<std::uint32_t, std::string> names;
    std::vector<std::string> lookup_order;
    std::vector<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>> subscriptions;
    std::vector<std::pair<std::uint32_t, std::string>> text;
    std::vector<std::uint32_t> records;
    std::vector<std::uint32_t> mouse_targets;
    std::vector<std::uint32_t> slider_maximums;
    std::vector<std::string> list_calls;
    std::uint32_t next_window = 0x10000u;
    std::uint32_t next_callback = 0x900000u;
    std::uint32_t columns = 3u;
    std::uint32_t end_count = 0u;
    bool fail_lookup = false;
    bool top = false;
    bool mask_enabled = true;
    std::uint32_t formatting = 0u;
    std::pair<std::uint32_t, std::uint32_t> copied_text{};

    InterfaceRuntime() {
        words[pal4::SystemSettingInterface::kClickedEvent] = 0xa1u;
        words[pal4::SystemSettingInterface::kHoverEvent] = 0xa2u;
        words[pal4::SystemSettingInterface::kMouseClickEvent] = 0xa3u;
        words[pal4::SystemSettingInterface::kMouseButtonUpEvent] = 0xa4u;
        words[pal4::SystemSettingInterface::kValueChangedEvent] = 0xa5u;
    }
    std::uint32_t BeginName(std::uint32_t frame) override {
        Check(frame == 0x4000u, "name begins with frame");
        return 0x7000u;
    }
    std::uint32_t ResetName(std::uint32_t buffer, std::uint32_t frame,
                            std::string_view suffix) override {
        Check(buffer == 0x7000u && frame == 0x4000u, "name reuse");
        names[0x7000u] = std::string(suffix);
        return 0x7000u;
    }
    std::uint32_t EndName(std::uint32_t buffer) override {
        Check(buffer == 0x7000u, "name cleanup buffer");
        ++end_count;
        return 0x55u;
    }
    std::uint32_t FindWindow(std::uint32_t name) override {
        Check(name == 0x7000u, "lookup name pointer");
        if (fail_lookup) throw std::runtime_error("find");
        const auto suffix = names.at(name);
        lookup_order.push_back(suffix);
        const auto address = next_window;
        next_window += 0x1000u;
        windows[suffix] = address;
        if (suffix == "ScroolAudio1" || suffix == "ScroolAudio2") {
            words[address + 1212u] = address + 0x800u;
        }
        return address;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "callback size");
        const auto result = next_callback;
        next_callback += 0x100u;
        return result;
    }
    std::uint32_t Subscribe(std::uint32_t window, std::uint32_t event,
                            std::uint32_t callback) override {
        subscriptions.emplace_back(window, event, callback);
        return 0x6000u;
    }
    void ReleaseConnection(std::uint32_t connection) override {
        Check(connection == 0x6000u, "connection release");
    }
    std::uint32_t TriggerText(std::uint32_t trigger_id) override {
        Check(trigger_id >= 205u && trigger_id < 215u, "trigger range");
        return trigger_id;
    }
    std::string_view ToUtf8(std::uint32_t value) override {
        static constexpr std::string_view translated = "translated";
        Check(value >= 205u && value < 215u, "translation input");
        return translated;
    }
    void SetText(std::uint32_t window, std::string_view value) override {
        text.emplace_back(window, value);
    }
    void CopyText(std::uint32_t target, std::uint32_t source) override {
        copied_text = {target, source};
    }
    void SetAlwaysOnTop(std::uint32_t, bool enabled) override { top = enabled; }
    void SetEnabled(std::uint32_t, bool enabled) override { mask_enabled = enabled; }
    void SetHorizontalFormatting(std::uint32_t, std::uint32_t value) override {
        formatting = value;
    }
    void ResetList(std::uint32_t) override { list_calls.emplace_back("reset"); }
    std::uint32_t ColumnCount(std::uint32_t) override {
        list_calls.emplace_back("count");
        return columns;
    }
    void RemoveColumn(std::uint32_t, std::uint32_t index) override {
        Check(index == 0u && columns > 0u, "remove first column");
        list_calls.emplace_back("remove");
        --columns;
    }
    void SetSortDirection(std::uint32_t, std::uint32_t direction) override {
        Check(direction == 0u, "sort direction");
        list_calls.emplace_back("sort");
    }
    void SetUserColumnSizing(std::uint32_t, bool enabled) override {
        Check(!enabled, "column sizing disabled");
        list_calls.emplace_back("sizing");
    }
    void SetUserColumnDragging(std::uint32_t, bool enabled) override {
        Check(!enabled, "column dragging disabled");
        list_calls.emplace_back("dragging");
    }
    void SetUserSortControl(std::uint32_t, bool enabled) override {
        Check(!enabled, "sort control disabled");
        list_calls.emplace_back("control");
    }
    void SetSliderMaximum(std::uint32_t window, float value) override {
        Check(value == 1.0f, "slider maximum");
        slider_maximums.push_back(window);
    }
    void SetupMouseHandlers(std::uint32_t member,
                            std::uint32_t window) override {
        Check(member == 0x4028u, "mouse handler member");
        mouse_targets.push_back(window);
    }
    void AppendRecordPointer(std::uint32_t collection,
                             std::uint32_t record) override {
        Check(collection == 0x4048u, "record collection offset");
        records.push_back(record);
    }
};

struct FrameRuntime final : pal4::ISystemSettingFrameRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    bool legacy_setup = false;
    std::uint32_t ReadWord(std::uint32_t address) override { return words[address]; }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x1fcu, "frame size");
        return 0x4000u;
    }
    void Free(std::uint32_t) override {}
    std::uint32_t FrameManager() override { return 0u; }
    std::uint32_t ProcessEvent(std::uint32_t, std::uint32_t,
                               std::uint32_t) override { return 0u; }
    void LoadLayout(std::uint32_t, std::uint32_t, std::uint32_t,
                    std::uint32_t) override {}
    void ConstructMember(std::uint32_t) override {}
    void CleanupMember(std::uint32_t) override {}
    void SetupInterface(std::uint32_t) override { legacy_setup = true; }
    std::uint32_t DestroyDialog(std::uint32_t) override { return 0u; }
};

void CheckInterface(InterfaceRuntime& runtime) {
    constexpr auto frame = 0x4000u;
    Check(runtime.lookup_order.size() == 50u, "all 50 named controls looked up");
    Check(runtime.lookup_order.front() == "SettingWindow0" &&
          runtime.lookup_order[3] == "SaveWindow0" &&
          runtime.lookup_order[6] == "BtnData0" &&
          runtime.lookup_order.back() == "StaticInfo14", "lookup order");
    Check(runtime.text.size() == 10u, "all data button texts assigned");
    for (std::uint32_t i = 0; i < 10u; ++i) {
        const auto button = runtime.windows.at("BtnData" + std::to_string(i));
        Check(runtime.words.at(frame + 136u + 4u * i) == button &&
              runtime.words.at(button + 280u) == i &&
              runtime.text[i] == std::pair{button, std::string("translated")},
              "data button binding");
    }
    Check(runtime.top && !runtime.mask_enabled && runtime.formatting == 2u &&
          runtime.copied_text == std::pair{runtime.windows.at("StaticDataMask"),
                                          runtime.windows.at("BtnData0")},
          "mask and label state");
    Check(runtime.columns == 0u && runtime.list_calls ==
          std::vector<std::string>{"reset", "count", "remove", "count",
                                   "remove", "count", "remove", "count",
                                   "sort", "sizing", "dragging", "control"},
          "list reset and column configuration");
    Check(runtime.subscriptions.size() == 30u, "30 event subscriptions");
    constexpr std::array<std::uint32_t, 30> functions = {
        0x481980u, 0x481980u, 0x481980u, 0x481980u, 0x481980u,
        0x481980u, 0x481980u, 0x481980u, 0x481980u, 0x481980u,
        0x481010u, 0x480de0u, 0x4812e0u, 0x481510u,
        0x481e10u, 0x4819a0u,
        0x481b80u, 0x481b80u, 0x481b80u,
        0x481bd0u, 0x481bd0u,
        0x481c20u, 0x481c20u, 0x481c20u,
        0x481c70u, 0x481c70u,
        0x481df0u, 0x481df0u, 0x481cc0u, 0x481d20u};
    for (std::size_t i = 0; i < functions.size(); ++i) {
        const auto [window, event, callback] = runtime.subscriptions[i];
        Check(window != 0u && callback != 0u &&
              runtime.words.at(callback) ==
                  pal4::SystemSettingInterface::kCallbackVtable &&
              runtime.words.at(callback + 8u) == functions[i] &&
              runtime.words.at(callback + 12u) == 0u &&
              runtime.words.at(callback + 16u) == frame,
              "callback layout and target");
        const auto expected_event = i == 14 ? 0xa2u : i == 15 ? 0xa3u :
                                    i == 26 || i == 27 ? 0xa4u :
                                    i >= 28 ? 0xa5u : 0xa1u;
        Check(event == expected_event, "subscription event");
    }
    constexpr std::uint32_t offsets[] = {240u, 260u, 280u, 300u, 320u,
                                         340u, 360u, 380u, 400u, 420u};
    for (std::size_t i = 0; i < 10u; ++i) {
        Check(runtime.records[i] == frame + offsets[i], "record insertion order");
    }
    Check(runtime.records.size() == 10u, "ten record pointers");
    constexpr std::uint32_t links[][2] = {
        {256, 340}, {248, 260}, {284, 260}, {372, 260},
        {264, 240}, {288, 300}, {324, 300}, {364, 340},
        {296, 380}, {368, 380}, {404, 380}, {432, 380},
        {308, 320}, {336, 420}, {412, 300}, {268, 280},
        {276, 360}, {304, 280}, {316, 400}, {352, 240},
        {348, 360}, {392, 280}, {384, 360}, {388, 400},
        {408, 420}, {424, 400}};
    for (const auto& link : links) {
        Check(runtime.words.at(frame + link[0]) == frame + link[1],
              "record cross-reference");
    }
    Check(runtime.words.at(runtime.windows.at("BtnAuto1") + 280u) == 1u &&
          runtime.words.at(runtime.windows.at("BtnAuto2") + 280u) == 0u &&
          runtime.words.at(runtime.windows.at("BtnAuto3") + 280u) == 2u,
          "auto button index permutation");
    Check(runtime.slider_maximums == std::vector<std::uint32_t>{
          runtime.windows.at("ScroolAudio1"),
          runtime.windows.at("ScroolAudio2")}, "slider maximum order");
    Check(runtime.mouse_targets == std::vector<std::uint32_t>{
          runtime.windows.at("BtnSaveData"),
          runtime.windows.at("BtnLoadData"),
          runtime.windows.at("BtnSystemSetting"),
          runtime.windows.at("BtnExit")}, "main button mouse handlers");
    Check(runtime.words.at(frame + 68u) == frame &&
          runtime.words.at(frame + 100u) == frame &&
          runtime.words.at(frame + 88u) == runtime.windows.at("ScroolAudio1"),
          "self and first slider references");
}

}  // namespace

int main() {
    InterfaceRuntime direct;
    Check(pal4::SystemSettingInterface::Initialize(0x4000u, direct) == 0x55u &&
          direct.end_count == 1u, "direct initialization and name release");
    CheckInterface(direct);

    InterfaceRuntime failure;
    failure.fail_lookup = true;
    bool caught = false;
    try {
        (void)pal4::SystemSettingInterface::Initialize(0x4000u, failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failure.end_count == 1u, "name release on lookup failure");

    FrameRuntime frame;
    InterfaceRuntime composed;
    Check(pal4::SystemSettingFrame::Create(0x5001u, frame, composed) == 0x4000u &&
          !frame.legacy_setup && composed.end_count == 1u &&
          frame.words.at(0x4000u) == pal4::SystemSettingFrame::kObjectVtable,
          "frame constructor composes interface initializer");
    CheckInterface(composed);
}
