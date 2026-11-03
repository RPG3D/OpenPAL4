#include "pal4/utilities_table_binding_refresh.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string name;
    std::uint32_t first = 0;
    std::uint32_t second = 0;
    pal4::UtilitiesBindingSchema schema =
        pal4::UtilitiesBindingSchema::IdString;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IUtilitiesTableBindingRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1080, 0x3000}, {0x1084, 0x3004},
        {0x1018, 0x4000}, {0x101c, 0x4078}};
    std::vector<Call> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void LogBind(std::string_view prefix,
                 std::uint32_t name) override {
        calls.push_back({prefix == "beginBindField, table:"
                             ? "begin" : "end", name});
    }
    void AssignEffects(std::uint32_t to,
                       std::uint32_t from) override {
        calls.push_back({"assign", to, from});
    }
    void RegisterFields(std::uint32_t binding,
                        pal4::UtilitiesBindingSchema schema) override {
        calls.push_back({"register", binding, 0, schema});
    }
    void BindEffects(std::uint32_t binding) override {
        calls.push_back({"bind", binding});
    }
};

using RefreshFn = void (*)(
    std::uint32_t, pal4::IUtilitiesTableBindingRuntime&);

struct Variant {
    RefreshFn refresh;
    pal4::UtilitiesBindingSchema schema;
};

constexpr std::array<Variant, 7> kVariants{{
    {&pal4::UtilitiesTableBindingRefresh::IdString,
     pal4::UtilitiesBindingSchema::IdString},
    {&pal4::UtilitiesTableBindingRefresh::PreviewInfo,
     pal4::UtilitiesBindingSchema::PreviewInfo},
    {&pal4::UtilitiesTableBindingRefresh::Introduction,
     pal4::UtilitiesBindingSchema::Introduction},
    {&pal4::UtilitiesTableBindingRefresh::Scene,
     pal4::UtilitiesBindingSchema::Scene},
    {&pal4::UtilitiesTableBindingRefresh::Window,
     pal4::UtilitiesBindingSchema::Window},
    {&pal4::UtilitiesTableBindingRefresh::StringId,
     pal4::UtilitiesBindingSchema::StringId},
    {&pal4::UtilitiesTableBindingRefresh::World,
     pal4::UtilitiesBindingSchema::World},
}};

void CheckRebind(const Variant& variant) {
    Runtime runtime;
    variant.refresh(0x1000, runtime);
    Check(runtime.calls == std::vector<Call>{
              {"begin", 0x1034},
              {"assign", 0x106c, 0x1014},
              {"register", 0x106c, 0, variant.schema},
              {"bind", 0x106c},
              {"end", 0x1034}},
          "mismatched counts log, assign, register and bind in order");

    Runtime no_fields;
    no_fields.words[0x1018] = 0;
    variant.refresh(0x1000, no_fields);
    Check(no_fields.calls == runtime.calls,
          "null field array counts as zero and triggers rebind");

    Runtime no_bindings;
    no_bindings.words[0x1080] = 0;
    variant.refresh(0x1000, no_bindings);
    Check(no_bindings.calls == runtime.calls,
          "null binding array counts as zero and triggers rebind");
}

void CheckNoRebind(const Variant& variant) {
    Runtime equal;
    equal.words[0x1084] = 0x3008;
    equal.words[0x101c] = 0x4078;
    variant.refresh(0x1000, equal);
    Check(equal.calls.empty(),
          "equal nonzero counts skip logging and binding");

    Runtime empty;
    empty.words[0x1080] = 0;
    empty.words[0x1018] = 0;
    variant.refresh(0x1000, empty);
    Check(empty.calls.empty(),
          "two null arrays skip binding");

    Runtime equal_larger;
    equal_larger.words[0x1084] = 0x300c;
    equal_larger.words[0x101c] = 0x40b4;
    variant.refresh(0x1000, equal_larger);
    Check(equal_larger.calls.empty(),
          "three 4-byte bindings match three 60-byte fields");
}

}  // namespace

int main() {
    try {
        for (const auto& variant : kVariants) {
            CheckRebind(variant);
            CheckNoRebind(variant);
        }
        std::cout << "utilities_table_binding_refresh_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
