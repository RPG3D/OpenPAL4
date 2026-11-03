#include "pal4/numbered_list_item.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::INumberedListItemRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::pair<std::string, std::string>> images;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> frees;
    std::string base_text;
    std::uint8_t initial_byte = 0x72u;

    void ConstructBase(std::uint32_t item,
                       std::string_view text,
                       std::uint32_t id,
                       std::uint32_t data,
                       bool disabled,
                       bool auto_delete) override {
        Check(item == 0x1000u && id == 0u && data == 0u &&
                  !disabled && auto_delete,
              "base ListboxItem flags");
        base_text = text;
        calls.emplace_back("base");
    }
    std::uint8_t InitialNameByte() override {
        calls.emplace_back("initial_byte");
        return initial_byte;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void SetErrorFlag(std::uint32_t address,
                      std::uint8_t value) override {
        Check(address == 0x1000u + 424u && value == 0u,
              "name flag initializer");
        calls.emplace_back("flag");
    }
    void AssignName(std::uint32_t destination,
                    std::uint32_t source,
                    std::uint32_t offset,
                    std::uint32_t length) override {
        Check(destination == 0x1000u + 424u &&
                  source == 0x6000u && offset == 0u &&
                  length == 0xffffffffu,
              "name assignment arguments");
        words[destination + 4u] = 0x7001u;
        words[destination + 8u] = 4u;
        words[destination + 12u] = 4u;
        calls.emplace_back("name");
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        Check(destination == 0x1000u + 444u &&
                  source == 0x8000u && length == 0x34u,
              "52-byte source record");
        for (std::uint32_t i = 0; i < length; ++i)
            bytes[destination + i] = bytes[source + i];
        calls.emplace_back("copy");
    }
    std::uint32_t FindImage(std::string_view imageset,
                            std::string_view image) override {
        images.emplace_back(imageset, image);
        calls.emplace_back("image");
        return 0xa000u +
               static_cast<std::uint32_t>(images.size()) * 0x100u;
    }
    void Free(std::uint32_t address) override {
        frees.push_back(address);
        calls.emplace_back("free:" + std::to_string(address));
    }
    std::uint32_t BaseItemVtable() override { return 0x8412f4u; }
    void DestroyString(std::uint32_t address) override {
        calls.emplace_back("string:" + std::to_string(address));
    }
};

std::size_t Find(const std::vector<std::string>& calls,
                 const std::string& target) {
    const auto it = std::find(calls.begin(), calls.end(), target);
    Check(it != calls.end(), "missing call");
    return static_cast<std::size_t>(it - calls.begin());
}

void CheckConstruct(std::int32_t index, bool alternate) {
    Runtime runtime;
    for (std::uint32_t i = 0; i < 0x34u; ++i)
        runtime.bytes[0x8000u + i] =
            static_cast<std::uint8_t>(i ^ 0x5au);
    const auto result = pal4::NumberedListItem::Construct(
        0x1000u, index, 0x8000u, 0x9000u,
        alternate, 0x6000u, runtime);
    const auto expected_number = static_cast<std::uint32_t>(index) + 1u;
    const auto expected_text = expected_number == 0x80000000u
                                   ? "-2147483648"
                                   : std::to_string(index + 1);
    Check(result == 0x1000u && runtime.base_text == expected_text &&
              runtime.bytes[0x1000u + 424u] == 0x72u &&
              runtime.words[0x1000u] ==
                  pal4::NumberedListItem::kVtable &&
              runtime.words[0x1000u + 440u] ==
                  static_cast<std::uint32_t>(index) &&
              runtime.words[0x1000u + 496u] == 0xa200u &&
              runtime.words[0x1000u + 500u] == 0xa100u &&
              runtime.words[0x1000u + 504u] == 0xa300u &&
              runtime.words[0x1000u + 508u] == 0x9000u,
          "constructor text, fields and image handles");
    for (std::uint32_t i = 0; i < 0x34u; ++i)
        Check(runtime.bytes[0x1000u + 444u + i] ==
                  runtime.bytes[0x8000u + i],
              "record bytes copied");
    Check(runtime.images ==
              (std::vector<std::pair<std::string, std::string>>{
                  {"gy0", "xuanzhekuang2"},
                  {"kuang0", "kuang21"},
                  {"kuang20", alternate ? "kuang21" : "kuang20"}}) &&
              Find(runtime.calls, "base") <
                  Find(runtime.calls, "flag") &&
              Find(runtime.calls, "flag") <
                  Find(runtime.calls, "name") &&
              Find(runtime.calls, "name") <
                  Find(runtime.calls, "copy") &&
              Find(runtime.calls, "copy") <
                  Find(runtime.calls, "image"),
          "constructor resource branch and operation order");
}

void CheckDestruct(std::uint8_t references,
                   std::uint8_t flags) {
    Runtime runtime;
    runtime.words[0x1000u + 428u] = 0x7001u;
    runtime.words[0x1000u + 432u] = 4u;
    runtime.words[0x1000u + 436u] = 4u;
    runtime.bytes[0x7000u] = references;
    Check(pal4::NumberedListItem::ConditionalDestruct(
              0x1000u, flags, runtime) == 0x1000u &&
              runtime.words[0x1000u + 428u] == 0u &&
              runtime.words[0x1000u + 432u] == 0u &&
              runtime.words[0x1000u + 436u] == 0u &&
              runtime.words[0x1000u] == 0x8412f4u &&
              Find(runtime.calls, "string:4252") <
                  Find(runtime.calls, "string:4100"),
          "name and ListboxItem destruction");
    if (references == 0u || references == 0xffu)
        Check(Find(runtime.calls, "free:28672") <
                  Find(runtime.calls, "string:4252"),
              "unshared name storage freed");
    else
        Check(runtime.bytes[0x7000u] == references - 1u &&
                  std::find(runtime.frees.begin(), runtime.frees.end(),
                            0x7000u) == runtime.frees.end(),
              "shared name reference decremented");
    const auto delete_item = (flags & 1u) != 0u;
    Check((std::find(runtime.frees.begin(), runtime.frees.end(),
                     0x1000u) != runtime.frees.end()) == delete_item,
          "deleting destructor flag");
    if (delete_item)
        Check(Find(runtime.calls, "string:4100") <
                  Find(runtime.calls, "free:4096"),
              "item freed only after base destructor");
}

void CheckNullName() {
    Runtime runtime;
    runtime.words[0x1000u + 428u] = 0u;
    pal4::NumberedListItem::Destruct(0x1000u, runtime);
    Check(runtime.frees.empty() &&
              runtime.words[0x1000u + 428u] == 0u,
          "null name skips release");
}

}  // namespace

int main() {
    try {
        CheckConstruct(0, true);
        CheckConstruct(-1, false);
        CheckConstruct(2147483647, true);
        CheckDestruct(0u, 1u);
        CheckDestruct(0xffu, 2u);
        CheckDestruct(3u, 3u);
        CheckNullName();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
