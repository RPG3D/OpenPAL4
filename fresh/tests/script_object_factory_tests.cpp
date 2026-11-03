#include "pal4/script_object_factory.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kOutput = 0x1000u;
constexpr std::uint32_t kObject = 0x3000u;

struct Runtime final : pal4::IScriptObjectFactoryRuntime {
    std::uint32_t allocation_result = kObject;
    std::uint32_t allocation_count = 0u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t size) override {
        ++allocation_count;
        calls.push_back("alloc:" + std::to_string(size));
        return allocation_result;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte:" + std::to_string(address));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word:" + std::to_string(address));
    }
};

enum class Form { Base12, Simple12, Count, Pending, Gate,
                  Reactive, VariantEight };
struct Case {
    std::int32_t group, variant;
    std::uint32_t size, vtable;
    Form form;
    bool pointer_first;
};

constexpr Case kCases[] = {
    {0, 1, 12u, 0x847e10u, Form::Base12, false},
    {0, 2, 12u, 0x847cb0u, Form::Base12, false},
    {0, 3, 12u, 0x847b50u, Form::Simple12, true},
    {0, 4, 12u, 0x8479f0u, Form::Simple12, true},
    {0, 5, 12u, 0x847890u, Form::Simple12, true},
    {1, 1, 12u, 0x847730u, Form::Simple12, true},
    {1, 2, 12u, 0x8475d0u, Form::Simple12, true},
    {1, 3, 12u, 0x847470u, Form::Simple12, true},
    {1, 4, 12u, 0x847310u, Form::Simple12, true},
    {1, 5, 12u, 0x8471b0u, Form::Simple12, true},
    {1, 6, 12u, 0x847050u, Form::Simple12, true},
    {2, 1, 16u, 0x846290u, Form::Count, true},
    {2, 2, 16u, 0x8463f0u, Form::Pending, true},
    {2, 3, 16u, 0x846550u, Form::Gate, true},
    {2, 4, 12u, 0x846ef0u, Form::Simple12, true},
    {2, 5, 16u, 0x8466b0u, Form::Reactive, true},
    {2, 6, 12u, 0x846d90u, Form::Simple12, true},
    {8, 1, 16u, 0x847f70u, Form::VariantEight, true},
    {8, 2, 16u, 0x846c30u, Form::VariantEight, false},
    {8, 3, 16u, 0x846ad0u, Form::VariantEight, true},
    {8, 4, 16u, 0x846970u, Form::VariantEight, false},
    {8, 5, 16u, 0x846810u, Form::VariantEight, false},
};

void CheckOutputOrder(const Runtime& r, bool pointer_first) {
    Check(r.calls.size() >= 2u, "output operations exist");
    const auto n = r.calls.size();
    if (pointer_first) {
        Check(r.calls[n - 2u] == "word:4100" &&
                  r.calls[n - 1u] == "byte:4096",
              "pointer-first output order");
    } else {
        Check(r.calls[n - 2u] == "byte:4096" &&
                  r.calls[n - 1u] == "word:4100",
              "success-first output order");
    }
}

void CheckConstructorOrder(const Runtime& r, const Case& item) {
    const auto byte4 = std::string("byte:12292");
    const auto word8 = std::string("word:12296");
    const auto byte12 = std::string("byte:12300");
    const auto word12 = std::string("word:12300");
    const auto vtable = std::string("word:12288");
    std::vector<std::string> expected{"alloc:" +
                                      std::to_string(item.size),
                                      byte4, word8};
    switch (item.form) {
        case Form::Base12:
            expected.insert(expected.end(), {vtable, vtable});
            break;
        case Form::Simple12:
        case Form::Gate:
            expected.push_back(vtable);
            break;
        case Form::Count:
            expected.insert(expected.end(), {word12, vtable});
            break;
        case Form::Pending:
            expected.insert(expected.end(), {vtable, word12});
            break;
        case Form::Reactive:
            expected.insert(expected.end(), {byte12, vtable});
            break;
        case Form::VariantEight:
            expected.insert(expected.end(), {byte12, vtable});
            if (item.variant != 1) expected.push_back(vtable);
            break;
    }
    if (item.pointer_first)
        expected.insert(expected.end(), {"word:4100", "byte:4096"});
    else
        expected.insert(expected.end(), {"byte:4096", "word:4100"});
    Check(r.calls == expected, "constructor and output write order");
}

void AllVariants() {
    for (const auto& item : kCases) {
        Runtime r;
        Check(pal4::ScriptObjectFactory::Create(
                  kOutput, item.group, item.variant, r) == kOutput &&
                  r.allocation_count == 1u &&
                  r.bytes.at(kOutput) == 1u &&
                  r.words.at(kOutput + 4u) == kObject &&
                  r.bytes.at(kObject + 4u) == 0u &&
                  r.words.at(kObject + 8u) == 0u &&
                  r.words.at(kObject) == item.vtable,
              "factory valid variant size, fields, vtable and output");
        if (item.form == Form::Count || item.form == Form::Pending)
            Check(r.words.at(kObject + 12u) == 0u,
                  "count and pending constructors clear word twelve");
        if (item.form == Form::Reactive ||
            item.form == Form::VariantEight)
            Check(r.bytes.at(kObject + 12u) == 0u,
                  "reactive and variant-eight constructors clear byte twelve");
        if (item.form == Form::Gate)
            Check(!r.words.contains(kObject + 12u) &&
                      !r.bytes.contains(kObject + 12u),
                  "pending-gate constructor leaves trailing word unwritten");
        CheckConstructorOrder(r, item);
        CheckOutputOrder(r, item.pointer_first);

        r = Runtime{};
        r.allocation_result = 0u;
        Check(pal4::ScriptObjectFactory::Create(
                  kOutput, item.group, item.variant, r) == kOutput &&
                  r.allocation_count == 1u &&
                  r.bytes.at(kOutput) == 0u &&
                  r.words.at(kOutput + 4u) == 0u &&
                  !r.words.contains(kObject) &&
                  !r.bytes.contains(kObject + 4u) &&
                  r.calls.size() == 3u &&
                  r.calls.front() == "alloc:" +
                                       std::to_string(item.size),
              "factory null allocation skips construction");
        CheckOutputOrder(r, item.pointer_first);
    }
}

void InvalidVariants() {
    const std::pair<std::int32_t, std::int32_t> cases[] = {
        {-1, 1}, {0, 0}, {0, 6}, {1, 7}, {2, 7},
        {3, 1}, {8, 0}, {8, 6}, {9, 1}};
    for (const auto [group, variant] : cases) {
        Runtime r;
        Check(pal4::ScriptObjectFactory::Create(
                  kOutput, group, variant, r) == kOutput &&
                  r.allocation_count == 0u &&
                  r.bytes.at(kOutput) == 0u &&
                  r.words.at(kOutput + 4u) == 0u &&
                  r.calls == std::vector<std::string>{
                      "byte:4096", "word:4100"},
              "invalid group or variant clears output without allocation");
    }
}

void BaseConstructor() {
    Runtime r;
    Check(pal4::ScriptObjectFactory::ConstructBase12(
              kObject, r) == kObject &&
              r.bytes.at(kObject + 4u) == 0u &&
              r.words.at(kObject + 8u) == 0u &&
              r.words.at(kObject) == 0x845fd0u &&
              r.calls == std::vector<std::string>{
                  "byte:12292", "word:12296", "word:12288"},
          "base constructor field order and vtable");
}

}  // namespace

int main() {
    try {
        AllVariants();
        InvalidVariants();
        BaseConstructor();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
