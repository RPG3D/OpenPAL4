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
    int exit_result = -17;
    std::uint32_t next_allocation = 0x1000u;

    std::uint32_t Allocate(std::uint32_t size, std::uint32_t type,
                           std::uint32_t protection) override {
        Check(type == 0x1000u && protection == 4u, "VirtualAlloc flags");
        Check(size == (next_allocation == 0x1000u ? 127u : 22u),
              "allocated string length including NUL");
        calls.emplace_back("allocate");
        const auto address = next_allocation;
        next_allocation += 0x1000u;
        return address;
    }
    void CopyString(std::uint32_t address, std::string_view bytes) override {
        Check(address == (strings.empty() ? 0x1000u : 0x2000u),
              "copy into matching allocation");
        strings.emplace_back(bytes);
        calls.emplace_back("copy");
    }
    void Release(std::uint32_t address, std::uint32_t size,
                 std::uint32_t type) override {
        Check(type == 0x8000u, "VirtualFree release flag");
        Check((address == 0x2000u && size == 22u) ||
                  (address == 0x1000u && size == 127u),
              "cleanup uses original string allocation");
        calls.emplace_back(address == 0x2000u ? "release signature"
                                                   : "release description");
    }
    void Register(std::uint32_t address,
                  const pal4::ScriptBuiltinDescriptor& descriptor) override {
        Check(address == 0x90f0c8u, "original descriptor address");
        registered = descriptor;
        calls.emplace_back("register");
    }
    int RegisterAtExit(std::uint32_t callback) override {
        Check(callback == 0x5d8d80u, "original cleanup callback");
        calls.emplace_back("atexit");
        return exit_result;
    }
};

void RegistrationAndCleanup() {
    Runtime runtime;
    pal4::ScriptWaitRegistration wait(runtime);
    Check(wait.Initialize() == -17, "signed atexit result");
    Check(runtime.calls == std::vector<std::string>{
              "allocate", "copy", "allocate", "copy", "register", "atexit"},
          "constructor completes before atexit registration");
    Check(runtime.strings.size() == 2 &&
              runtime.strings[0].size() == 127u &&
              Hash(runtime.strings[0]) == 0x56af5efcu &&
              runtime.strings[0].back() == '\0',
          "exact IDA description bytes including NUL");
    Check(runtime.strings[1] == std::string("void giWait( double )\0", 22),
          "exact script prototype including NUL");
    const auto& descriptor = wait.Descriptor();
    Check(descriptor.description == 0x1000u &&
              descriptor.description_size == 127u &&
              descriptor.signature == 0x2000u &&
              descriptor.signature_size == 22u &&
              descriptor.callback_data[0] == 0x5dfa30u &&
              std::all_of(descriptor.callback_data.begin() + 1,
                          descriptor.callback_data.end(),
                          [](std::uint32_t value) { return value == 0u; }),
          "48-byte descriptor fields and handler");
    Check(runtime.registered.callback_data == descriptor.callback_data,
          "registered descriptor carries handler");
    wait.Cleanup();
    Check(runtime.calls[runtime.calls.size() - 2] == "release signature" &&
              runtime.calls.back() == "release description" &&
              wait.Descriptor().signature == 0u &&
              wait.Descriptor().description == 0u,
          "cleanup releases signature before description and clears pointers");
    const auto count = runtime.calls.size();
    wait.Cleanup();
    Check(runtime.calls.size() == count, "second cleanup is inert");
}

}  // namespace

int main() {
    try {
        RegistrationAndCleanup();
        Runtime direct_runtime;
        pal4::ScriptWaitRegistration direct(direct_runtime);
        Check(direct.Construct() == direct.kDescriptorAddress,
              "direct giWait constructor returns descriptor address");
        direct.Cleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
