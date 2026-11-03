#include "pal4/pal_test_window_elements.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Subscription {
    std::uint32_t window;
    std::string event;
    std::uint32_t callback;
};

struct Runtime final : pal4::PalTestWindowElementsRuntimeStub {
    std::vector<std::string> names;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> deleted;
    std::vector<std::uint32_t> freed_callbacks;
    std::vector<std::uint32_t> mouse_windows;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t colour_calls{};
    std::uint32_t next_callback{0xa000u};
    std::uint32_t fail_lookup{0xffffffffu};
    bool null_callbacks{};
    bool fail_subscribe{};

    std::string ReadPrefix(std::uint32_t address) override {
        Check(address == 0x9000u, "layout prefix at +0x14");
        return "PalTestWindow/";
    }
    std::uint32_t FindWindow(std::string_view name) override {
        if (names.size() == fail_lookup) throw std::runtime_error("lookup");
        names.emplace_back(name);
        return 0x2000u + static_cast<std::uint32_t>(names.size()) * 0x100u;
    }
    std::uint32_t AllocateCallback(std::size_t bytes) override {
        Check(bytes == 0x18u, "event callback is 0x18 bytes");
        if (null_callbacks) return 0u;
        const auto address = next_callback;
        next_callback += 0x20u;
        return address;
    }
    void FreeCallback(std::uint32_t callback) override {
        freed_callbacks.push_back(callback);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            std::string_view event,
                            std::uint32_t callback) override {
        if (fail_subscribe) throw std::runtime_error("subscribe");
        subscriptions.push_back({window, std::string(event), callback});
        return 0xc000u + static_cast<std::uint32_t>(subscriptions.size()) * 4u;
    }
    std::uint32_t ReferenceCount(std::uint32_t) override { return 1u; }
    void SetReferenceCount(std::uint32_t,
                           std::uint32_t count) override {
        Check(count == 0u, "subscription local owner decremented");
    }
    void DeleteReference(std::uint32_t reference,
                         std::uint32_t flags) override {
        Check(flags == 1u, "zero reference deletes local owner");
        deleted.push_back(reference);
    }
    void SetAnswerTextColour(std::uint32_t window,
                             std::uint32_t state,
                             std::uint32_t argb) override {
        Check(window >= 0x2400u && window <= 0x2600u &&
                  state < 4u && argb == 0xff000000u,
              "all answer states receive opaque black text");
        ++colour_calls;
    }
    void SetupMouseEventHandlers(std::uint32_t member,
                                 std::uint32_t window) override {
        Check(member == 0x6028u, "category mouse handlers use member");
        mouse_windows.push_back(window);
    }
};

void CheckElements() {
    pal4::PalTestWindowState state{};
    state.Write(0x14u, 0x9000u);
    Runtime runtime;
    pal4::PalTestWindowElements::Initialize(state, 0x6000u, runtime);
    Check(runtime.names == std::vector<std::string>{
              "PalTestWindow/BtnClose", "PalTestWindow/EditQuestion",
              "PalTestWindow/PanelQuestion", "PalTestWindow/StaticAnswer1",
              "PalTestWindow/StaticAnswer2", "PalTestWindow/StaticAnswer3",
              "PalTestWindow/BtnOk", "PalTestWindow/BtnHistory",
              "PalTestWindow/BtnStory", "PalTestWindow/BtnWorld"},
          "ten windows resolved in IDA order");
    Check(state.Read(0x44u) == 0x2100u &&
                  state.Read(0x40u) == 0x2200u &&
                  state.Read(0x64u) == 0x2300u &&
                  state.Read(0x58u) == 0x2400u &&
                  state.Read(0x60u) == 0x2600u &&
                  state.Read(0x48u) == 0x2700u &&
                  state.Read(0x4cu) == 0x2800u &&
                  state.Read(0x54u) == 0x2a00u &&
                  state.Read(0x88u) == 0x6000u,
          "control slots and owner pointer match object layout");
    Check(runtime.colour_calls == 12u &&
                  runtime.subscriptions.size() == 8u &&
                  runtime.deleted.size() == 8u &&
                  runtime.mouse_windows == std::vector<std::uint32_t>{
                      0x2800u, 0x2900u, 0x2a00u},
          "answer colours, subscriptions and category mouse handlers");
    constexpr std::uint32_t handlers[]{0x44a150u, 0x44e4e0u,
                                        0x44e4e0u, 0x44e4e0u,
                                        0x44e480u, 0x44e460u,
                                        0x44e460u, 0x44e460u};
    for (std::uint32_t i = 0u; i < 8u; ++i) {
        const auto& sub = runtime.subscriptions[i];
        Check(sub.callback == 0xa000u + i * 0x20u &&
                      runtime.words.at(sub.callback) == 0x84381cu &&
                      runtime.words.at(sub.callback + 8u) == handlers[i] &&
                      runtime.words.at(sub.callback + 0xcu) == 0u &&
                      runtime.words.at(sub.callback + 0x10u) == 0x6000u &&
                      sub.event == (i >= 1u && i <= 3u ?
                                        "MouseButtonUp" : "Clicked"),
              "callback fields and event types match IDA");
    }
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        Check(runtime.words.at(0x2400u + i * 0x100u + 0x118u) == i &&
                      runtime.words.at(0x2800u + i * 0x100u + 0x118u) == i,
              "answer and category indices stored at +0x118");
    }
}

void CheckFailureBoundaries() {
    pal4::PalTestWindowState state{};
    state.Write(0x14u, 0x9000u);
    Runtime runtime;
    runtime.null_callbacks = true;
    pal4::PalTestWindowElements::Initialize(state, 0x6000u, runtime);
    Check(runtime.subscriptions.size() == 8u &&
                  runtime.words.size() == 6u,
          "null callback still subscribes while only six indices are written");
    state = {};
    state.Write(0x14u, 0x9000u);
    runtime = Runtime{};
    runtime.fail_lookup = 3u;
    try {
        pal4::PalTestWindowElements::Initialize(state, 0x6000u, runtime);
        Check(false, "lookup failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.names.size() == 3u &&
                      runtime.subscriptions.size() == 1u &&
                      state.Read(0x88u) == 0u,
              "lookup failure leaves later controls uninitialized");
    }
    state = {};
    state.Write(0x14u, 0x9000u);
    runtime = Runtime{};
    runtime.fail_subscribe = true;
    try {
        pal4::PalTestWindowElements::Initialize(state, 0x6000u, runtime);
        Check(false, "subscription failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.freed_callbacks ==
                  std::vector<std::uint32_t>{0xa000u} &&
                  runtime.subscriptions.empty(),
              "failed subscription frees untransferred callback");
    }
}

}  // namespace

int main() {
    CheckElements();
    CheckFailureBoundaries();
    std::cout << "Pal test window elements tests passed\n";
}
