#include "pal4/magic_window_panel.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}

struct Subscription {
    std::uint32_t window;
    pal4::MagicPanelEvent event;
    std::uint32_t callback;
    std::uint32_t handler;
};

struct Runtime final : pal4::MagicWindowPanelRuntimeStub {
    std::vector<std::string> names;
    std::vector<std::string> calls;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> released;
    std::unordered_map<std::string, std::uint32_t> windows;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> indices;
    std::uint32_t allocations{};
    bool fail_allocations{};
    std::uint32_t top_count{};
    std::uint32_t mouse_handler_count{};
    std::uint32_t black_button_count{};
    std::uint32_t clear_field_count{};
    std::uint32_t copy_hover_count{};
    std::uint32_t set_field_count{};
    std::uint32_t black_static_count{};

    std::uint32_t FindWindow(std::string_view name) override {
        const auto value = 0x10000u + static_cast<std::uint32_t>(names.size()) * 0x100u;
        names.emplace_back(name);
        windows.emplace(name, value);
        calls.push_back("find:" + std::string(name));
        return value;
    }
    void SetUserIndex(std::uint32_t window, std::uint32_t index) override {
        indices[window] = index;
        calls.push_back("index");
    }
    void SetAlwaysOnTop(std::uint32_t, bool enabled) override {
        Check(enabled, "always on top enabled");
        ++top_count;
        calls.push_back("top");
    }
    std::uint32_t AllocateCallback(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "24 byte callback");
        ++allocations;
        calls.push_back("allocate");
        return fail_allocations ? 0u : 0x80000u + allocations * 0x20u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write");
    }
    std::uint32_t Subscribe(std::uint32_t window, pal4::MagicPanelEvent event,
                            std::uint32_t callback) override {
        const auto handler = callback == 0 ? 0u : words.at(callback + 8u);
        subscriptions.push_back({window, event, callback, handler});
        calls.push_back("subscribe");
        return subscriptions.size() % 2u == 0u ?
            0u : 0x90000u + static_cast<std::uint32_t>(subscriptions.size());
    }
    void ReleaseReference(std::uint32_t reference) override {
        released.push_back(reference);
        calls.push_back("release");
    }
    void SetupMouseHandlers(std::uint32_t member, std::uint32_t) override {
        Check(member == 0x3028u, "member pointer passed to mouse setup");
        ++mouse_handler_count;
        calls.push_back("mouse-setup");
    }
    void SetButtonTextColorsBlack(std::uint32_t) override { ++black_button_count; }
    void ClearButtonField(std::uint32_t) override { ++clear_field_count; }
    void SetHoverTextColorFromNormal(std::uint32_t) override { ++copy_hover_count; }
    void SetButtonField(std::uint32_t, std::uint32_t value) override {
        Check(value == 0x3058u, "stunt button owner field");
        ++set_field_count;
    }
    void SetStaticTextColorsBlack(std::uint32_t) override { ++black_static_count; }
};

void CheckBindings(const pal4::MagicWindowPanelState& state, const Runtime& runtime) {
    constexpr std::array<std::string_view, 6> category_names{
        "BtnStunt", "BtnMagicWater", "BtnMagicFire", "BtnMagicThunder",
        "BtnMagicAir", "BtnMagicEarth"};
    for (std::uint32_t i = 0; i < 6u; ++i) {
        const auto index = std::to_string(i);
        Check(state.At(0xa8u + 4u * i) == runtime.windows.at("Magic/" + std::string(category_names[i])) &&
                  state.At(0xc0u + 4u * i) == runtime.windows.at("Magic/StaticMagicName" + index) &&
                  state.At(0xd8u + 4u * i) == runtime.windows.at("Magic/Magicpic" + index) &&
                  state.At(0xf0u + 4u * i) == runtime.windows.at("Magic/BtnMagicPoint" + index),
              "category and magic six-control binding offsets");
        Check(runtime.indices.at(state.At(0xa8u + 4u * i)) == i &&
                  runtime.indices.at(state.At(0xc0u + 4u * i)) == i &&
                  runtime.indices.at(state.At(0xf0u + 4u * i)) == i,
              "six-control user indices");
    }
    for (std::uint32_t i = 0; i < 7u; ++i) {
        const auto index = std::to_string(i);
        Check(state.At(0x108u + 4u * i) == runtime.windows.at("Magic/BtnStunt" + index) &&
                  state.At(0x124u + 4u * i) == runtime.windows.at("Magic/Stuntpic" + index) &&
                  runtime.indices.at(state.At(0x108u + 4u * i)) == i,
              "seven stunt binding offsets and indices");
    }
    constexpr std::array<std::pair<std::string_view, std::uint32_t>, 23> fixed{{
        {"StaticRoleName", 0x7cu}, {"BtnLeft", 0x80u}, {"BtnRight", 0x84u},
        {"EditInfo", 0x88u}, {"EditProp", 0x8cu}, {"PanelStunt", 0x164u},
        {"PanelMagic", 0x15cu}, {"ImageWuling", 0x160u},
        {"MagicInfoPanel", 0x158u}, {"EditMagicInfo", 0x168u},
        {"EditMagicProp", 0x16cu}, {"zia", 0x90u}, {"zib", 0x94u},
        {"zi1", 0x98u}, {"dianshuzi", 0x9cu}, {"dianshuzi1", 0xa0u},
        {"dianshuzi12", 0xa4u}, {"zuozi", 0x154u},
        {"PicWater", 0x140u}, {"PicFire", 0x144u},
        {"PicThurder", 0x148u}, {"PicWind", 0x14cu},
        {"PicEarth", 0x150u}}};
    for (const auto& [name, offset] : fixed)
        Check(state.At(offset) == runtime.windows.at("Magic/" + std::string(name)),
              "fixed control binding offset");
}

void TestInitialize() {
    pal4::MagicWindowPanelState state;
    Runtime runtime;
    pal4::MagicWindowPanel::Initialize(0x3000u, "Magic/", state, runtime);
    Check(runtime.names.size() == 61u && runtime.allocations == 34u &&
              runtime.subscriptions.size() == 34u && runtime.released.size() == 17u,
          "control, callback and reference counts");
    CheckBindings(state, runtime);
    Check(runtime.top_count == 8u && runtime.mouse_handler_count == 6u &&
              runtime.black_button_count == 6u && runtime.clear_field_count == 6u &&
              runtime.copy_hover_count == 6u && runtime.set_field_count == 7u &&
              runtime.black_static_count == 3u, "control property operations");
    Check(runtime.windows.find("Magic/ImgMagicBackground0") == runtime.windows.end(),
          "background name is built but not resolved in IDA");
    Check(runtime.calls[0] == "find:Magic/BtnStunt" &&
              runtime.calls[1] == "index" && runtime.calls[2] == "allocate" &&
              runtime.calls[7] == "subscribe" && runtime.calls[8] == "release" &&
              runtime.calls[9] == "mouse-setup", "first category binding sequence");

    constexpr std::array<std::uint32_t, 7> handlers{
        0x459ae0u, 0x459bb0u, 0x459780u, 0x459c00u,
        0x459bd0u, 0x4596e0u, 0x459730u};
    std::array<std::uint32_t, 7> observed{};
    for (const auto& subscription : runtime.subscriptions) {
        const auto it = std::find(handlers.begin(), handlers.end(), subscription.handler);
        Check(it != handlers.end(), "IDA-backed callback target");
        ++observed[static_cast<std::size_t>(it - handlers.begin())];
        Check(runtime.words.at(subscription.callback) == 0x843994u &&
                  runtime.words.at(subscription.callback + 0xcu) == 0u &&
                  runtime.words.at(subscription.callback + 0x10u) == 0x3000u,
              "callback vtable, parameter and owner");
    }
    Check(observed == std::array<std::uint32_t, 7>{6u, 6u, 6u, 7u, 7u, 1u, 1u},
          "handler registration cardinalities");
    Check(runtime.subscriptions[0].event == pal4::MagicPanelEvent::Clicked &&
              runtime.subscriptions[6].event == pal4::MagicPanelEvent::MouseMove &&
              runtime.subscriptions[7].event == pal4::MagicPanelEvent::Clicked &&
              runtime.subscriptions[16].event == pal4::MagicPanelEvent::MouseMove &&
              runtime.subscriptions[17].event == pal4::MagicPanelEvent::Clicked &&
              runtime.subscriptions[18].event == pal4::MagicPanelEvent::MouseClick &&
              runtime.subscriptions[19].event == pal4::MagicPanelEvent::MouseMove,
          "event kinds follow IDA control groups");
}

void TestNullCallbacks() {
    pal4::MagicWindowPanelState state;
    Runtime runtime;
    runtime.fail_allocations = true;
    pal4::MagicWindowPanel::Initialize(0x3000u, "Magic/", state, runtime);
    Check(runtime.allocations == 34u && runtime.words.empty() &&
              std::all_of(runtime.subscriptions.begin(), runtime.subscriptions.end(),
                          [](const Subscription& item) { return item.callback == 0u; }) &&
              runtime.released.size() == 17u,
          "null allocations still subscribe and release returned references");
}

}  // namespace

int main() { TestInitialize(); TestNullCallbacks(); }
