#include "pal4/trade_window_item_snapshot.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICeguiTextRecordRuntime {
    static constexpr std::uint32_t source = 0x1000u;
    static constexpr std::uint32_t destination = 0x2000u;
    std::map<std::uint32_t, std::uint8_t> memory;
    std::vector<std::string> calls;
    std::uint32_t allocations = 0u;
    std::uint32_t detach_count = 0u;
    bool fail_first_allocation = false;

    Runtime() {
        for (std::uint32_t offset = 0u;
             offset < pal4::TradeWindowItemSnapshot::kBytes; ++offset)
            memory[source + offset] =
                static_cast<std::uint8_t>((offset * 7u) & 0xffu);
        for (std::uint32_t index = 0u;
             index < pal4::TradeWindowItemSnapshot::kTextCount; ++index) {
            const auto record = source + 0x98u + index * 16u;
            const auto text = 0x4000u + index * 0x100u;
            WriteByte(record, 0u);
            WriteDword(record + 4u, text);
            WriteDword(record + 8u, 3u);
            WriteDword(record + 12u, 4u);
            WriteByte(text - 1u, index % 2u == 0u ? 0u : 0xfeu);
            memory[text] = 'A';
            memory[text + 1u] = 'B';
            memory[text + 2u] = 'C';
        }
        calls.clear();
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        const auto found = memory.find(address);
        return found == memory.end() ? 0u : found->second;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        std::uint32_t value = 0u;
        for (std::uint32_t index = 0u; index < 4u; ++index)
            value |= static_cast<std::uint32_t>(ReadByte(address + index)) <<
                     (index * 8u);
        return value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        memory[address] = value;
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        for (std::uint32_t index = 0u; index < 4u; ++index)
            WriteByte(address + index,
                      static_cast<std::uint8_t>(value >> (index * 8u)));
    }
    std::uint32_t EmptyTextAddress() override { return 0x9000u; }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t flag) override {
        calls.emplace_back("share");
        WriteByte(record, flag);
    }
    bool AllocateText(std::uint32_t record, std::uint32_t size,
                      std::uint8_t mode) override {
        Check(mode == 1u && size <= 3u, "text allocation mode and size");
        calls.emplace_back("allocate");
        ++allocations;
        if (fail_first_allocation && allocations == 1u) return false;
        WriteDword(record + 4u, 0xa000u + allocations * 0x100u);
        return true;
    }
    void DetachAndReset(std::uint32_t) override {
        ++detach_count;
        calls.emplace_back("detach");
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t size) override {
        WriteDword(record + 8u, size);
    }
    void CopyBytes(std::uint32_t output, std::uint32_t input,
                   std::uint32_t size) override {
        calls.push_back("copy:" + std::to_string(size));
        std::vector<std::uint8_t> bytes;
        for (std::uint32_t index = 0u; index < size; ++index)
            bytes.push_back(ReadByte(input + index));
        for (std::uint32_t index = 0u; index < size; ++index)
            WriteByte(output + index, bytes[index]);
    }
    [[noreturn]] void ThrowRange() override {
        throw std::out_of_range("self text copy");
    }
};

void CheckCopyConstruct() {
    Runtime runtime;
    const auto result = pal4::TradeWindowItemSnapshot::CopyConstruct(
        Runtime::destination, Runtime::source, runtime);
    Check(result == Runtime::destination, "destination return");
    for (std::uint32_t offset = 0u; offset < 0x98u; ++offset)
        Check(runtime.ReadByte(Runtime::destination + offset) ==
                  runtime.ReadByte(Runtime::source + offset),
              "first 152 fixed bytes");
    for (std::uint32_t offset = 0x168u;
         offset < pal4::TradeWindowItemSnapshot::kBytes; ++offset)
        Check(runtime.ReadByte(Runtime::destination + offset) ==
                  runtime.ReadByte(Runtime::source + offset),
              "last 40 fixed bytes");
    for (std::uint32_t index = 0u;
         index < pal4::TradeWindowItemSnapshot::kTextCount; ++index) {
        const auto record = Runtime::destination + 0x98u + index * 16u;
        const auto source_text = 0x4000u + index * 0x100u;
        const auto copied_text = runtime.ReadDword(record + 4u);
        Check(runtime.ReadDword(record + 8u) == 3u,
              "text length copied");
        if (index % 2u == 0u) {
            Check(copied_text == source_text &&
                  runtime.ReadByte(record) == 1u &&
                  runtime.ReadByte(source_text - 1u) == 1u,
                  "shareable text refcount and pointer");
        } else {
            Check(copied_text != source_text &&
                  runtime.ReadByte(copied_text) == 'A' &&
                  runtime.ReadByte(copied_text + 2u) == 'C' &&
                  runtime.ReadByte(copied_text + 3u) == 0u,
                  "sentinel text deep copied");
        }
    }
    Check(runtime.allocations == 6u &&
          runtime.calls.front() == "copy:144" &&
          runtime.calls.back() == "copy:24", "copy phases and count");
}

void CheckAllocationFailureAndAlias() {
    Runtime failing;
    failing.fail_first_allocation = true;
    (void)pal4::TradeWindowItemSnapshot::CopyConstruct(
        Runtime::destination, Runtime::source, failing);
    const auto failed_record = Runtime::destination + 0x98u + 16u;
    Check(failing.ReadDword(failed_record + 4u) == 0u &&
          failing.ReadDword(failed_record + 8u) == 0u &&
          failing.allocations == 6u,
          "failed deep allocation leaves field empty and continues");

    Runtime alias;
    Check(pal4::TradeWindowItemSnapshot::CopyConstruct(
              Runtime::source, Runtime::source, alias) == Runtime::source &&
          alias.detach_count == 26u &&
          alias.allocations == 0u,
          "self alias uses two detach calls for each text field");
}

}  // namespace

int main() {
    try {
        CheckCopyConstruct();
        CheckAllocationFailureAndAlias();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
