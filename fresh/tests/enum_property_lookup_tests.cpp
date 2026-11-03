#include "pal4/enum_property_lookup.h"

#include <cstdlib>
#include <iostream>
#include <map>
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

struct Runtime final : pal4::IEnumPropertyLookupRuntime {
    std::int32_t mapped{};
    std::uint32_t next_address{0x2000u};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> registered;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint8_t> flags;
    std::vector<std::uint32_t> allocations;
    std::uint32_t last_slot{};
    std::int32_t last_input{};

    std::int32_t MapIndex(const pal4::EnumPropertyState&,
                          std::int32_t input, std::uint32_t slot) override {
        last_input = input;
        last_slot = slot;
        return mapped;
    }
    void RegisterAtExit(std::uint32_t callback) override {
        registered.push_back(callback);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override { freed.push_back(address); }
    void SetErrorFlag(pal4::StringBufferState& text,
                      std::uint8_t flag) override {
        flags.push_back(flag);
        if (text.data != 0u && flag == 1u) {
            bytes[text.data - 1u] = 0u;
        }
    }
    void Reallocate(pal4::StringBufferState& text,
                    std::uint32_t length) override {
        const auto old = text.data;
        const auto allocated = next_address;
        next_address += 0x100u;
        allocations.push_back(length);
        bytes[allocated - 1u] = 0u;
        if (old != 0u) {
            for (std::uint32_t i = 0; i < text.length; ++i)
                bytes[allocated + i] = bytes.at(old + i);
        }
        text.data = allocated;
        text.capacity = length;
    }
    void Copy(std::uint32_t destination, std::string_view value) override {
        for (std::size_t i = 0; i < value.size(); ++i)
            bytes[destination + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(value[i]);
    }
    void SetSize(pal4::StringBufferState& text,
                 std::uint32_t length) override {
        text.length = length;
        bytes[text.data + length] = 0u;
    }
    std::string Text(const pal4::StringBufferState& text) const {
        std::string value;
        for (std::uint32_t i = 0; i < text.length; ++i)
            value.push_back(static_cast<char>(bytes.at(text.data + i)));
        return value;
    }
};

void TestValidEntries() {
    const pal4::EnumPropertyState property{0x1234u, 0x4000u, 3};
    pal4::EnumPropertyErrorState error{};
    Runtime runtime;
    runtime.mapped = 0;
    Check(pal4::EnumPropertyLookup::Lookup(property, 99, error, runtime) ==
              0x4000u && runtime.last_slot == 0x10u &&
              runtime.last_input == 99,
          "first entry and virtual map arguments");
    runtime.mapped = 2;
    Check(pal4::EnumPropertyLookup::Lookup(property, 1, error, runtime) ==
              0x4020u && runtime.registered.empty() &&
              error.guard_word == 0u,
          "last entry bypasses error text");
}

void TestErrorTextAndReuse() {
    const pal4::EnumPropertyState property{0u, 0x4000u, 3};
    pal4::EnumPropertyErrorState error{0xabcdef00u,
                                       {0x1111u, 5u, 9u}, 0x12340000u};
    Runtime runtime;
    runtime.mapped = -12;
    Check(pal4::EnumPropertyLookup::Lookup(property, 0x12345678, error,
                                           runtime) ==
              pal4::EnumPropertyLookup::kErrorAddress &&
              runtime.Text(error.text) ==
                  "-12 <ptnEnumProperty EnumError>" &&
              error.flag_word == 0xabcdef78u &&
              error.guard_word == 0x12340001u &&
              runtime.registered == std::vector<std::uint32_t>{
                  pal4::EnumPropertyLookup::kCleanupAddress},
          "negative index initializes static error record and exact text");
    runtime.mapped = 3;
    Check(pal4::EnumPropertyLookup::Lookup(property, 0x99, error, runtime) ==
              pal4::EnumPropertyLookup::kErrorAddress &&
              runtime.Text(error.text) ==
                  "3 <ptnEnumProperty EnumError>" &&
              runtime.registered.size() == 1u &&
              error.flag_word == 0xabcdef78u,
          "index equal to count replaces text without registering twice");
    Check(runtime.allocations.size() == 2u,
          "reuse fits the existing error-text capacity");
}

void TestSharedTextDetaches() {
    pal4::EnumPropertyState property{0u, 0x4000u, 0};
    pal4::EnumPropertyErrorState error{};
    Runtime runtime;
    runtime.mapped = 4;
    (void)pal4::EnumPropertyLookup::Lookup(property, 4, error, runtime);
    const auto old_data = error.text.data;
    runtime.bytes[old_data - 1u] = 2u;
    runtime.mapped = 5;
    (void)pal4::EnumPropertyLookup::Lookup(property, 5, error, runtime);
    Check(error.text.data != old_data &&
              runtime.Text(error.text) ==
                  "5 <ptnEnumProperty EnumError>" &&
              runtime.registered.size() == 1u,
          "shared text detaches before replacement");
}

void TestCleanup() {
    for (const std::uint8_t reference : {2u, 0u, 0xffu}) {
        pal4::EnumPropertyErrorState error{0x12345678u,
                                           {0x3000u, 9u, 32u}, 0xaabbcc01u};
        Runtime runtime;
        runtime.bytes[0x2fffu] = reference;
        pal4::EnumPropertyLookup::Cleanup(error, runtime);
        Check(error.text.data == 0u && error.text.length == 0u &&
                  error.text.capacity == 0u &&
                  error.flag_word == 0x12345678u &&
                  error.guard_word == 0xaabbcc01u,
              "cleanup clears only string fields");
        if (reference == 2u) {
            Check(runtime.bytes.at(0x2fffu) == 1u && runtime.freed.empty(),
                  "positive reference count is decremented");
        } else {
            Check(runtime.freed == std::vector<std::uint32_t>{0x2fffu},
                  "zero and ff reference counts free the header");
        }
    }
    pal4::EnumPropertyErrorState empty{0u, {}, 1u};
    Runtime runtime;
    pal4::EnumPropertyLookup::Cleanup(empty, runtime);
    Check(runtime.freed.empty() && empty.guard_word == 1u,
          "empty cleanup leaves guard set");

    const pal4::EnumPropertyState property{0u, 0x4000u, 0};
    runtime.mapped = -1;
    (void)pal4::EnumPropertyLookup::Lookup(property, 0x12, empty, runtime);
    pal4::EnumPropertyLookup::Cleanup(empty, runtime);
    runtime.mapped = -2;
    (void)pal4::EnumPropertyLookup::Lookup(property, 0x34, empty, runtime);
    Check(runtime.registered.empty() &&
              runtime.Text(empty.text) ==
                  "-2 <ptnEnumProperty EnumError>" &&
              (empty.flag_word & 0xffu) == 0u,
          "post-cleanup lookup rebuilds text without registration");
}

}  // namespace

int main() {
    TestValidEntries();
    TestErrorTextAndReuse();
    TestSharedTextDetaches();
    TestCleanup();
}
