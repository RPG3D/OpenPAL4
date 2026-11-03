#include "pal4/combat_hint_window2.h"

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
constexpr std::uint32_t kData = 0x6000u;
constexpr std::uint32_t kCallback = 0x7000u;

struct Runtime final : pal4::ICombatHintWindowRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kWindow + 36u, 0x8000u}, {kWindow + 52u, kData},
        {kWindow + 56u, 1u}, {kWindow + 60u, 16u}};
    std::unordered_map<std::uint32_t, std::uint8_t> bytes{
        {kData - 1u, 0u}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    bool text_allocation = true;
    bool callback_allocation = true;
    bool fail_lookup = false;
    std::uint32_t connection = 0x9000u;
    std::uint32_t subscribed_callback = 0xffffffffu;

    void LoadLayout(std::uint32_t, std::string_view) override {
        Check(false, "generic layout is unused");
    }
    void LoadCombatLayout(std::uint32_t window, std::string_view name,
                          std::uint32_t argument) override {
        Check(window == kWindow &&
                  name == pal4::CombatHintWindow2::kLayoutName &&
                  argument == 0u,
              "second combat hint layout");
        calls.emplace_back("layout");
    }
    void AppendPrefixToClock(std::uint32_t,
                             std::uint32_t) override {
        Check(false, "second hint does not append prefix to clock");
    }
    std::uint32_t DestroyBase(std::uint32_t window) override {
        Check(window == kWindow, "base destructor window");
        calls.emplace_back("destroy-base");
        return 0x1234u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.count(address) != 0u, "unexpected second hint read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        if (address == kWindow)
            calls.emplace_back("vtable");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(bytes.count(address) != 0u, "unexpected reference read");
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t flag) override {
        Check(record == kWindow + 48u && flag == 0u,
              "clock flag reset");
        calls.emplace_back("clock-reset");
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        Check(record == kWindow + 48u &&
                  length == pal4::CombatHintWindow2::kClockName.size() &&
                  mode == 1u,
              "clock allocation");
        calls.emplace_back("clock-allocate");
        if (!text_allocation) words[kWindow + 52u] = 0u;
        return text_allocation;
    }
    void CopyText(std::uint32_t destination,
                  std::string_view text) override {
        Check(destination == kData &&
                  text == pal4::CombatHintWindow2::kClockName,
              "second clock literal");
        calls.emplace_back("clock-copy");
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t length) override {
        Check(record == kWindow + 48u &&
                  length == pal4::CombatHintWindow2::kClockName.size(),
              "second clock length");
        calls.emplace_back("clock-size");
    }
    std::string GetNamePrefix(std::uint32_t field) override {
        Check(field == kWindow + 20u, "second hint name field");
        calls.emplace_back("prefix");
        return "Combathint2Window/";
    }
    std::uint32_t FindWindow(std::string_view name) override {
        calls.emplace_back("find-window");
        if (fail_lookup) throw std::runtime_error("lookup failure");
        if (name == "Combathint2Window/StaticPanel") return 0x8100u;
        if (name == "Combathint2Window/StaticText") return 0x8200u;
        Check(false, "unexpected second hint child");
        return 0u;
    }
    void SetAutoBreak(std::uint32_t, bool) override {
        Check(false, "static text has no auto-break setup");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 12u, "callback allocation size");
        calls.emplace_back("callback-allocate");
        return callback_allocation ? kCallback : 0u;
    }
    std::uint32_t Subscribe(std::uint32_t root, std::uint32_t event,
                            std::uint32_t callback) override {
        Check(root == 0x8000u &&
                  event == pal4::HintWindow::kZOrderChangedEvent,
              "second hint z-order event");
        subscribed_callback = callback;
        calls.emplace_back("subscribe");
        return connection;
    }
    void ReleaseConnection(std::uint32_t value) override {
        Check(value == connection, "subscription connection");
        calls.emplace_back("release-connection");
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free");
    }
};

void ConstructAndFailures() {
    Runtime runtime;
    Check(pal4::CombatHintWindow2::Construct(kWindow, runtime) == kWindow &&
              runtime.words.at(kWindow) ==
                  pal4::CombatHintWindow2::kVtable &&
              runtime.words.at(kWindow + 44u) == 0x8100u &&
              runtime.words.at(kWindow + 40u) == 0x8200u &&
              runtime.bytes.at(kWindow + 64u) == 1u &&
              runtime.words.at(kCallback) ==
                  pal4::CombatHintWindow2::kHandlerVtable &&
              runtime.words.at(kCallback + 4u) ==
                  pal4::CombatHintWindow::kOnZOrderChanged &&
              runtime.words.at(kCallback + 8u) == kWindow &&
              runtime.subscribed_callback == kCallback,
          "second hint binds static controls and callback");
    Check(runtime.calls == std::vector<std::string>{
              "layout", "clock-reset", "vtable", "clock-allocate",
              "clock-copy", "clock-size", "prefix", "find-window",
              "find-window", "callback-allocate", "subscribe",
              "release-connection"},
          "second hint constructor order");

    Runtime no_allocations;
    no_allocations.text_allocation = false;
    no_allocations.callback_allocation = false;
    no_allocations.connection = 0u;
    Check(pal4::CombatHintWindow2::Construct(kWindow, no_allocations) ==
                  kWindow &&
              no_allocations.subscribed_callback == 0u &&
              no_allocations.calls.back() == "subscribe",
          "second hint null allocation branches");

    Runtime failure;
    failure.fail_lookup = true;
    try {
        (void)pal4::CombatHintWindow2::Construct(kWindow, failure);
        Check(false, "lookup should throw");
    } catch (const std::runtime_error&) {}
    Check(failure.freed == std::vector<std::uint32_t>{kData - 1u} &&
              failure.calls.back() == "destroy-base",
          "second hint constructor unwinds clock and base");
}

void DestructAndDelete() {
    for (const std::uint8_t reference : {0u, 0xffu, 2u}) {
        Runtime runtime;
        runtime.bytes[kData - 1u] = reference;
        Check(pal4::CombatHintWindow2::Destruct(kWindow, runtime) ==
                  0x1234u &&
                  runtime.words.at(kWindow) ==
                      pal4::CombatHintWindow2::kVtable &&
                  runtime.words.at(kWindow + 52u) == 0u &&
                  runtime.words.at(kWindow + 56u) == 0u &&
                  runtime.words.at(kWindow + 60u) == 0u,
              "second hint destructor clears clock record");
        if (reference == 2u) {
            Check(runtime.freed.empty() &&
                      runtime.bytes.at(kData - 1u) == 1u,
                  "shared second clock decrements reference byte");
        } else {
            Check(runtime.freed == std::vector<std::uint32_t>{kData - 1u},
                  "unique second clock frees prefixed storage");
        }
    }

    for (const std::uint8_t flags : {0u, 1u, 2u, 3u}) {
        Runtime runtime;
        runtime.words[kWindow + 52u] = 0u;
        Check(pal4::CombatHintWindow2::Delete(
                  kWindow, flags, runtime) == kWindow &&
                  runtime.freed == ((flags & 1u) != 0u
                      ? std::vector<std::uint32_t>{kWindow}
                      : std::vector<std::uint32_t>{}),
              "deleting thunk frees only when low flag bit is set");
        Check(runtime.calls.front() == "vtable" &&
                  runtime.calls.at(1) == "destroy-base" &&
                  runtime.calls.size() == ((flags & 1u) != 0u ? 3u : 2u),
              "deleting thunk runs destructor before optional free");
    }
}

}  // namespace

int main() {
    try {
        ConstructAndFailures();
        DestructAndDelete();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
