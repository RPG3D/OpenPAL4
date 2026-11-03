#include "pal4/combat_ending_window_constructor.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kWindow = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEndingWindowConstructorRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::string> images;
    std::vector<std::string> hint_names;
    int allocation_index = 0;
    int failed_allocation = -1;
    int throwing_hint = -1;
    std::uint32_t freed = 0u;

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }

    void LoadLayout(std::uint32_t window, std::string_view name) override {
        Check(window == kWindow && name == "CombatEndingWindow",
              "layout arguments");
        calls.emplace_back("layout");
    }
    std::uint32_t CreateListNode() override {
        calls.emplace_back("list");
        return 0x2000u;
    }
    void ConstructStream(std::uint32_t address) override {
        Check(address == kWindow + 420u, "stream offset");
        calls.emplace_back("stream");
    }
    std::uint32_t FindImage(std::string_view imageset,
                            std::string_view image) override {
        Check(imageset == "zi0", "imageset name");
        images.emplace_back(image);
        calls.emplace_back("image:" + std::string(image));
        return 0x4000u + static_cast<std::uint32_t>(images.size());
    }
    std::uint32_t FindIcon(std::uint32_t index, std::uint32_t variant,
                           std::uint32_t flags) override {
        Check(index < 4u && variant == 0u && flags == 0u,
              "icon arguments");
        calls.emplace_back("icon:" + std::to_string(index));
        return 0x5000u + index;
    }
    std::uint32_t AllocateHint(std::uint32_t size) override {
        Check(size == 0x48u, "hint allocation size");
        const auto index = allocation_index++;
        calls.emplace_back("allocate:" + std::to_string(index));
        return index == failed_allocation ? 0u : 0x6000u + 0x100u * index;
    }
    std::uint32_t ConstructHint(std::uint32_t address,
                                std::string_view name) override {
        const auto index = static_cast<int>((address - 0x6000u) / 0x100u);
        Check(address == 0x6000u + 0x100u * index, "hint allocation address");
        hint_names.emplace_back(name);
        calls.emplace_back("hint:" + std::to_string(index));
        if (index == throwing_hint) throw std::runtime_error("hint failure");
        return address + 0x10u;
    }
    void FreeHint(std::uint32_t address) override {
        freed = address;
        calls.emplace_back("free");
    }
    void SetupElements(std::uint32_t window) override {
        Check(window == kWindow, "setup owner");
        calls.emplace_back("setup");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t EventKey(std::string_view name) override {
        Check(name == "combat_prize", "event name");
        calls.emplace_back("event-key");
        return 0x7000u;
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x8000u;
    }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t key, std::uint32_t owner,
                           std::uint32_t callback) override {
        Check(router == 0x8000u && mode == 2u && key == 0x7000u &&
                  owner == kWindow && callback == 0x545ae0u,
              "prize registration");
        calls.emplace_back("register");
        return 0x9000u;
    }
    std::uint32_t FindEventGroup(std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("new window has no prior event group");
    }
    void ReleaseOwnedSlot(std::uint32_t) override {}
    void FreeNode(std::uint32_t) override {}
};

void CompleteConstruction() {
    Runtime runtime;
    Check(pal4::CombatEndingWindowConstructor::Construct(
              kWindow, "CombatEndingWindow", runtime) == kWindow,
          "constructor return");
    Check(runtime.words[kWindow] == 0x845370u &&
              runtime.words[kWindow + 40u] == 0x845358u,
          "final vtables");
    Check(runtime.bytes[kWindow + 44u] == 0u &&
              runtime.words[kWindow + 48u] == 0x2000u,
          "embedded list initialization");
    for (auto offset : {52u, 700u, 704u, 708u, 712u}) {
        Check(runtime.words[kWindow + offset] == 0u,
              "zero initialized field");
    }
    const std::array<std::string_view, 9> expected_images{
        "yuntianheheng", "hanlingshaheng", "liumengliheng",
        "murongziyinheng", "tedeng", "yideng", "erdeng", "sandeng",
        "sideng"};
    Check(runtime.images.size() == expected_images.size(), "image count");
    for (std::uint32_t i = 0; i < expected_images.size(); ++i) {
        Check(runtime.images[i] == expected_images[i], "image order/name");
        Check(runtime.words[kWindow + 648u + 4u * i] == 0x4001u + i,
              "image slot order");
    }
    for (std::uint32_t i = 0; i < 4u; ++i) {
        Check(runtime.words[kWindow + 632u + 4u * i] == 0x5000u + i,
              "icon slot");
        Check(runtime.words[kWindow + 684u + 4u * i] ==
                  0x6010u + 0x100u * i,
              "hint slot");
        Check(runtime.hint_names[i] ==
                  "CombatEndingLevelUpHint" + std::to_string(i),
              "hint name");
    }
    Check(runtime.words[kWindow + 56u] == 0x9000u &&
              runtime.words[kWindow + 60u] == 0x7000u,
          "event pair");
    Check(runtime.calls.front() == "layout" &&
              runtime.calls[1] == "list" &&
              runtime.calls[2] == "stream",
          "base initialization order");
    Check(runtime.calls[3] == "image:yuntianheheng" &&
              runtime.calls[6] == "image:murongziyinheng" &&
              runtime.calls[7] == "icon:0" &&
              runtime.calls[19] == "image:tedeng",
          "portrait/icon/rank ordering");
    Check(runtime.calls[runtime.calls.size() - 4] == "event-key" &&
              runtime.calls[runtime.calls.size() - 3] == "router" &&
              runtime.calls[runtime.calls.size() - 2] == "register" &&
              runtime.calls.back() == "setup",
          "event and element ordering");
}

void FailedAllocation() {
    Runtime runtime;
    runtime.failed_allocation = 1;
    (void)pal4::CombatEndingWindowConstructor::Construct(
        kWindow, "CombatEndingWindow", runtime);
    Check(runtime.words[kWindow + 688u] == 0u,
          "failed hint allocation stores null");
    Check(runtime.words[kWindow + 696u] == 0x6310u,
          "later hints still construct");
    Check(runtime.hint_names.size() == 3u &&
              runtime.calls.back() == "setup",
          "allocation failure still completes dialog");
}

void ConstructorFailure() {
    Runtime runtime;
    runtime.throwing_hint = 2;
    bool threw = false;
    try {
        (void)pal4::CombatEndingWindowConstructor::Construct(
            kWindow, "CombatEndingWindow", runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.freed == 0x6200u,
          "failing hint allocation released");
    Check(runtime.words[kWindow + 692u] == 0u &&
              runtime.images.size() == 4u &&
              runtime.calls.back() == "free",
          "construction stops before remaining fields and subscriptions");
}

}  // namespace

int main() {
    try {
        CompleteConstruction();
        FailedAllocation();
        ConstructorFailure();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
