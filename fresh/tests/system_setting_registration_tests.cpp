#include "pal4/system_setting_registration.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemSettingRegistrationRuntime {
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x4000u;
    bool throw_insert = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == pal4::SystemSettingRegistration::kNameSource,
              "SystemSetting source name");
        calls.emplace_back("name");
        return 0x5000u;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 8u, "registration callback size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check((address == 0x4004u && value == 0x5000u) ||
                  (address == 0x4000u &&
                   value == pal4::SystemSettingRegistration::kHandlerVtable),
              "handler name and vtable fields");
        calls.emplace_back(address == 0x4004u ? "write-name" :
                                               "write-vtable");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x6000u;
    }
    void InsertOrReplace(std::uint32_t tree,
                         std::uint32_t key,
                         std::uint32_t handler) override {
        Check(tree == 0x6104u &&
                  key == pal4::SystemSettingRegistration::kEventKey &&
                  handler == allocation,
              "show_system_setting insertion arguments");
        calls.emplace_back("insert");
        if (throw_insert) throw std::runtime_error("insert");
    }
    void ReleaseHandler(std::uint32_t handler,
                        std::uint32_t flags) override {
        Check(handler == allocation && flags == 1u,
              "temporary handler release flag");
        calls.emplace_back("release");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::SystemSettingRegistration::GetHandler(runtime) ==
              pal4::SystemSettingRegistration::kStorage &&
              runtime.calls == std::vector<std::string>{
                                   "name", "allocate", "write-name",
                                   "write-vtable", "manager", "insert",
                                   "release"},
          "thunk builds and registers SystemSetting handler in order");

    Runtime empty;
    empty.allocation = 0u;
    Check(pal4::SystemSettingRegistration::RegisterHandler(empty) ==
              pal4::SystemSettingRegistration::kStorage &&
              empty.calls == std::vector<std::string>{
                                 "name", "allocate", "manager", "insert"},
          "null allocation inserts null handler without release");

    Runtime failed;
    failed.throw_insert = true;
    bool caught = false;
    try {
        (void)pal4::SystemSettingRegistration::RegisterHandler(failed);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failed.calls.back() == "release",
          "insertion failure releases temporary handler");
}
