#include "pal4/movie_play_window_buttons.h"

#include <array>
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

struct Write {
    std::uint32_t address;
    std::uint32_t value;
};

struct Subscription {
    std::uint32_t window;
    std::uint32_t callback;
};

struct Runtime final : pal4::MoviePlayWindowButtonsRuntimeStub {
    std::vector<std::string> names;
    std::vector<Write> writes;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> counts;
    std::vector<std::uint32_t> deleted;
    std::uint32_t null_callback_index = 0xffffffffu;
    std::uint32_t throw_subscription_index = 0xffffffffu;

    std::uint32_t FindWindow(std::string_view name) override {
        names.emplace_back(name);
        return 0x3000u + static_cast<std::uint32_t>(names.size());
    }
    std::uint32_t AllocateCallback(std::size_t bytes) override {
        Check(bytes == 0x18u, "button callback allocates 24 bytes");
        if (subscriptions.size() == null_callback_index) return 0u;
        return 0x5000u + static_cast<std::uint32_t>(subscriptions.size()) * 0x20u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes.push_back({address, value});
    }
    std::uint32_t SubscribeClicked(std::uint32_t window,
                                   std::uint32_t callback) override {
        if (subscriptions.size() == throw_subscription_index)
            throw std::runtime_error("subscribe");
        subscriptions.push_back({window, callback});
        return 0x7000u + static_cast<std::uint32_t>(subscriptions.size()) * 0x10u;
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
};

}  // namespace

int main() {
    constexpr std::array<std::string_view, 3> names{{
        "MoviePlayWindow/BtnClose", "MoviePlayWindow/BtnPause",
        "MoviePlayWindow/BtnPlay"}};
    constexpr std::array<std::uint32_t, 3> offsets{{
        0x34u, 0x30u, 0x2cu}};
    constexpr std::array<std::uint32_t, 3> handlers{{
        0x449960u, 0x449970u, 0x449980u}};
    pal4::MoviePlayWindowState state{};
    Runtime runtime;
    pal4::MoviePlayWindowButtons::Initialize(state, 0x900u, runtime);
    Check(runtime.names.size() == 3u &&
              runtime.subscriptions.size() == 3u &&
              runtime.writes.size() == 12u &&
              runtime.deleted == std::vector<std::uint32_t>{
                  0x7010u, 0x7030u} &&
              runtime.counts == std::vector<std::uint32_t>{0u, 1u, 0u},
          "three buttons subscribe and local references are released");
    for (std::size_t i = 0; i < names.size(); ++i) {
        const auto callback = 0x5000u + static_cast<std::uint32_t>(i) * 0x20u;
        const auto base = i * 4u;
        Check(runtime.names[i] == names[i] &&
                  state.Read(offsets[i]) == 0x3001u + i &&
                  runtime.subscriptions[i].window == 0x3001u + i &&
                  runtime.subscriptions[i].callback == callback &&
                  runtime.writes[base].address == callback + 8u &&
                  runtime.writes[base].value == handlers[i] &&
                  runtime.writes[base + 1u].address == callback + 0xcu &&
                  runtime.writes[base + 1u].value == 0u &&
                  runtime.writes[base + 2u].address == callback + 0x10u &&
                  runtime.writes[base + 2u].value == 0x900u &&
                  runtime.writes[base + 3u].address == callback &&
                  runtime.writes[base + 3u].value ==
                      pal4::MoviePlayWindowButtons::kCallbackVtable,
              "control slot and callback write order follow IDA");
    }

    runtime = Runtime{};
    runtime.null_callback_index = 1u;
    pal4::MoviePlayWindowButtons::Initialize(state, 0x900u, runtime);
    Check(runtime.subscriptions[1].callback == 0u &&
              runtime.writes.size() == 8u,
          "null callback allocation still subscribes the pause button");

    runtime = Runtime{};
    runtime.throw_subscription_index = 1u;
    try {
        pal4::MoviePlayWindowButtons::Initialize(state, 0x900u, runtime);
        Check(false, "subscription failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.names.size() == 2u &&
                  runtime.subscriptions.size() == 1u &&
                  state.Read(0x30u) == 0x3002u,
              "failure follows second lookup and state write");
    }
    std::cout << "Movie play window buttons tests passed\n";
}
