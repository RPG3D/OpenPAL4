#include "pal4/shop_ui_controls.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

constexpr std::array<std::string_view, 10> kNames{
    "BtnBuyEquip", "BtnSellEquip", "BtnBuyProp", "BtnSellProp",
    "BtnBuyDescription", "BtnExit", "BtnTask", "BtnSmith",
    "PanelButton", "imgBottom"};
constexpr std::array<std::uint32_t, 8> kWindowOffsets{
    0x40u, 0x44u, 0x48u, 0x4cu, 0x50u, 0x5cu, 0x58u, 0x54u};
constexpr std::array<std::uint32_t, 8> kPositionOffsets{
    0x80u, 0x88u, 0x90u, 0x98u, 0xa0u, 0xb8u, 0xb0u, 0xa8u};
constexpr std::array<std::uint32_t, 8> kTriggers{
    147u, 148u, 145u, 146u, 149u, 30u, 189u, 258u};
constexpr std::array<std::uint32_t, 8> kFunctions{
    0x4864f0u, 0x486500u, 0x486510u, 0x486520u,
    0x486530u, 0x486540u, 0x486570u, 0x486730u};

struct Runtime final : pal4::IShopUiControlsRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kFrame + 0x18u, 0x7000u}, {0x8411b8u, 0xe000u}};
    std::vector<std::string> names;
    std::vector<std::uint32_t> triggers;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> bound;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> texts;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> released_callbacks;
    std::vector<std::uint32_t> released_connections;
    std::uint32_t prefix_reads = 0u;
    std::uint32_t empty_lookups = 0u;
    std::uint32_t callback_constructors = 0u;
    std::uint32_t name_cleanups = 0u;
    std::uint32_t allocations = 0u;
    std::uint32_t null_allocation_index = 99u;
    std::uint32_t bind_failure_index = 99u;
    bool fail_callback_constructor = false;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        if (address == kFrame + 0x18u) ++prefix_reads;
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t EmptyText() override {
        ++empty_lookups;
        return 0xbeefu;
    }
    std::uint32_t InitializeNameBuffer() override { return 0xc000u; }
    void FormatName(const std::uint32_t buffer,
                    const std::uint32_t prefix,
                    const std::string_view suffix) override {
        Check(buffer == 0xc000u &&
                  prefix == (words.at(kFrame + 0x18u) == 0u ?
                                 0xbeefu : 0x7000u),
              "each formatted name uses current frame prefix");
        names.emplace_back(suffix);
    }
    std::uint32_t FindWindow(const std::uint32_t buffer) override {
        Check(buffer == 0xc000u && !names.empty() &&
                  names.back() == kNames[names.size() - 1u],
              "ordered formatted control lookup");
        return 0x6000u + static_cast<std::uint32_t>(names.size() - 1u);
    }
    std::uint32_t CleanupNameBuffer(const std::uint32_t buffer) override {
        Check(buffer == 0xc000u, "formatted-name buffer cleanup");
        ++name_cleanups;
        return 0xfaceu;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 0x18u, "24-byte shop callback allocation");
        const auto index = allocations++;
        return index == null_allocation_index ?
                   0u : 0x8000u + index * 0x20u;
    }
    std::uint32_t ConstructClickCallback(
        const std::uint32_t storage, const std::uint32_t function,
        const std::uint32_t adjustment,
        const std::uint32_t frame) override {
        const auto index = callback_constructors++;
        Check(index < 2u && storage == 0x8000u + index * 0x20u &&
                  function == kFunctions[index] && adjustment == 0u &&
                  frame == kFrame,
              "first two buttons call shared callback constructor");
        if (fail_callback_constructor) {
            throw std::runtime_error("callback constructor");
        }
        WriteWord(storage + 8u, function);
        WriteWord(storage + 0xcu, adjustment);
        WriteWord(storage + 0x10u, frame);
        WriteWord(storage, 0x843d1cu);
        return storage;
    }
    void Free(const std::uint32_t storage) override {
        freed.push_back(storage);
    }
    void ReleaseCallback(const std::uint32_t callback,
                         const std::uint32_t flags) override {
        Check(flags == 1u, "failed binding callback delete flag");
        released_callbacks.push_back(callback);
    }
    std::uint32_t BindClicked(
        const std::uint32_t window, const std::uint32_t callback,
        const std::uint32_t clicked_event) override {
        const auto index = static_cast<std::uint32_t>(bound.size());
        Check(window == 0x6000u + index && clicked_event == 0xe000u &&
                  callback == (index == null_allocation_index ?
                                   0u : 0x8000u + index * 0x20u),
              "click binding window, callback and CEGUI event");
        bound.emplace_back(window, callback);
        if (index == bind_failure_index) {
            throw std::runtime_error("bind");
        }
        return 0x9000u + index;
    }
    void ReleaseConnection(const std::uint32_t connection) override {
        released_connections.push_back(connection);
    }
    std::pair<std::uint32_t, std::uint32_t> GetPositionBits(
        const std::uint32_t window) override {
        Check(window >= 0x6000u && window < 0x6008u,
              "button position query");
        const auto index = window - 0x6000u;
        return {0x3f000000u + index, 0x40000000u + index};
    }
    std::uint32_t GameDatabase() override { return 0xa000u; }
    std::uint32_t TriggerText(const std::uint32_t collection,
                              const std::uint32_t trigger) override {
        Check(collection == 0xa034u &&
                  trigger == kTriggers[triggers.size()],
              "ordered GameDB trigger query at +0x34");
        triggers.push_back(trigger);
        return trigger == 145u ? 0u : 0xb000u + trigger;
    }
    std::uint32_t ConvertToUtf8(const std::uint32_t text) override {
        Check(text != 0u, "null trigger text uses empty fallback");
        return text + 0x100u;
    }
    void SetWindowText(const std::uint32_t window,
                       const std::uint32_t utf8) override {
        Check(window == 0x6000u + texts.size(),
              "window text follows click binding order");
        texts.emplace_back(window, utf8);
    }
};

void TestAllControls() {
    Runtime runtime;
    Check(pal4::ShopUiControls::Initialize(Runtime::kFrame, runtime) ==
              0xfaceu &&
              runtime.names == std::vector<std::string>(kNames.begin(),
                                                        kNames.end()) &&
              runtime.prefix_reads == 10u &&
              runtime.allocations == 8u &&
              runtime.callback_constructors == 2u &&
              runtime.bound.size() == 8u &&
              runtime.released_connections.size() == 8u &&
              runtime.triggers == std::vector<std::uint32_t>(
                  kTriggers.begin(), kTriggers.end()) &&
              runtime.texts.size() == 8u &&
              runtime.empty_lookups == 1u &&
              runtime.name_cleanups == 1u,
          "eight ordered buttons, two passive windows and temporary cleanup");
    for (std::uint32_t i = 0u; i < 8u; ++i) {
        const auto callback = 0x8000u + i * 0x20u;
        Check(runtime.words.at(Runtime::kFrame + kWindowOffsets[i]) ==
                  0x6000u + i &&
                  runtime.words.at(Runtime::kFrame + kPositionOffsets[i]) ==
                  0x3f000000u + i &&
                  runtime.words.at(Runtime::kFrame + kPositionOffsets[i] +
                                   4u) == 0x40000000u + i &&
                  runtime.words.at(callback) == 0x843d1cu &&
                  runtime.words.at(callback + 8u) == kFunctions[i] &&
                  runtime.words.at(callback + 0xcu) == 0u &&
                  runtime.words.at(callback + 0x10u) == Runtime::kFrame &&
                  runtime.released_connections[i] == 0x9000u + i,
              "every button field, position, callback and connection");
    }
    Check(runtime.words.at(Runtime::kFrame + 0xc0u) == 0x6008u &&
              runtime.words.at(Runtime::kFrame + 0xc4u) == 0x6009u &&
              runtime.texts[2].second == 0xbfefu,
          "passive windows and missing trigger text fallback");
}

void TestNullPrefixAndAllocation() {
    Runtime runtime;
    runtime.words[Runtime::kFrame + 0x18u] = 0u;
    runtime.null_allocation_index = 2u;
    Check(pal4::ShopUiControls::Initialize(Runtime::kFrame, runtime) ==
              0xfaceu &&
              runtime.empty_lookups == 11u &&
              runtime.bound[2].second == 0u &&
              runtime.released_callbacks.empty() &&
              runtime.words.count(0x8040u) == 0u,
          "null prefix uses fallback per lookup and null callback still binds");
}

void TestExceptionalCleanup() {
    Runtime constructor;
    constructor.fail_callback_constructor = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::ShopUiControls::Initialize(
            Runtime::kFrame, constructor));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && constructor.freed ==
              std::vector<std::uint32_t>{0x8000u} &&
              constructor.name_cleanups == 1u &&
              constructor.bound.empty(),
          "shared callback constructor failure frees storage and name buffer");

    Runtime bind;
    bind.bind_failure_index = 3u;
    caught = false;
    try {
        static_cast<void>(pal4::ShopUiControls::Initialize(
            Runtime::kFrame, bind));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && bind.released_callbacks ==
              std::vector<std::uint32_t>{0x8060u} &&
              bind.released_connections.size() == 3u &&
              bind.name_cleanups == 1u,
          "binding failure releases current callback and temporary name");
}

}  // namespace

int main() {
    TestAllControls();
    TestNullPrefixAndAllocation();
    TestExceptionalCleanup();
    return EXIT_SUCCESS;
}
