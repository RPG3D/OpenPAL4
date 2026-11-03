#include "pal4/script_dialog_registration.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct Expected {
    std::uint32_t registration;
    std::uint32_t constructor;
    std::uint32_t cleanup;
    std::uint32_t descriptor;
    std::uint32_t handler;
    std::uint32_t description_size;
    std::uint32_t description_hash;
    std::uint32_t signature_size;
    std::string_view signature;
};

constexpr std::array<Expected, pal4::ScriptDialogLateRegistry::kCount>
    kExpected{{
#include "data/script_dialog_late_expected.inc"
    }};

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::uint32_t Hash(std::string_view bytes) {
    std::uint32_t value = 0x811c9dc5u;
    for (const unsigned char byte : bytes)
        value = (value ^ byte) * 0x01000193u;
    return value;
}

class Runtime final : public pal4::IScriptBuiltinRuntime {
public:
    explicit Runtime(const Expected& expected) : expected_(expected) {}

    std::uint32_t Allocate(std::uint32_t size, std::uint32_t type,
                           std::uint32_t protection) override {
        Check(type == 0x1000u && protection == 4u, "allocation flags");
        Check(size == (strings.empty() ? expected_.description_size
                                       : expected_.signature_size),
              "allocation size");
        calls.emplace_back("allocate");
        return strings.empty() ? 0x1000u : 0x2000u;
    }

    void CopyString(std::uint32_t address, std::string_view bytes) override {
        Check(address == (strings.empty() ? 0x1000u : 0x2000u),
              "copy destination");
        strings.emplace_back(bytes);
        calls.emplace_back("copy");
    }

    void Release(std::uint32_t address, std::uint32_t size,
                 std::uint32_t type) override {
        Check(type == 0x8000u, "release flag");
        Check((address == 0x2000u && size == expected_.signature_size) ||
                  (address == 0x1000u && size == expected_.description_size),
              "release address and size");
        calls.emplace_back(address == 0x2000u ? "release signature"
                                                   : "release description");
    }

    void Register(std::uint32_t address,
                  const pal4::ScriptBuiltinDescriptor& descriptor) override {
        Check(address == expected_.descriptor, "original descriptor address");
        registered = descriptor;
        calls.emplace_back("register");
    }

    int RegisterAtExit(std::uint32_t callback) override {
        Check(callback == expected_.cleanup, "original cleanup callback");
        calls.emplace_back("atexit");
        return -43;
    }

    std::vector<std::string> calls;
    std::vector<std::string> strings;
    pal4::ScriptBuiltinDescriptor registered{};

private:
    const Expected& expected_;
};

void CheckEntry(std::size_t index) {
    const auto& expected = kExpected[index];
    const auto& spec = pal4::ScriptDialogLateRegistry::Describe(index);
    Check(spec.registration_entry == expected.registration &&
              spec.constructor_entry == expected.constructor &&
              spec.cleanup_entry == expected.cleanup &&
              spec.descriptor_address == expected.descriptor &&
              spec.handler_address == expected.handler,
          "IDA function and descriptor addresses");
    Check(spec.description_with_nul.size() == expected.description_size &&
              spec.description_with_nul.back() == '\0' &&
              Hash(spec.description_with_nul) == expected.description_hash &&
              spec.signature_with_nul.size() == expected.signature_size &&
              spec.signature_with_nul.back() == '\0' &&
              spec.signature_with_nul.substr(0, expected.signature.size()) ==
                  expected.signature,
          "original GBK description and script prototype");

    Runtime runtime(expected);
    pal4::ScriptDialogLateRegistry registry(runtime);
    Check(registry.Initialize(index) == -43, "signed CRT result");
    Check(runtime.calls == std::vector<std::string>{
              "allocate", "copy", "allocate", "copy", "register", "atexit"},
          "construction and CRT registration order");
    Check(runtime.strings.size() == 2 &&
              runtime.strings[0] == spec.description_with_nul &&
              runtime.strings[1] == spec.signature_with_nul,
          "native copy receives the original bytes including NUL");
    const auto& descriptor = registry.Descriptor(index);
    Check(descriptor.description == 0x1000u &&
              descriptor.description_size == expected.description_size &&
              descriptor.signature == 0x2000u &&
              descriptor.signature_size == expected.signature_size &&
              descriptor.callback_data[0] == expected.handler &&
              std::all_of(descriptor.callback_data.begin() + 1,
                          descriptor.callback_data.end(),
                          [](std::uint32_t word) { return word == 0u; }),
          "48-byte registered descriptor contents");
    Check(runtime.registered.callback_data == descriptor.callback_data,
          "registered payload matches current descriptor");
    registry.Cleanup(index);
    Check(runtime.calls[runtime.calls.size() - 2] == "release signature" &&
              runtime.calls.back() == "release description" &&
              descriptor.signature == 0u && descriptor.description == 0u,
          "reverse release order and cleared descriptor fields");
    const auto call_count = runtime.calls.size();
    registry.Cleanup(index);
    Check(runtime.calls.size() == call_count, "repeat cleanup is inert");

    Runtime direct_runtime(expected);
    pal4::ScriptDialogLateRegistry direct(direct_runtime);
    Check(direct.Construct(index) == expected.descriptor,
          "direct constructor returns the descriptor address");
    Check(direct_runtime.calls.back() == "register",
          "direct constructor does not register CRT cleanup");
    direct.Cleanup(index);
}

void CheckInvalidIndex() {
    Runtime runtime(kExpected[0]);
    pal4::ScriptDialogLateRegistry registry(runtime);
    const auto bad = pal4::ScriptDialogLateRegistry::kCount;
    const auto expect_out_of_range = [](auto&& action) {
        try {
            action();
        } catch (const std::out_of_range&) {
            return;
        }
        throw std::runtime_error("out-of-range index was accepted");
    };
    expect_out_of_range([&] { static_cast<void>(registry.Describe(bad)); });
    expect_out_of_range([&] { static_cast<void>(registry.Initialize(bad)); });
    expect_out_of_range([&] { static_cast<void>(registry.Construct(bad)); });
    expect_out_of_range([&] { registry.Cleanup(bad); });
    expect_out_of_range([&] { static_cast<void>(registry.Descriptor(bad)); });
    Check(runtime.calls.empty(), "invalid indexes make no native calls");
}

}  // namespace

int main() {
    try {
        for (std::size_t index = 0; index < kExpected.size(); ++index)
            CheckEntry(index);
        CheckInvalidIndex();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
