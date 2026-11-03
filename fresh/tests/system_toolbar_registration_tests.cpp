#include "pal4/system_toolbar_registration.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemToolbarRegistrationRuntime {
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x5000u;
    std::uint32_t key = 0u;
    std::uint32_t callback = 0u;
    std::uint32_t vtable = 0u;
    bool throw_insert = false;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(address == 0x8b07d8u, "main handler name source");
        calls.emplace_back("name");
        return 0x7000u;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 8u, "handler allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        if (address == allocation + 4u) {
            callback = value;
            calls.emplace_back("callback");
        } else {
            Check(address == allocation, "handler vtable address");
            vtable = value;
            calls.emplace_back("vtable");
        }
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x6000u;
    }
    void InsertOrReplace(const std::uint32_t tree,
                         const std::uint32_t event_key,
                         const std::uint32_t handler) override {
        Check(tree == 0x6104u && handler == allocation,
              "manager tree and handler");
        key = event_key;
        calls.emplace_back("insert");
        if (throw_insert) throw std::runtime_error("insert");
    }
    void ReleaseHandler(const std::uint32_t handler,
                        const std::uint32_t flags) override {
        Check(handler == allocation && flags == 1u,
              "temporary handler release");
        calls.emplace_back("release");
    }
};

using HandlerEntry = std::uint32_t (*)(
    pal4::ISystemToolbarRegistrationRuntime&);

struct Case {
    HandlerEntry thunk;
    HandlerEntry direct;
    std::uint32_t storage;
    std::uint32_t key;
    std::uint32_t callback;
    std::uint32_t vtable;
    bool main;
};

constexpr std::array kCases{
    Case{pal4::SystemToolbarRegistration::GetMainHandler,
         pal4::SystemToolbarRegistration::RegisterMainHandler,
         0x8e797bu, 0x8a80acu, 0x7000u, 0x843494u, true},
    Case{pal4::SystemToolbarRegistration::GetButton1Handler,
         pal4::SystemToolbarRegistration::RegisterButton1Handler,
         0x8e797au, 0x8a80b0u, 0x482f50u, 0x8435dcu, false},
    Case{pal4::SystemToolbarRegistration::GetButton2Handler,
         pal4::SystemToolbarRegistration::RegisterButton2Handler,
         0x8e7979u, 0x8a80b4u, 0x482fb0u, 0x8435dcu, false},
    Case{pal4::SystemToolbarRegistration::GetButton3Handler,
         pal4::SystemToolbarRegistration::RegisterButton3Handler,
         0x8e7978u, 0x8a80b8u, 0x482fe0u, 0x8435dcu, false}};

void TestAllHandlers() {
    for (const auto& entry : kCases) {
        const auto expected = entry.main ?
            std::vector<std::string>{"name", "allocate", "callback",
                                     "vtable", "manager", "insert", "release"} :
            std::vector<std::string>{"allocate", "callback", "vtable",
                                     "manager", "insert", "release"};
        for (const auto call : {entry.thunk, entry.direct}) {
            Runtime runtime;
            Check(call(runtime) == entry.storage &&
                      runtime.key == entry.key &&
                      runtime.callback == entry.callback &&
                      runtime.vtable == entry.vtable &&
                      runtime.calls == expected,
                  "thunk and direct handler registration");
        }
    }
}

void TestNullAndException() {
    for (const auto& entry : kCases) {
        Runtime empty;
        empty.allocation = 0u;
        Check(entry.direct(empty) == entry.storage &&
                  empty.key == entry.key &&
                  empty.calls.back() == "insert",
              "null allocation inserts null without release");
        Runtime failed;
        failed.throw_insert = true;
        bool caught = false;
        try {
            (void)entry.direct(failed);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && failed.calls.back() == "release",
              "insert exception releases temporary handler");
    }
}

}  // namespace

int main() {
    TestAllHandlers();
    TestNullAndException();
    return EXIT_SUCCESS;
}
