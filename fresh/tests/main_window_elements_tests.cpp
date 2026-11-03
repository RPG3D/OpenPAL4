#include "pal4/main_window_elements.h"

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

struct Runtime final : pal4::MainWindowElementsRuntimeStub {
    std::vector<std::string> names;
    std::vector<Write> writes;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> constructor_callbacks;
    std::vector<std::uint32_t> deleted_references;
    std::vector<std::uint32_t> counts;
    std::uint32_t null_callback_index = 0xffffffffu;
    std::uint32_t throw_lookup_index = 0xffffffffu;
    std::uint32_t throw_subscription_index = 0xffffffffu;
    std::uint32_t initialized{};
    std::uint32_t cleaned{};

    void InitializeNameBuffer() override { ++initialized; }
    std::string_view LayoutPrefix(
        const pal4::MainWindowState& state) override {
        Check(state.Read(0x14u) == 0x1234u,
              "layout prefix is stored at MainWindow +0x14");
        return "mainWindow/";
    }
    std::uint32_t FindWindow(std::string_view name) override {
        if (names.size() == throw_lookup_index)
            throw std::runtime_error("lookup");
        names.emplace_back(name);
        return 0x3000u + static_cast<std::uint32_t>(names.size());
    }
    std::uint32_t AllocateCallback(std::size_t bytes) override {
        Check(bytes == 0x18u, "callback allocation is 24 bytes");
        if (subscriptions.size() == null_callback_index) return 0u;
        return 0x5000u + static_cast<std::uint32_t>(subscriptions.size()) * 0x20u;
    }
    void ConstructCallback(std::uint32_t callback,
                           std::uint32_t handler,
                           std::uint32_t owner) override {
        constructor_callbacks.push_back(callback);
        WriteCallbackWord(callback + 8u, handler);
        WriteCallbackWord(callback + 0xcu, 0u);
        WriteCallbackWord(callback + 0x10u, owner);
        WriteCallbackWord(callback, pal4::MainWindowElements::kCallbackVtable);
    }
    void WriteCallbackWord(std::uint32_t address,
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
        deleted_references.push_back(reference);
    }
    std::uint32_t CleanupNameBuffer() override {
        ++cleaned;
        return 0xcafeu;
    }
};

pal4::MainWindowState MakeState() {
    pal4::MainWindowState state{};
    state.Write(0x14u, 0x1234u);
    return state;
}

void CheckNormal() {
    constexpr std::array<std::string_view, 9> names{{
        "BtnNewGame", "BtnLoadGame", "BtnExit", "BtnMovie",
        "BtnInfomation", "BtnAboutPal", "BtnQuest", "BtnPicture",
        "youxia"}};
    constexpr std::array<std::uint32_t, 8> handlers{{
        0x448930u, 0x448ac0u, 0x448a70u, 0x448970u,
        0x448a30u, 0x4489f0u, 0x448b00u, 0x4489b0u}};
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::MainWindowElements::Initialize(state, 0x900u, runtime) ==
                  0xcafeu && runtime.initialized == 1u &&
                  runtime.cleaned == 1u && runtime.names.size() == 9u &&
                  runtime.subscriptions.size() == 8u &&
                  runtime.constructor_callbacks.size() == 5u &&
                  runtime.writes.size() == 32u &&
                  runtime.deleted_references.size() == 7u,
          "all controls, callbacks and temporary references are handled");
    for (std::size_t i = 0; i < names.size(); ++i) {
        Check(runtime.names[i] == std::string("mainWindow/") +
                                      std::string(names[i]),
              "control lookup retains the original spelling and order");
        if (i == 8u) {
            Check(state.Read(0x64u) == 0x3009u,
                  "youxia is stored at +0x64 without subscription");
            break;
        }
        const auto callback = 0x5000u + static_cast<std::uint32_t>(i) * 0x20u;
        const auto base = i * 4u;
        Check(state.Read(0x40u + static_cast<std::uint32_t>(i) * 4u) ==
                  0x3001u + i &&
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
                      pal4::MainWindowElements::kCallbackVtable,
              "button slot, handler and callback field order match IDA");
        if (i < 5u)
            Check(runtime.constructor_callbacks[i] == callback,
                  "first five callbacks use constructor4");
    }
    Check(runtime.counts == std::vector<std::uint32_t>{
              0u, 1u, 0u, 0u, 0u, 0u, 0u, 0u} &&
              runtime.deleted_references[0] == 0x7010u &&
              runtime.deleted_references[1] == 0x7030u,
          "local subscription reference decrements and deletes at zero");
}

void CheckFailures() {
    auto state = MakeState();
    Runtime runtime;
    runtime.null_callback_index = 0u;
    Check(pal4::MainWindowElements::Initialize(state, 0x900u, runtime) ==
                  0xcafeu && runtime.subscriptions[0].callback == 0u &&
                  runtime.constructor_callbacks.size() == 4u &&
                  runtime.writes.size() == 28u,
          "null allocation still subscribes and continues");

    runtime = Runtime{};
    runtime.throw_lookup_index = 3u;
    try {
        (void)pal4::MainWindowElements::Initialize(state, 0x900u, runtime);
        Check(false, "lookup failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.cleaned == 1u && runtime.names.size() == 3u &&
                  runtime.subscriptions.size() == 3u,
              "lookup failure cleans the temporary name buffer");
    }

    runtime = Runtime{};
    runtime.throw_subscription_index = 6u;
    try {
        (void)pal4::MainWindowElements::Initialize(state, 0x900u, runtime);
        Check(false, "subscription failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.cleaned == 1u && runtime.names.size() == 7u &&
                  runtime.subscriptions.size() == 6u &&
                  state.Read(0x58u) == 0x3007u,
              "subscription failure preserves the lookup write and cleans names");
    }
}

}  // namespace

int main() {
    CheckNormal();
    CheckFailures();
    std::cout << "Main window elements tests passed\n";
}
