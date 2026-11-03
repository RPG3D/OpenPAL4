#include "pal4/combat_hint_window.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kWindow = 0x5000u;
constexpr std::uint32_t kClockData = 0x6000u;
constexpr std::uint32_t kCallback = 0x7000u;

struct Runtime final : pal4::ICombatHintWindowRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kWindow + 36u, 0x8000u},
        {kWindow + 52u, kClockData},
        {kWindow + 56u, 1u},
        {kWindow + 60u, 16u}};
    std::unordered_map<std::uint32_t, std::uint8_t> bytes{
        {kClockData - 1u, 0u}};
    std::vector<std::string> calls;
    bool text_allocation = true;
    bool callback_allocation = true;
    bool fail_window_lookup = false;
    std::uint32_t connection = 0x9000u;
    std::uint32_t subscribed_callback = 0xffffffffu;
    std::uint32_t freed = 0u;
    std::uint32_t text_size = 0u;

    void LoadLayout(std::uint32_t, std::string_view) override {
        Check(false, "generic layout path is unused");
    }
    void LoadCombatLayout(std::uint32_t window, std::string_view name,
                          std::uint32_t argument) override {
        Check(window == kWindow &&
                  name == pal4::CombatHintWindow::kLayoutName &&
                  argument == 0x1234u,
              "combat layout arguments");
        calls.emplace_back("layout");
    }
    void AppendPrefixToClock(std::uint32_t clock,
                             std::uint32_t prefix) override {
        Check(clock == kWindow + 48u && prefix == kWindow + 20u,
              "clock target and prefix source");
        calls.emplace_back("append-prefix");
    }
    std::uint32_t DestroyBase(std::uint32_t window) override {
        Check(window == kWindow, "base destructor window");
        calls.emplace_back("destroy-base");
        return window;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.count(address) != 0u, "unexpected hint word read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        if (address == kWindow)
            calls.emplace_back("vtable");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(bytes.count(address) != 0u, "unexpected hint byte read");
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t flag) override {
        Check(record == kWindow + 48u && flag == 0u,
              "clock error flag reset");
        calls.emplace_back("clock-reset");
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        Check(record == kWindow + 48u &&
                  length == pal4::CombatHintWindow::kClockName.size() &&
                  mode == 1u,
              "clock allocation arguments");
        calls.emplace_back("clock-allocate");
        if (!text_allocation)
            words[kWindow + 52u] = 0u;
        return text_allocation;
    }
    void CopyText(std::uint32_t destination,
                  std::string_view text) override {
        Check(destination == kClockData &&
                  text == pal4::CombatHintWindow::kClockName,
              "clock literal and destination");
        calls.emplace_back("clock-copy");
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t length) override {
        Check(record == kWindow + 48u, "clock size record");
        text_size = length;
        calls.emplace_back("clock-size");
    }
    std::string GetNamePrefix(std::uint32_t field) override {
        Check(field == kWindow + 20u, "combat hint name field");
        calls.emplace_back("prefix");
        return "CombathintWindow/";
    }
    std::uint32_t FindWindow(std::string_view name) override {
        calls.emplace_back("find-window");
        if (fail_window_lookup)
            throw std::runtime_error("window lookup failure");
        if (name == "CombathintWindow/ImgFrame") return 0x8100u;
        if (name == "CombathintWindow/EditMessage") return 0x8200u;
        Check(false, "unexpected child name");
        return 0u;
    }
    void SetAutoBreak(std::uint32_t window,
                      bool enabled) override {
        Check(window == 0x8200u && !enabled, "edit auto-break disabled");
        calls.emplace_back("auto-break");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 12u, "handler allocation size");
        calls.emplace_back("handler-allocate");
        return callback_allocation ? kCallback : 0u;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            std::uint32_t event,
                            std::uint32_t handler) override {
        Check(window == 0x8000u &&
                  event == pal4::HintWindow::kZOrderChangedEvent,
              "root z-order subscription");
        subscribed_callback = handler;
        calls.emplace_back("subscribe");
        return connection;
    }
    void ReleaseConnection(std::uint32_t value) override {
        Check(value == connection, "returned connection released");
        calls.emplace_back("release-connection");
    }
    void Free(std::uint32_t address) override {
        freed = address;
        calls.emplace_back("free");
    }
};

void ConstructSuccess() {
    Runtime runtime;
    Check(pal4::CombatHintWindow::Construct(
              kWindow, 0x1234u, runtime) == kWindow &&
              runtime.words.at(kWindow) == pal4::CombatHintWindow::kVtable &&
              runtime.words.at(kWindow + 44u) == 0x8100u &&
              runtime.words.at(kWindow + 40u) == 0x8200u &&
              runtime.bytes.at(kWindow + 64u) == 1u &&
              runtime.text_size ==
                  pal4::CombatHintWindow::kClockName.size() &&
              runtime.words.at(kCallback) ==
                  pal4::CombatHintWindow::kHandlerVtable &&
              runtime.words.at(kCallback + 4u) ==
                  pal4::CombatHintWindow::kOnZOrderChanged &&
              runtime.words.at(kCallback + 8u) == kWindow &&
              runtime.subscribed_callback == kCallback,
          "combat hint binds clock, child windows and z-order handler");
    Check(runtime.calls == std::vector<std::string>{
              "layout", "clock-reset", "vtable", "clock-allocate",
              "clock-copy", "clock-size", "append-prefix", "prefix",
              "find-window", "find-window", "auto-break",
              "handler-allocate", "subscribe", "release-connection"},
          "constructor preserves observed stage order");
}

void ConstructAllocationBranchesAndUnwind() {
    Runtime no_allocations;
    no_allocations.text_allocation = false;
    no_allocations.callback_allocation = false;
    no_allocations.connection = 0u;
    Check(pal4::CombatHintWindow::Construct(
              kWindow, 0x1234u, no_allocations) == kWindow &&
              no_allocations.text_size == 0u &&
              no_allocations.subscribed_callback == 0u &&
              no_allocations.bytes.at(kWindow + 64u) == 1u,
          "allocation failures still set clock flag and subscribe");
    Check(no_allocations.calls.back() == "subscribe",
          "null subscription has no connection release");

    Runtime failure;
    failure.fail_window_lookup = true;
    try {
        (void)pal4::CombatHintWindow::Construct(
            kWindow, 0x1234u, failure);
        Check(false, "lookup must throw");
    } catch (const std::runtime_error&) {}
    Check(failure.freed == kClockData - 1u &&
              failure.words.at(kWindow + 52u) == 0u &&
              failure.words.at(kWindow + 56u) == 0u &&
              failure.words.at(kWindow + 60u) == 0u &&
              failure.calls.back() == "destroy-base",
          "constructor failure releases clock before base destruction");
}

void DestructReferenceCases() {
    for (const std::uint8_t reference : {0u, 0xffu, 2u}) {
        Runtime runtime;
        runtime.bytes[kClockData - 1u] = reference;
        Check(pal4::CombatHintWindow::Destruct(kWindow, runtime) ==
                  kWindow &&
                  runtime.words.at(kWindow) ==
                      pal4::CombatHintWindow::kVtable &&
                  runtime.words.at(kWindow + 52u) == 0u &&
                  runtime.words.at(kWindow + 56u) == 0u &&
                  runtime.words.at(kWindow + 60u) == 0u,
              "destructor clears string record and returns base result");
        if (reference == 2u) {
            Check(runtime.freed == 0u &&
                      runtime.bytes.at(kClockData - 1u) == 1u,
                  "shared clock storage decrements reference byte");
        } else {
            Check(runtime.freed == kClockData - 1u,
                  "unshared clock storage is freed");
        }
    }
    Runtime empty;
    empty.words[kWindow + 52u] = 0u;
    Check(pal4::CombatHintWindow::Destruct(kWindow, empty) == kWindow &&
              empty.freed == 0u,
          "null clock storage skips free");
}

}  // namespace

int main() {
    try {
        ConstructSuccess();
        ConstructAllocationBranchesAndUnwind();
        DestructReferenceCases();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
