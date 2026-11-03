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
    int return_value = -23;

    std::uint32_t Allocate(std::uint32_t size, std::uint32_t type,
                           std::uint32_t protection) override {
        Check(type == 0x1000u && protection == 4u, "allocation flags");
        Check(size == (strings.empty() ? 103u : 38u), "allocation size");
        calls.emplace_back("allocate");
        return strings.empty() ? 0x3000u : 0x4000u;
    }
    void CopyString(std::uint32_t address, std::string_view bytes) override {
        Check(address == (strings.empty() ? 0x3000u : 0x4000u),
              "copy destination");
        strings.emplace_back(bytes);
        calls.emplace_back("copy");
    }
    void Release(std::uint32_t address, std::uint32_t size,
                 std::uint32_t type) override {
        Check(type == 0x8000u, "release flag");
        Check((address == 0x4000u && size == 38u) ||
                  (address == 0x3000u && size == 103u),
              "release address and size");
        calls.emplace_back(address == 0x4000u ? "release signature"
                                                   : "release description");
    }
    void Register(std::uint32_t address,
                  const pal4::ScriptBuiltinDescriptor& descriptor) override {
        Check(address == 0x90f090u, "descriptor address");
        registered = descriptor;
        calls.emplace_back("register");
    }
    int RegisterAtExit(std::uint32_t callback) override {
        Check(callback == 0x5d8ee0u, "atexit callback");
        calls.emplace_back("atexit");
        return return_value;
    }
};

void RegistrationAndCleanup() {
    Runtime runtime;
    pal4::ScriptTalkRegistration talk(runtime);
    Check(talk.Initialize() == -23, "signed atexit result");
    Check(runtime.calls == std::vector<std::string>{
              "allocate", "copy", "allocate", "copy", "register", "atexit"},
          "construction precedes CRT registration");
    Check(runtime.strings.size() == 2 &&
              runtime.strings[0].size() == 103u &&
              Hash(runtime.strings[0]) == 0x721c349fu &&
              runtime.strings[0].back() == '\0',
          "exact original GBK description");
    Check(runtime.strings[1] ==
              std::string("void giTalk( string &in, string &in )\0", 38),
          "exact script prototype");
    const auto& descriptor = talk.Descriptor();
    Check(descriptor.description == 0x3000u &&
              descriptor.description_size == 103u &&
              descriptor.signature == 0x4000u &&
              descriptor.signature_size == 38u &&
              descriptor.callback_data[0] == 0x5dfb10u &&
              std::all_of(descriptor.callback_data.begin() + 1,
                          descriptor.callback_data.end(),
                          [](std::uint32_t value) { return value == 0u; }),
          "descriptor fields and handler");
    Check(runtime.registered.callback_data == descriptor.callback_data,
          "registered callback payload");
    talk.Cleanup();
    Check(runtime.calls[runtime.calls.size() - 2] == "release signature" &&
              runtime.calls.back() == "release description" &&
              descriptor.signature == 0u && descriptor.description == 0u,
          "shared cleanup releases in reverse order and clears descriptor");
    const auto count = runtime.calls.size();
    talk.Cleanup();
    Check(runtime.calls.size() == count, "repeated cleanup is inert");
}

}  // namespace

int main() {
    try {
        RegistrationAndCleanup();
        Runtime direct_runtime;
        pal4::ScriptTalkRegistration direct(direct_runtime);
        Check(direct.Construct() == direct.kDescriptorAddress,
              "direct giTalk constructor returns descriptor address");
        direct.Cleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
