#include "pal4/picture_view_window_elements.h"

#include <array>
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

struct Write {
    std::uint32_t address;
    std::uint32_t value;
};

struct Subscription {
    std::uint32_t window;
    std::uint32_t callback;
};

struct Runtime final : pal4::PictureViewWindowElementsRuntimeStub {
    std::vector<std::string> names;
    std::vector<std::string> calls;
    std::vector<Write> writes;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> counts;
    std::vector<std::uint32_t> deleted;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> mouse_bindings;
    std::uint32_t null_callback_index = 0xffffffffu;
    std::uint32_t throw_lookup_index = 0xffffffffu;

    std::uint32_t FindWindow(std::string_view name) override {
        if (names.size() == throw_lookup_index)
            throw std::runtime_error("lookup");
        names.emplace_back(name);
        calls.emplace_back("find");
        return 0x3001u + static_cast<std::uint32_t>(names.size() - 1u);
    }
    void SetImageFormatting(std::uint32_t image,
                            std::uint32_t horizontal,
                            std::uint32_t vertical) override {
        Check(image == 0x3002u && horizontal == 2u && vertical == 2u,
              "StaticImage uses horizontal and vertical format two");
        calls.emplace_back("format");
    }
    std::uint32_t AllocateCallback(std::size_t bytes) override {
        Check(bytes == 0x18u, "each button callback allocates 24 bytes");
        calls.emplace_back("allocate");
        if (subscriptions.size() == null_callback_index) return 0u;
        return 0x5000u + static_cast<std::uint32_t>(subscriptions.size()) * 0x20u;
    }
    void WriteCallbackWord(std::uint32_t address,
                           std::uint32_t value) override {
        writes.push_back({address, value});
    }
    std::uint32_t SubscribeClicked(std::uint32_t window,
                                   std::uint32_t callback) override {
        subscriptions.push_back({window, callback});
        calls.emplace_back("subscribe");
        return 0x7000u +
               static_cast<std::uint32_t>(subscriptions.size()) * 0x10u;
    }
    std::uint32_t ReferenceCount(std::uint32_t reference) override {
        return reference == 0x7020u ? 2u : 1u;
    }
    void SetReferenceCount(std::uint32_t,
                           std::uint32_t count) override {
        counts.push_back(count);
    }
    void DeleteReference(std::uint32_t reference,
                         std::uint32_t flags) override {
        Check(flags == 1u, "zero reference uses deleting flag one");
        deleted.push_back(reference);
    }
    void BindMouseEvents(std::uint32_t member,
                         std::uint32_t window) override {
        mouse_bindings.emplace_back(member, window);
        calls.emplace_back("bind-mouse");
    }
};

}  // namespace

int main() {
    constexpr std::array<std::string_view, 4> names{{
        "PictureViewWindow/BtnClose", "PictureViewWindow/Image",
        "PictureViewWindow/BtnLeft", "PictureViewWindow/BtnRight"}};
    constexpr std::array<std::uint32_t, 3> handlers{{
        0x44ad50u, 0x44aeb0u, 0x44aef0u}};
    constexpr std::array<std::uint32_t, 3> windows{{
        0x3001u, 0x3003u, 0x3004u}};
    pal4::PictureViewWindowState state{};
    Runtime runtime;
    pal4::PictureViewWindowElements::Initialize(state, 0x2000u, runtime);
    Check(runtime.names.size() == 4u &&
              state.Read(0x40u) == 0x3001u &&
              state.Read(0x4cu) == 0x3002u &&
              state.Read(0x44u) == 0x3003u &&
              state.Read(0x48u) == 0x3004u &&
              runtime.subscriptions.size() == 3u &&
              runtime.writes.size() == 12u &&
              runtime.counts == std::vector<std::uint32_t>{0u, 1u, 0u} &&
              runtime.deleted == std::vector<std::uint32_t>{
                  0x7010u, 0x7030u} &&
              runtime.mouse_bindings ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x2028u, 0x3003u}, {0x2028u, 0x3004u}}),
          "four controls and three click callbacks use observed slots");
    for (std::size_t i = 0; i < names.size(); ++i)
        Check(runtime.names[i] == names[i],
              "control lookup order and full layout names are exact");
    for (std::size_t i = 0; i < handlers.size(); ++i) {
        const auto callback = 0x5000u + static_cast<std::uint32_t>(i) * 0x20u;
        const auto base = i * 4u;
        Check(runtime.subscriptions[i].window == windows[i] &&
                  runtime.subscriptions[i].callback == callback &&
                  runtime.writes[base].address == callback + 8u &&
                  runtime.writes[base].value == handlers[i] &&
                  runtime.writes[base + 1u].address == callback + 0xcu &&
                  runtime.writes[base + 1u].value == 0u &&
                  runtime.writes[base + 2u].address == callback + 0x10u &&
                  runtime.writes[base + 2u].value == 0x2000u &&
                  runtime.writes[base + 3u].address == callback &&
                  runtime.writes[base + 3u].value ==
                      pal4::PictureViewWindowElements::kCallbackVtable,
              "each callback preserves handler, zero, owner and vtable order");
    }
    Check(runtime.calls == std::vector<std::string>{
              "find", "allocate", "subscribe", "find", "format",
              "find", "allocate", "subscribe", "find", "allocate",
              "subscribe", "bind-mouse", "bind-mouse"},
          "image formatting and mouse binding retain native order");

    runtime = Runtime{};
    runtime.null_callback_index = 1u;
    pal4::PictureViewWindowElements::Initialize(state, 0x2000u, runtime);
    Check(runtime.subscriptions[1].callback == 0u &&
              runtime.writes.size() == 8u,
          "null middle callback still subscribes left button");

    runtime = Runtime{};
    runtime.throw_lookup_index = 3u;
    try {
        pal4::PictureViewWindowElements::Initialize(state, 0x2000u, runtime);
        Check(false, "right lookup failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.names.size() == 3u &&
                  runtime.subscriptions.size() == 2u &&
                  runtime.mouse_bindings.empty(),
              "right lookup failure occurs after left subscription");
    }
    std::cout << "Picture view window elements tests passed\n";
}
