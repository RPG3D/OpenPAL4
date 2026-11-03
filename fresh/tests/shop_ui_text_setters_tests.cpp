#include "pal4/shop_ui_text_setters.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IShopUiTextRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    static constexpr std::uint32_t kEvent = 0x6000u;
    std::uint32_t offset = 0x60u;
    std::map<std::uint32_t, std::uint32_t> words{
        {0x8b098cu, 0x8000u}, {kEvent + 4u, 0x9000u}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t manager_calls = 0u;
    std::uint32_t find_calls = 0u;
    std::uint32_t register_calls = 0u;
    std::uint32_t flag_calls = 0u;
    std::uint32_t reallocations = 0u;
    std::uint32_t copies = 0u;
    std::uint32_t length = 4u;
    std::uint32_t first_frame = kFrame;
    std::uint32_t second_frame = kFrame;

    explicit Runtime(const std::uint32_t field_offset) : offset(field_offset) {
        words[kFrame + offset + 4u] = 0xb001u;
        words[kFrame + offset + 8u] = 3u;
        words[kFrame + offset + 0xcu] = 8u;
        bytes[0xb000u] = 2u;
        for (std::uint32_t i = 0u; i < 4u; ++i) {
            bytes[0x9000u + i] =
                static_cast<std::uint8_t>('A' + i);
        }
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back(address == 0x8b098cu ? "name" : "read");
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        const auto it = bytes.find(address);
        return it == bytes.end() ? 0u : it->second;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        ++manager_calls;
        return 0x7000u;
    }
    std::uint32_t FindFrame(const std::uint32_t manager,
                            const std::uint32_t name) override {
        Check(manager == 0x7000u && name == 0x8000u,
              "frame lookup manager and TradeSelectWindow name");
        calls.emplace_back("find");
        return find_calls++ == 0u ? first_frame : second_frame;
    }
    void RegisterFrame(const std::uint32_t manager,
                       const std::uint32_t name) override {
        Check(manager == 0x7000u && name == 0x8000u,
              "missing shop frame registration arguments");
        calls.emplace_back("register");
        ++register_calls;
    }
    std::uint32_t CStringLength(
        const std::uint32_t text) override {
        Check(text == 0x9000u, "input event string pointer at +4");
        calls.emplace_back("length");
        return length;
    }
    [[noreturn]] void ThrowLength() override {
        calls.emplace_back("throw length");
        throw std::length_error("length");
    }
    void SetErrorFlag(const std::uint32_t text,
                      const std::uint8_t flag) override {
        Check(text == kFrame + offset && flag == 1u,
              "text detach flag and selected field offset");
        calls.emplace_back("flag");
        ++flag_calls;
    }
    void Reallocate(const std::uint32_t text,
                    const std::uint32_t requested) override {
        Check(text == kFrame + offset && requested == length,
              "reallocation target and byte length");
        calls.emplace_back("reallocate");
        ++reallocations;
        words[text + 4u] = 0xa001u;
        words[text + 0xcu] = requested;
    }
    void Copy(const std::uint32_t destination,
              const std::uint32_t source,
              const std::uint32_t count) override {
        Check(source == 0x9000u && count == length &&
                  (destination == 0xa001u || destination == 0xb001u),
              "copy bytes into selected shop text buffer");
        calls.emplace_back("copy");
        ++copies;
        for (std::uint32_t i = 0u; i < count; ++i) {
            bytes[destination + i] = ReadByte(source + i);
        }
    }
};

using Setter = std::uint8_t (*)(std::uint32_t,
                                pal4::IShopUiTextRuntime&);
constexpr std::array<Setter, 2> kSetters{
    &pal4::ShopUiTextSetters::SetShopId,
    &pal4::ShopUiTextSetters::SetTradeTaskScript};
constexpr std::array<std::uint32_t, 2> kOffsets{0x60u, 0x70u};

void TestNonemptyBranches() {
    for (std::size_t i = 0u; i < kSetters.size(); ++i) {
        Runtime shared(kOffsets[i]);
        Check(kSetters[i](Runtime::kEvent, shared) == 1u &&
                  shared.reallocations == 1u &&
                  shared.flag_calls == 0u &&
                  shared.copies == 1u &&
                  shared.words.at(Runtime::kFrame + kOffsets[i] + 8u) ==
                      4u &&
                  shared.ReadByte(0xa005u) == 0u &&
                  shared.ReadByte(0xa004u) == 'D',
              "shared nonempty text reallocates then copies and terminates");
        Check(shared.calls[0] == "manager" &&
                  shared.calls[1] == "name" &&
                  shared.calls[2] == "find",
              "frame manager call precedes name read and lookup");

        Runtime inline_text(kOffsets[i]);
        inline_text.bytes[0xb000u] = 0xffu;
        Check(kSetters[i](Runtime::kEvent, inline_text) == 1u &&
                  inline_text.flag_calls == 0u &&
                  inline_text.reallocations == 0u &&
                  inline_text.copies == 1u &&
                  inline_text.ReadByte(0xb005u) == 0u,
              "unshared small capacity copies into existing buffer");

        Runtime expanded(kOffsets[i]);
        expanded.bytes[0xb000u] = 0u;
        expanded.words[Runtime::kFrame + kOffsets[i] + 0xcu] = 0x20u;
        Check(kSetters[i](Runtime::kEvent, expanded) == 1u &&
                  expanded.flag_calls == 1u &&
                  expanded.reallocations == 1u &&
                  expanded.copies == 1u,
              "unshared large capacity detaches then reallocates");
    }
}

void TestEmptyAndFrameLookup() {
    for (std::size_t i = 0u; i < kSetters.size(); ++i) {
        Runtime shared(kOffsets[i]);
        shared.length = 0u;
        Check(kSetters[i](Runtime::kEvent, shared) == 1u &&
                  shared.ReadByte(0xb000u) == 1u &&
                  shared.words.at(Runtime::kFrame + kOffsets[i] + 4u) ==
                      0u &&
                  shared.words.at(Runtime::kFrame + kOffsets[i] + 8u) ==
                      0u &&
                  shared.words.at(Runtime::kFrame + kOffsets[i] + 0xcu) ==
                      0u &&
                  shared.flag_calls == 0u && shared.copies == 0u,
              "empty shared text decrements reference and clears three words");

        Runtime unique(kOffsets[i]);
        unique.length = 0u;
        unique.bytes[0xb000u] = 0xffu;
        Check(kSetters[i](Runtime::kEvent, unique) == 1u &&
                  unique.flag_calls == 1u && unique.copies == 0u,
              "empty unique text uses detach flag");

        Runtime registered(kOffsets[i]);
        registered.first_frame = 0u;
        Check(kSetters[i](Runtime::kEvent, registered) == 1u &&
                  registered.manager_calls == 3u &&
                  registered.find_calls == 2u &&
                  registered.register_calls == 1u &&
                  registered.copies == 1u,
              "missing shop frame is registered and found again");

        Runtime missing(kOffsets[i]);
        missing.first_frame = 0u;
        missing.second_frame = 0u;
        Check(kSetters[i](Runtime::kEvent, missing) == 1u &&
                  missing.register_calls == 1u &&
                  missing.find_calls == 2u &&
                  missing.copies == 0u &&
                  missing.calls.back() == "find",
              "unavailable frame returns before reading event data");
    }
}

void TestLengthGate() {
    Runtime runtime(0x60u);
    runtime.length = 0xfffffffeu;
    bool caught = false;
    try {
        static_cast<void>(
            pal4::ShopUiTextSetters::SetShopId(Runtime::kEvent, runtime));
    } catch (const std::length_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "throw length" &&
              runtime.reallocations == 0u && runtime.copies == 0u,
          "lengths above FFFFFFFD use standard length exception");
}

}  // namespace

int main() {
    TestNonemptyBranches();
    TestEmptyAndFrameLookup();
    TestLengthGate();
    return EXIT_SUCCESS;
}
