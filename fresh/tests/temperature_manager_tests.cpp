#include "pal4/temperature_manager.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <tuple>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ITemperatureManagerRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> colors;
    std::vector<std::array<std::uint32_t, 4>> floor_calls;
    std::uint32_t allocation = 0x1000;
    std::uint32_t allocated_bytes = 0;
    std::uint32_t singleton_manager = 0x4000;
    std::uint32_t registration = 0x3000;
    std::uint32_t registered_manager = 0;
    std::uint32_t registered_id = 0;
    std::uint32_t registered_factory = 0;
    std::uint32_t registered_destructor = 0;
    std::string registered_name;
    std::uint32_t event_key = 0;
    std::uint8_t event_state = 0;
    std::array<std::uint32_t, 4> players{};
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::int32_t> maxima;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::int32_t> values;
    std::vector<std::tuple<std::string, std::int32_t,
                           std::int32_t, std::uint32_t>> debug_lines;
    std::uint32_t frame = 0x6000;
    std::uint32_t found_window_name = 0;
    std::uint32_t positioned_frame = 0;
    std::array<float, 3> projected_world{};
    std::array<float, 2> converted_screen{};
    std::array<float, 2> floating_position{};
    std::string floating_text;
    std::string floating_style;
    float floating_alpha = 0;
    float floating_scale_x = 0;
    float floating_scale_y = 0;
    float floating_lifetime = 0;
    std::uint32_t floating_list = 0;
    std::uint32_t floating_object = 0;
    std::int32_t fpu_status = 0;
    float compared_timer = -1;
    float compared_threshold = -1;
    float window_temperature = -1;

    Runtime() { words[0x5014] = 0x778899; }

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("write byte");
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        allocated_bytes = size;
        calls.emplace_back("allocate");
        if (size == 72 && allocation != 0) return 0x7000;
        return allocation;
    }
    std::uint32_t SingletonManager() override {
        calls.emplace_back("singleton manager");
        return singleton_manager;
    }
    std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) override {
        registered_manager = manager;
        registered_name = name;
        registered_id = id;
        registered_factory = factory;
        registered_destructor = destructor;
        calls.emplace_back("register singleton");
        return registration;
    }
    std::uint32_t FloorAttribute() override {
        calls.emplace_back("floor attribute");
        return 0x5000;
    }
    std::uint32_t RgbaToPixel(std::uint32_t rgba,
                              std::uint32_t mode) override {
        Check(mode == 1280, "RenderWare pixel mode");
        colors.push_back(rgba);
        calls.emplace_back("rgba to pixel");
        return rgba ^ 0x01010101u;
    }
    std::int32_t AddFloorColor(
        std::uint32_t floor, std::uint32_t pixel,
        std::uint32_t context, std::uint32_t callback) override {
        floor_calls.push_back({floor, pixel, context, callback});
        calls.emplace_back("add floor color");
        return static_cast<std::int32_t>(floor_calls.size());
    }
    std::int32_t ProcessStateEvent(std::uint32_t key,
                                   std::uint8_t state) override {
        event_key = key;
        event_state = state;
        calls.emplace_back("state event");
        return 17;
    }
    std::int32_t Ftol(double value) override {
        calls.emplace_back("ftol");
        return static_cast<std::int32_t>(std::lrint(value));
    }
    std::int32_t FpuCompareStatus(float value,
                                   float threshold) override {
        compared_timer = value;
        compared_threshold = threshold;
        calls.emplace_back("fpu status");
        return fpu_status;
    }
    std::uint32_t PlayerByIndex(std::uint32_t index) override {
        calls.emplace_back("player");
        return players.at(index);
    }
    std::int32_t PlayerMaximum(std::uint32_t component,
                                std::uint32_t kind) override {
        calls.emplace_back("maximum");
        return maxima.at({component, kind});
    }
    std::int32_t PlayerValue(std::uint32_t component,
                              std::uint32_t property) override {
        calls.emplace_back("value");
        return values.at({component, property});
    }
    void SetPlayerValue(std::uint32_t component,
                        std::uint32_t property,
                        std::int32_t value) override {
        values[{component, property}] = value;
        calls.emplace_back("set value");
    }
    std::array<float, 2> ProjectToScreen(
        const std::array<float, 3>& position) override {
        projected_world = position;
        calls.emplace_back("project");
        return {position[0] + 1.0f, position[1] + 2.0f};
    }
    std::array<float, 2> ScreenToUi(
        const std::array<float, 2>& position) override {
        converted_screen = position;
        calls.emplace_back("ui coordinates");
        return {position[0] + 3.0f, position[1] + 4.0f};
    }
    std::uint32_t InitializeFloatingText(std::uint32_t memory) override {
        calls.emplace_back("floating text constructor");
        return memory;
    }
    void SetFloatingText(std::uint32_t object, std::string_view text,
                         std::string_view style, float alpha) override {
        floating_object = object;
        floating_text = text;
        floating_style = style;
        floating_alpha = alpha;
        calls.emplace_back("floating text");
    }
    void SetFloatingTextPosition(
        std::uint32_t, const std::array<float, 2>& position) override {
        floating_position = position;
        calls.emplace_back("floating position");
    }
    void SetFloatingTextScale(std::uint32_t, float horizontal,
                              float vertical) override {
        floating_scale_x = horizontal;
        floating_scale_y = vertical;
        calls.emplace_back("floating scale");
    }
    void SetFloatingTextLifetime(std::uint32_t,
                                  float lifetime) override {
        floating_lifetime = lifetime;
        calls.emplace_back("floating lifetime");
    }
    std::int32_t AppendFloatingText(std::uint32_t player_list,
                                    std::uint32_t text) override {
        floating_list = player_list;
        floating_object = text;
        calls.emplace_back("append floating text");
        return 41;
    }
    void AddDebugText(std::string_view text, std::int32_t x,
                      std::int32_t y, std::uint32_t color) override {
        debug_lines.emplace_back(text, x, y, color);
        calls.emplace_back("debug");
    }
    std::uint32_t FindTemperatureWindow(
        std::uint32_t name_pointer) override {
        found_window_name = name_pointer;
        calls.emplace_back("find window");
        return frame;
    }
    std::int32_t SetWindowPosition(std::uint32_t found_frame,
                                   float temperature) override {
        positioned_frame = found_frame;
        window_temperature = temperature;
        calls.emplace_back("window");
        return 23;
    }
};

void TestRegistrationAndCreation() {
    Runtime runtime;
    pal4::TemperatureManagerRegistration state{0x40, 0};
    Check(pal4::TemperatureManager::Register(state, runtime) == 0x3000 &&
              state.guard == 0x41 && state.instance == 0x3000 &&
              runtime.registered_manager == 0x4000 &&
              runtime.registered_name == "palTemperatureGame" &&
              runtime.registered_id == 1015 &&
              runtime.registered_factory == 0x5b45b0 &&
              runtime.registered_destructor == 0x5fc6c0 &&
              runtime.calls == std::vector<std::string>{
                  "singleton manager", "register singleton"},
          "guarded temperature singleton registration");
    Check(pal4::TemperatureManager::Register(state, runtime) == 0x3000 &&
              runtime.calls.size() == 2,
          "registered temperature singleton is cached");

    Runtime failed;
    failed.allocation = 0;
    Check(pal4::TemperatureManager::Create(failed) == 0 &&
              failed.allocated_bytes == 44 && failed.floor_calls.empty(),
          "temperature factory allocation failure");

    Runtime created;
    Check(pal4::TemperatureManager::Create(created) == 0x1000 &&
              created.allocated_bytes == 44 &&
              created.words.at(0x1014) == 4 &&
              created.words.at(0x1018) == 0x3f000000 &&
              created.words.at(0x101c) == 0x3d800000 &&
              created.words.at(0x1020) == 2 &&
              created.words.at(0x1024) == 0 &&
              created.bytes.at(0x1028) == 0 &&
              created.words.at(0x1010) == 0x778899,
          "temperature defaults and floor color copy");
    Check(created.colors == std::vector<std::uint32_t>{
                  0xffff99ccu, 0xffff6633u, 0xffff00ffu, 0xff0000ffu} &&
              created.floor_calls.size() == 5,
          "four RenderWare colors and five floor registrations");
    for (std::uint32_t i = 0; i < 5; ++i) {
        Check(created.floor_calls[i] ==
                  std::array<std::uint32_t, 4>{
                      0x5000, created.words.at(0x1000 + i * 4),
                      0x1000, 0x5b4900},
              "floor callback identity and ordered pixels");
    }
}

void TestStateAndFloorSelection() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x1000;
    static_cast<void>(pal4::TemperatureManager::Reset(object, runtime));
    runtime.calls.clear();
    Check(pal4::TemperatureManager::SetState(object, 0, runtime) == 17 &&
              runtime.event_key == 0x8b135c && runtime.event_state == 0 &&
              runtime.calls == std::vector<std::string>{
                  "state event", "write byte"},
          "disabled state broadcasts without reset");
    runtime.calls.clear();
    Check(pal4::TemperatureManager::SetState(object, 3, runtime) == 23 &&
              runtime.event_state == 3 && runtime.bytes.at(object + 40) == 3 &&
              runtime.window_temperature == 0.5f &&
              runtime.calls.front() == "state event" &&
              runtime.calls[runtime.calls.size() - 2] == "window" &&
              runtime.calls.back() == "write byte",
          "enabled state resets, updates window and preserves raw state byte");
    Check(pal4::TemperatureManager::FloorAttributeThunk(runtime) == 0x5000,
          "floor attribute thunk");
    for (std::uint32_t i = 0; i < 5; ++i) {
        runtime.words[object + i * 4] = 0xa00 + i;
    }
    runtime.words[object + 20] = 4;
    Check(pal4::TemperatureManager::SelectFloorColor(
              object, 0xa02, runtime) == 2 &&
              runtime.words.at(object + 20) == 2,
          "matching floor color selects its index");
    Check(pal4::TemperatureManager::SelectFloorColor(
              object, 0xbeef, runtime) == 5 &&
              runtime.words.at(object + 20) == 2,
          "unregistered floor color leaves state untouched");
    runtime.words[object] = 0xa02;
    Check(pal4::TemperatureManager::SelectFloorColor(
              object, 0xa02, runtime) == 0,
          "duplicate colors select first index");
}

void TestTemperatureUpdate() {
    constexpr std::uint32_t object = 0x1000;
    Runtime inactive;
    static_cast<void>(pal4::TemperatureManager::Reset(object, inactive));
    inactive.calls.clear();
    Check(pal4::TemperatureManager::Update(object, 1.0f, inactive) == 0 &&
              inactive.calls.empty(), "disabled temperature does no work");
    inactive.bytes[object + 40] = 1;
    Check(pal4::TemperatureManager::Update(object, 1.0f, inactive) == 4 &&
              inactive.debug_lines.size() == 3 &&
              std::find(inactive.calls.begin(), inactive.calls.end(),
                        "player") == inactive.calls.end() &&
              inactive.window_temperature < 0,
          "normal floor only emits diagnostics");

    struct Case {
        std::uint32_t state;
        float start;
        float elapsed;
        float expected;
        std::uint32_t band;
    };
    constexpr std::array<Case, 6> cases = {{
        {0, 0.5f, 8.0f, 0.0f, 0},
        {1, 0.5f, 1.0f, 0.4375f, 1},
        {2, 0.5f, 1.0f, 0.375f, 1},
        {2, 0.05f, 1.0f, 0.0f, 0},
        {3, 0.875f, 1.0f, 1.0f, 3},
        {3, 0.5f, 1.0f, 0.625f, 2},
    }};
    for (const auto& test : cases) {
        Runtime runtime;
        static_cast<void>(pal4::TemperatureManager::Reset(object, runtime));
        runtime.bytes[object + 40] = 1;
        runtime.words[object + 20] = test.state;
        runtime.words[object + 24] =
            std::bit_cast<std::uint32_t>(test.start);
        runtime.calls.clear();
        Check(pal4::TemperatureManager::Update(
                  object, test.elapsed, runtime) ==
                  (test.elapsed >= 3.0f ? 4 : 0) &&
                  std::abs(std::bit_cast<float>(
                      runtime.words.at(object + 24)) - test.expected) <
                      0.00001f &&
                  runtime.words.at(object + 32) == test.band &&
                  runtime.window_temperature == test.expected &&
                  runtime.found_window_name == 0x8b1360 &&
                  runtime.positioned_frame == 0x6000 &&
                  runtime.debug_lines.size() == 3 &&
                  runtime.calls.front() == "debug" &&
                  std::count(runtime.calls.begin(), runtime.calls.end(),
                             "player") ==
                      (test.elapsed >= 3.0f ? 4 : 0),
              "temperature change, clamp, band and call order");
    }
    for (const auto& threshold :
         std::array<std::pair<float, std::uint32_t>, 6>{{
             {0.124f, 0}, {0.125f, 1}, {0.499f, 1},
             {0.5f, 2}, {0.874f, 2}, {0.875f, 3}}}) {
        Runtime runtime;
        static_cast<void>(pal4::TemperatureManager::Reset(object, runtime));
        runtime.bytes[object + 40] = 1;
        runtime.words[object + 20] = 1;
        runtime.words[object + 24] =
            std::bit_cast<std::uint32_t>(threshold.first);
        static_cast<void>(pal4::TemperatureManager::Update(
            object, 0.0f, runtime));
        Check(runtime.words.at(object + 32) == threshold.second,
              "temperature band threshold");
    }
}

void TestPlayerTickAndPopup() {
    constexpr std::uint32_t object = 0x1000;
    Runtime runtime;
    static_cast<void>(pal4::TemperatureManager::Reset(object, runtime));
    runtime.words[object + 20] = 0;
    runtime.words[object + 36] = std::bit_cast<std::uint32_t>(2.5f);
    runtime.players = {0x2000, 0x3000, 0, 0x4000};
    runtime.words[0x2000 + 0xb08] = 1;
    runtime.words[0x3000 + 0xb08] = 0;
    runtime.words[0x4000 + 0xb08] = 1;
    for (const auto player : {0x2000u, 0x4000u}) {
        const auto component = player + 0x654;
        runtime.maxima[{component, 0}] = 100;
        runtime.maxima[{component, 2}] = player == 0x2000 ? 50 : 30;
        runtime.values[{component, 7}] = player == 0x2000 ? 5 : 90;
        runtime.values[{component, 9}] = player == 0x2000 ? 5 : 28;
        runtime.words[player + 172] = std::bit_cast<std::uint32_t>(1.0f);
        runtime.words[player + 176] = std::bit_cast<std::uint32_t>(2.0f);
        runtime.words[player + 180] = std::bit_cast<std::uint32_t>(3.0f);
        runtime.words[player + 244] = std::bit_cast<std::uint32_t>(4.0f);
    }
    runtime.calls.clear();
    Check(pal4::TemperatureManager::UpdatePlayers(object, 1.0f,
                                                   runtime) == 4 &&
              std::bit_cast<float>(runtime.words.at(object + 36)) == 0.5f &&
              runtime.values.at({0x2000 + 0x654, 7}) == 1 &&
              runtime.values.at({0x2000 + 0x654, 9}) == 0 &&
              runtime.values.at({0x4000 + 0x654, 7}) == 80 &&
              runtime.values.at({0x4000 + 0x654, 9}) == 25 &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "player") == 4 &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "append floating text") == 2 &&
              runtime.floating_text == "0" &&
              runtime.floating_style == "HPinc",
          "three-second tick adjusts eligible players and emits popups");
    runtime.calls.clear();
    runtime.fpu_status = 0x500;
    Check(pal4::TemperatureManager::UpdatePlayers(object, 0.1f,
                                                   runtime) == 0x500 &&
              runtime.compared_timer == 0.6f &&
              runtime.compared_threshold == 3.0f &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "player") == runtime.calls.end(),
          "sub-three-second timer does not visit players");

    Runtime warm;
    static_cast<void>(pal4::TemperatureManager::Reset(object, warm));
    warm.words[object + 20] = 2;
    warm.words[object + 36] = std::bit_cast<std::uint32_t>(3.0f);
    warm.players[0] = 0x2000;
    warm.words[0x2000 + 0xb08] = 1;
    for (const auto offset : {172u, 176u, 180u, 244u}) {
        warm.words[0x2000 + offset] =
            std::bit_cast<std::uint32_t>(1.0f);
    }
    const auto component = 0x2000u + 0x654u;
    warm.maxima[{component, 0}] = 100;
    warm.maxima[{component, 2}] = 30;
    warm.values[{component, 7}] = 99;
    warm.values[{component, 9}] = 29;
    Check(pal4::TemperatureManager::UpdatePlayers(object, 0.0f,
                                                   warm) == 4 &&
              warm.values.at({component, 7}) == 100 &&
              warm.values.at({component, 9}) == 30,
          "warming clamps health and mana to their maxima");

    Runtime popup;
    popup.words[0x2000 + 172] = std::bit_cast<std::uint32_t>(1.0f);
    popup.words[0x2000 + 176] = std::bit_cast<std::uint32_t>(2.0f);
    popup.words[0x2000 + 180] = std::bit_cast<std::uint32_t>(3.0f);
    popup.words[0x2000 + 244] = std::bit_cast<std::uint32_t>(4.0f);
    Check(pal4::TemperatureManager::EmitHealthPopup(0x2000, -7,
                                                     popup) == 41 &&
              popup.projected_world == std::array<float, 3>{1, 26, 3} &&
              popup.converted_screen == std::array<float, 2>{2, 28} &&
              popup.floating_position == std::array<float, 2>{5, 32} &&
              popup.floating_text == "-7" &&
              popup.floating_style == "RoleHPdec" &&
              popup.floating_alpha == 1.0f &&
              popup.floating_scale_x == 0.5f &&
              popup.floating_scale_y == 0.5f &&
              popup.floating_lifetime == 40.0f &&
              popup.floating_list == 0x2000 + 2728 &&
              popup.floating_object == 0x7000,
          "negative health popup world projection, style and animation");
    popup.allocation = 0;
    Check(pal4::TemperatureManager::EmitHealthPopup(0x2000, 3,
                                                     popup) == 0,
          "popup allocation failure returns zero");
}

void TestDiagnosticsAndWindow() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x1000;
    static_cast<void>(pal4::TemperatureManager::Reset(object, runtime));
    runtime.words[object + 20] = 2;
    runtime.words[object + 24] = std::bit_cast<std::uint32_t>(0.375f);
    runtime.words[object + 32] = 1;
    pal4::TemperatureManager::TraceTemperature(object, runtime);
    Check(runtime.debug_lines ==
              std::vector<std::tuple<std::string, std::int32_t,
                                     std::int32_t, std::uint32_t>>{
                  {"Temperature: 0.375000", 0, 50, 65280},
                  {"floorColour: Freeze", 0, 60, 65280},
                  {"TempRange:\tCold", 0, 70, 65280}},
          "temperature debug text and screen positions");
    runtime.frame = 0;
    Check(pal4::TemperatureManager::SyncWindow(object, runtime) == 0 &&
              runtime.found_window_name == 0x8b1360 &&
              runtime.positioned_frame == 0,
          "missing temperature window does not receive a position");
    runtime.frame = 0x6000;
    Check(pal4::TemperatureManager::SyncWindow(object, runtime) == 23 &&
              runtime.positioned_frame == 0x6000 &&
              runtime.window_temperature == 0.375f,
          "temperature window receives current temperature");
}

}  // namespace

int main() {
    try {
        TestRegistrationAndCreation();
        TestStateAndFloorSelection();
        TestTemperatureUpdate();
        TestPlayerTickAndPopup();
        TestDiagnosticsAndWindow();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
