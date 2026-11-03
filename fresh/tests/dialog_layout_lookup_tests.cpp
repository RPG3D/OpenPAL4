#include "pal4/dialog_layout_base.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IDialogLayoutLookupRuntime {
    std::vector<std::string> events;
    std::string source;
    std::string converted;
    std::u32string name;

    std::string ConvertToUtf8(
        const std::string& value) override {
        source = value;
        events.emplace_back("convert");
        return converted;
    }
    std::uint32_t GetWindow(
        const std::u32string& value) override {
        name = value;
        events.emplace_back("lookup");
        return 0x12345678u;
    }
};

void CheckLookup() {
    for (const auto& pair : {
             std::pair<std::string, std::u32string>{"", U""},
             {"MainWindow", U"MainWindow"},
             {"\xe4\xb8\xad\xe6\x96\x87", U"\u4e2d\u6587"},
             {"A\xf0\x9f\x98\x80", U"A\U0001f600"},
             {std::string("A\0ignored", 9u), U"A"}}) {
        Runtime runtime;
        runtime.converted = pair.first;
        Check(pal4::DialogLayoutBase::GetWindow(
                  "source", runtime) == 0x12345678u &&
                  runtime.source == "source" &&
                  runtime.name == pair.second &&
                  runtime.events ==
                      std::vector<std::string>{
                          "convert", "lookup"},
              "encoded wrapper conversion and lookup");

        Runtime direct;
        Check(pal4::DialogLayoutBase::GetWindowByName(
                  pair.first, direct) == 0x12345678u &&
                  direct.name == pair.second &&
                  direct.events ==
                      std::vector<std::string>{"lookup"},
              "UTF-8 lookup directly decodes name");
    }
}

void CheckTruncated() {
    Runtime runtime;
    bool threw = false;
    try {
        (void)pal4::DialogLayoutBase::GetWindowByName(
            "\xe4\xb8", runtime);
    } catch (const std::length_error&) {
        threw = true;
    }
    Check(threw && runtime.events.empty(),
          "truncated UTF-8 stops before native lookup");
}

struct VirtualRuntime final : pal4::IDialogLayoutVirtualRuntime {
    std::vector<std::uint32_t> offsets;
    std::uint32_t Invoke(std::uint32_t object,
                         std::uint32_t offset) override {
        Check(object == 0x1000u, "virtual call owner");
        offsets.push_back(offset);
        return 0x2000u + offset;
    }
};

void CheckVirtualDispatch() {
    VirtualRuntime runtime;
    Check(pal4::DialogLayoutBase::InvokeVirtual12(
              0x1000u, runtime) == 0x200cu &&
              pal4::DialogLayoutBase::InvokeVirtual16(
                  0x1000u, runtime) == 0x2010u &&
              pal4::DialogLayoutBase::InvokeVirtual20(
                  0x1000u, runtime) == 0x2014u &&
              runtime.offsets ==
                  std::vector<std::uint32_t>{12u, 16u, 20u},
          "three virtual thunks forward receiver, slot and return");
}

}  // namespace

int main() {
    try {
        CheckLookup();
        CheckTruncated();
        CheckVirtualDispatch();
        std::cout << "dialog_layout_lookup_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
