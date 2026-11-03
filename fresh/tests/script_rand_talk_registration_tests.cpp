#include "pal4/script_dialog_registration.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::uint32_t Hash(std::string_view bytes) {
    std::uint32_t value = 0x811c9dc5u;
    for (const unsigned char byte : bytes)
        value = (value ^ byte) * 0x01000193u;
    return value;
}

struct Expected {
    std::uint32_t descriptor;
    std::uint32_t handler;
    std::uint32_t callback;
    std::size_t description_size;
    std::uint32_t description_hash;
    std::string signature;
};

struct Runtime final : pal4::IScriptBuiltinRuntime {
    explicit Runtime(const Expected& expected) : expected(expected) {}

    const Expected& expected;
    std::vector<std::string> calls;
    std::vector<std::string> strings;
    pal4::ScriptBuiltinDescriptor registered{};

    std::uint32_t Allocate(std::uint32_t size, std::uint32_t type,
                           std::uint32_t protection) override {
        Check(type == 0x1000u && protection == 4u, "VirtualAlloc flags");
        Check(size == (strings.empty() ? expected.description_size
                                       : expected.signature.size()),
              "allocation sizes include NUL");
        calls.emplace_back("allocate");
        return strings.empty() ? 0x1000u : 0x2000u;
    }
    void CopyString(std::uint32_t destination,
                    std::string_view bytes) override {
        Check(destination == (strings.empty() ? 0x1000u : 0x2000u),
              "copy destination");
        strings.emplace_back(bytes);
        calls.emplace_back("copy");
    }
    void Release(std::uint32_t address, std::uint32_t size,
                 std::uint32_t type) override {
        Check(type == 0x8000u, "VirtualFree release flag");
        Check((address == 0x2000u && size == expected.signature.size()) ||
                  (address == 0x1000u && size == expected.description_size),
              "cleanup uses original address and size");
        calls.emplace_back(address == 0x2000u ? "release signature"
                                                   : "release description");
    }
    void Register(std::uint32_t address,
                  const pal4::ScriptBuiltinDescriptor& descriptor) override {
        Check(address == expected.descriptor, "original descriptor address");
        registered = descriptor;
        calls.emplace_back("register");
    }
    int RegisterAtExit(std::uint32_t callback) override {
        Check(callback == expected.callback, "original cleanup callback");
        calls.emplace_back("atexit");
        return -27;
    }
};

template <typename Registration>
void CheckLifecycle(const Expected& expected) {
    Runtime runtime(expected);
    Registration registration(runtime);
    Check(registration.Initialize() == -27, "signed atexit result");
    Check(runtime.calls == std::vector<std::string>{
              "allocate", "copy", "allocate", "copy", "register", "atexit"},
          "constructor completes before CRT registration");
    Check(runtime.strings.size() == 2 &&
              runtime.strings[0].size() == expected.description_size &&
              Hash(runtime.strings[0]) == expected.description_hash &&
              runtime.strings[0].back() == '\0' &&
              runtime.strings[1] == expected.signature,
          "exact IDA GBK bytes and prototype including NUL");
    const auto& descriptor = registration.Descriptor();
    Check(descriptor.description == 0x1000u &&
              descriptor.description_size == expected.description_size &&
              descriptor.signature == 0x2000u &&
              descriptor.signature_size == expected.signature.size() &&
              descriptor.callback_data[0] == expected.handler &&
              std::all_of(descriptor.callback_data.begin() + 1,
                          descriptor.callback_data.end(),
                          [](std::uint32_t value) { return value == 0u; }),
          "48-byte descriptor contents");
    Check(runtime.registered.callback_data == descriptor.callback_data,
          "registered descriptor carries original handler");
    registration.Cleanup();
    Check(runtime.calls[runtime.calls.size() - 2] == "release signature" &&
              runtime.calls.back() == "release description" &&
              descriptor.signature == 0u && descriptor.description == 0u,
          "cleanup order and cleared fields");
    const auto call_count = runtime.calls.size();
    registration.Cleanup();
    Check(runtime.calls.size() == call_count, "repeated cleanup is inert");

    Runtime direct_runtime(expected);
    Registration direct(direct_runtime);
    Check(direct.Construct() == expected.descriptor,
          "direct constructor returns descriptor address");
    Check(direct_runtime.calls.back() == "register",
          "direct construction does not register CRT cleanup");
    direct.Cleanup();
}

}  // namespace

int main() {
    try {
        CheckLifecycle<pal4::ScriptRandTalkRegistration>(
            {0x90f020u, 0x5dfee0u, 0x5d91a0u, 49u, 0x26327d42u,
             std::string("void giRandTalk()\0", 18)});
        CheckLifecycle<pal4::ScriptRandTalkReleaseRegistration>(
            {0x90efe8u, 0x5dffc0u, 0x5d9300u, 25u, 0x73e725d5u,
             std::string("void giRandTalkRelease()\0", 25)});
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
