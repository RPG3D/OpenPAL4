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
    std::uint32_t hash = 0x811c9dc5u;
    for (const unsigned char byte : bytes)
        hash = (hash ^ byte) * 0x01000193u;
    return hash;
}

struct Runtime final : pal4::IScriptBuiltinRuntime {
    std::vector<std::string> calls;
    std::vector<std::string> strings;
    pal4::ScriptBuiltinDescriptor registered{};

    std::uint32_t Allocate(std::uint32_t size, std::uint32_t type,
                           std::uint32_t protection) override {
        Check(type == 0x1000u && protection == 4u, "allocation flags");
        Check(size == (strings.empty() ? 75u : 34u), "allocation size");
        calls.emplace_back("allocate");
        return strings.empty() ? 0x5000u : 0x6000u;
    }
    void CopyString(std::uint32_t address, std::string_view bytes) override {
        Check(address == (strings.empty() ? 0x5000u : 0x6000u),
              "copy destination");
        strings.emplace_back(bytes);
        calls.emplace_back("copy");
    }
    void Release(std::uint32_t address, std::uint32_t size,
                 std::uint32_t type) override {
        Check(type == 0x8000u, "release flag");
        Check((address == 0x6000u && size == 34u) ||
                  (address == 0x5000u && size == 75u),
              "release address and size");
        calls.emplace_back(address == 0x6000u ? "release signature"
                                                   : "release description");
    }
    void Register(std::uint32_t address,
                  const pal4::ScriptBuiltinDescriptor& descriptor) override {
        Check(address == 0x90f058u, "descriptor address");
        registered = descriptor;
        calls.emplace_back("register");
    }
    int RegisterAtExit(std::uint32_t callback) override {
        Check(callback == 0x5d9040u, "atexit callback");
        calls.emplace_back("atexit");
        return -31;
    }
};

void RegistrationAndCleanup() {
    Runtime runtime;
    pal4::ScriptRandTalkPushRegistration push(runtime);
    Check(push.Initialize() == -31, "signed atexit result");
    Check(runtime.calls == std::vector<std::string>{
              "allocate", "copy", "allocate", "copy", "register", "atexit"},
          "construction precedes CRT registration");
    Check(runtime.strings.size() == 2 &&
              runtime.strings[0].size() == 75u &&
              Hash(runtime.strings[0]) == 0x57ab912du &&
              runtime.strings[0].back() == '\0',
          "exact original GBK description");
    Check(runtime.strings[1] ==
              std::string("void giRandTalkPush( string &in )\0", 34),
          "exact script prototype");
    const auto& descriptor = push.Descriptor();
    Check(descriptor.description == 0x5000u &&
              descriptor.description_size == 75u &&
              descriptor.signature == 0x6000u &&
              descriptor.signature_size == 34u &&
              descriptor.callback_data[0] == 0x5dfd30u &&
              std::all_of(descriptor.callback_data.begin() + 1,
                          descriptor.callback_data.end(),
                          [](std::uint32_t value) { return value == 0u; }),
          "descriptor fields and handler");
    Check(runtime.registered.callback_data == descriptor.callback_data,
          "registered callback payload");
    push.Cleanup();
    Check(runtime.calls[runtime.calls.size() - 2] == "release signature" &&
              runtime.calls.back() == "release description" &&
              descriptor.signature == 0u && descriptor.description == 0u,
          "cleanup releases in reverse order and clears descriptor");
    const auto count = runtime.calls.size();
    push.Cleanup();
    Check(runtime.calls.size() == count, "repeated cleanup is inert");
}

}  // namespace

int main() {
    try {
        RegistrationAndCleanup();
        Runtime direct_runtime;
        pal4::ScriptRandTalkPushRegistration direct(direct_runtime);
        Check(direct.Construct() == direct.kDescriptorAddress,
              "direct giRandTalkPush constructor returns descriptor address");
        direct.Cleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
