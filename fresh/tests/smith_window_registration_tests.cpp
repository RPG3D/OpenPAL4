#include "pal4/smith_window_registration.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowRegistrationRuntime {
    std::uint32_t name = 0x8b1000u;
    std::uint32_t registered = 0xffffffffu;
    std::uint32_t register_result = 0x6000u;
    std::uint32_t allocated_handler = 0x7000u;
    bool throw_on_insert = false;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(address == 0x8b0f20u || address == 0x8eac6cu,
              "SmithWindow name pointer or registration slot");
        calls.emplace_back("read");
        return address == 0x8b0f20u ? name : registered;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(address == 0x8eac6cu || address == allocated_handler + 4u ||
                  address == allocated_handler,
              "SmithWindow registration or handler field");
        if (address == 0x8eac6cu) {
            registered = value;
        } else if (address == allocated_handler + 4u) {
            Check(value == name, "handler stores SmithWindow name");
        } else {
            Check(value == 0x843494u, "handler vtable");
        }
        calls.emplace_back("write");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    std::uint32_t RegisterFactory(std::uint32_t manager,
                                  std::uint32_t name_value,
                                  std::uint32_t factory) override {
        Check(manager == 0x5000u && name_value == registered &&
                  factory == 0x496ae0u,
              "registers SmithWindow factory with initialized name");
        calls.emplace_back("register");
        return register_result;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 8u, "handler allocation size");
        calls.emplace_back("allocate");
        return allocated_handler;
    }
    void InsertOrReplace(std::uint32_t tree, std::uint32_t key,
                         std::uint32_t handler) override {
        Check(tree == 0x5104u && key == 0x8b0f1cu &&
                  handler == allocated_handler,
              "event tree, key and temporary handler");
        calls.emplace_back("insert");
        if (throw_on_insert)
            throw std::runtime_error("insert failed");
    }
    void DestroyHandler(std::uint32_t handler,
                        std::uint32_t flags) override {
        Check(handler == allocated_handler && flags == 1u,
              "temporary handler destruction flags");
        calls.emplace_back("destroy");
    }
};

void CheckEntries() {
    Runtime direct;
    Check(pal4::SmithWindowRegistration::Initialize(direct) == direct.name &&
              direct.registered == direct.name &&
              direct.calls == std::vector<std::string>{"read", "write"},
          "initializer writes name pointer and returns it");

    Runtime thunk;
    thunk.name = 0u;
    Check(pal4::SmithWindowRegistration::InitializeThunk(thunk) == 0u &&
              thunk.registered == 0u &&
              thunk.calls == std::vector<std::string>{"read", "write"},
          "jump thunk forwards unchanged return and side effect");

    direct.calls.clear();
    Check(pal4::SmithWindowRegistration::RegisterUI(direct) ==
              direct.register_result &&
              direct.calls ==
                  std::vector<std::string>{"manager", "read", "register"},
          "registration reads initialized name after manager lookup");
    direct.calls.clear();
    direct.register_result = 0u;
    Check(pal4::SmithWindowRegistration::RegisterUIThunk(direct) == 0u &&
              direct.calls ==
                  std::vector<std::string>{"manager", "read", "register"},
          "registration jump thunk forwards return and side effects");
}

void CheckEventHandler() {
    Runtime success;
    Check(pal4::SmithWindowRegistration::RegisterEventHandler(success) ==
              0x8eac68u &&
              success.calls == (std::vector<std::string>{
                  "read", "allocate", "write", "write",
                  "manager", "insert", "destroy"}),
          "event registration builds, inserts and destroys temporary handler");

    Runtime allocation_failure;
    allocation_failure.allocated_handler = 0u;
    Check(pal4::SmithWindowRegistration::RegisterEventHandlerThunk(
              allocation_failure) == 0x8eac68u &&
              allocation_failure.calls ==
                  (std::vector<std::string>{
                      "read", "allocate", "manager", "insert"}),
          "event thunk still inserts null handler after failed allocation");

    Runtime insert_failure;
    insert_failure.throw_on_insert = true;
    bool threw = false;
    try {
        (void)pal4::SmithWindowRegistration::RegisterEventHandler(
            insert_failure);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && insert_failure.calls.back() == "destroy",
          "temporary handler is destroyed when insertion throws");
}

}  // namespace

int main() {
    try {
        CheckEntries();
        CheckEventHandler();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
