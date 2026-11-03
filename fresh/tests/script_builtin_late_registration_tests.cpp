#include "pal4/script_builtin_late_registration.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Expected {
    pal4::StaticBuiltin builtin;
    std::uint32_t descriptor;
    std::uint32_t registration_entry;
    std::uint32_t cleanup_callback;
};

constexpr std::array<Expected,
                     static_cast<std::size_t>(pal4::StaticBuiltin::Count)> kExpected{{
#include "data/script_builtin_late_expected.inc"
}};

struct Runtime final : pal4::IScriptBuiltinLateRuntime {
    std::vector<std::string> calls;
    pal4::StaticBuiltin builtin = pal4::StaticBuiltin::Count;
    std::uint32_t descriptor = 0u;
    std::uint32_t registration_entry = 0u;
    std::uint32_t cleanup_callback = 0u;
    std::uint32_t deallocated = 0u;
    bool fail_registration = false;
    bool fail_atexit = false;

    void RegisterBuiltin(pal4::StaticBuiltin entry,
                         std::uint32_t address,
                         std::uint32_t native_entry) override {
        calls.emplace_back("register");
        builtin = entry;
        descriptor = address;
        registration_entry = native_entry;
        if (fail_registration) throw std::runtime_error("register");
    }
    int RegisterAtExit(std::uint32_t callback) override {
        calls.emplace_back("atexit");
        cleanup_callback = callback;
        if (fail_atexit) throw std::runtime_error("atexit");
        return 17;
    }
    void DeallocateDescriptor(std::uint32_t address) override {
        calls.emplace_back("deallocate");
        deallocated = address;
    }
};

void CheckAllEntries() {
    using Registration = pal4::ScriptBuiltinLateRegistration;
    for (const auto& expected : kExpected) {
        const auto spec = Registration::SpecFor(expected.builtin);
        Check(spec.descriptor == expected.descriptor &&
                  spec.registration_entry == expected.registration_entry &&
                  spec.cleanup_callback == expected.cleanup_callback,
              "IDA descriptor, target and cleanup mapping");
        Runtime initialize;
        Check(Registration::Initialize(expected.builtin, initialize) == 17 &&
                  initialize.calls == (std::vector<std::string>{
                      "register", "atexit"}) &&
                  initialize.builtin == expected.builtin &&
                  initialize.descriptor == expected.descriptor &&
                  initialize.registration_entry ==
                      expected.registration_entry &&
                  initialize.cleanup_callback == expected.cleanup_callback,
              "initializer registers then installs exact cleanup callback");
        Runtime direct;
        Registration::Register(expected.builtin, direct);
        Check(direct.calls == (std::vector<std::string>{"register"}) &&
                  direct.descriptor == expected.descriptor &&
                  direct.registration_entry == expected.registration_entry,
              "direct registration wrapper");
        Registration::Cleanup(expected.builtin, direct);
        Check(direct.calls == (std::vector<std::string>{
                  "register", "deallocate"}) &&
                  direct.deallocated == expected.descriptor,
              "cleanup wrapper releases matching descriptor");
    }
}

void CheckFailurePaths() {
    using Registration = pal4::ScriptBuiltinLateRegistration;
    bool caught = false;
    try { (void)Registration::SpecFor(pal4::StaticBuiltin::Count); }
    catch (const std::out_of_range&) { caught = true; }
    Check(caught, "unmapped entry is rejected");
    const auto first = Registration::SpecFor(pal4::StaticBuiltin::ImmBegin);
    const auto last = Registration::SpecFor(
        pal4::StaticBuiltin::ScriptMusicResume);
    Check(first.descriptor == 0x8edf40u &&
              first.registration_entry == 0x45ad50u &&
              last.descriptor == 0x8eacb8u &&
              last.registration_entry == 0x46e540u &&
              last.cleanup_callback == 0x4a5fc0u,
          "independent first and last IDA anchors");
    Runtime failed_registration;
    failed_registration.fail_registration = true;
    caught = false;
    try {
        (void)Registration::Initialize(pal4::StaticBuiltin::ImmBegin,
                                       failed_registration);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && failed_registration.calls ==
              (std::vector<std::string>{"register"}),
          "registration failure skips atexit");
    Runtime failed_atexit;
    failed_atexit.fail_atexit = true;
    caught = false;
    try {
        (void)Registration::Initialize(pal4::StaticBuiltin::ArenaReady,
                                       failed_atexit);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && failed_atexit.calls ==
              (std::vector<std::string>{"register", "atexit"}),
          "atexit failure happens after descriptor registration");
}

}  // namespace

int main() {
    try {
        CheckAllEntries();
        CheckFailurePaths();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
