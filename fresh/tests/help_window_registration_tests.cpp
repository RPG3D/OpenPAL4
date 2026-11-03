#include "pal4/help_window_registration.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IHelpWindowRegistrationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8b13acu, 0x8b13b0u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x2000u;
    std::uint32_t allocation_size = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t constructor_name = 0u;
    std::uint32_t factory_name = 0u;
    std::uint32_t factory_address = 0u;
    std::uint32_t tree = 0u;
    std::uint32_t event_key = 0u;
    std::uint32_t handler = 0u;
    std::uint32_t destroyed = 0u;
    bool fail_construct = false;
    bool fail_insert = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocation_size = bytes;
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed = address;
    }
    std::uint32_t Construct(std::uint32_t object,
                            std::uint32_t name) override {
        calls.emplace_back("construct");
        Check(object == 0x2000u, "constructor object");
        constructor_name = name;
        if (fail_construct) throw std::runtime_error("construct");
        return 0x2004u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x3000u;
    }
    std::uint32_t RegisterFactory(std::uint32_t manager,
                                   std::uint32_t name,
                                   std::uint32_t creator) override {
        calls.emplace_back("register-factory");
        Check(manager == 0x3000u, "factory manager");
        factory_name = name;
        factory_address = creator;
        return 0x41u;
    }
    void InsertOrReplace(std::uint32_t target, std::uint32_t key,
                         std::uint32_t value) override {
        calls.emplace_back("insert");
        tree = target;
        event_key = key;
        handler = value;
        if (fail_insert) throw std::runtime_error("insert");
    }
    void DestroyHandler(std::uint32_t value,
                        std::uint32_t flags) override {
        calls.emplace_back("destroy-handler");
        Check(flags == 1u, "deleting destructor flag");
        destroyed = value;
    }
};

void CheckNameFactory() {
    using Registration = pal4::HelpWindowRegistration;
    Runtime runtime;
    Check(Registration::InitializeNameThunk(runtime) == 0x8b13b0u &&
              runtime.words.at(Registration::kStoredName) == 0x8b13b0u &&
              runtime.calls == (std::vector<std::string>{"read", "write"}),
          "name thunk and stored pointer");
    runtime.calls.clear();
    Check(Registration::InitializeName(runtime) == 0x8b13b0u,
          "direct name initializer");
    runtime.calls.clear();
    Check(Registration::RegisterFactoryThunk(runtime) == 0x41u &&
              runtime.factory_name == 0x8b13b0u &&
              runtime.factory_address == Registration::kFactory &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "read", "register-factory"}),
          "factory thunk and address");
    Check(Registration::RegisterFactory(runtime) == 0x41u,
          "direct factory registration");
    runtime.calls.clear();
    Check(Registration::Create(0x9000u, runtime) == 0x2004u &&
              runtime.allocation_size == 0x54u &&
              runtime.constructor_name == 0x9000u &&
              runtime.calls == (std::vector<std::string>{
                  "allocate", "construct"}),
          "object size and constructor forwarding");
}

void CheckShowHandler() {
    using Registration = pal4::HelpWindowRegistration;
    Runtime runtime;
    Check(Registration::RegisterShowThunk(runtime) == 0x8eaca0u &&
              runtime.allocation_size == 8u &&
              runtime.words.at(0x2000u) == 0x8435dcu &&
              runtime.words.at(0x2004u) == 0x4a77a0u &&
              runtime.tree == 0x3104u &&
              runtime.event_key == Registration::kShowEventKey &&
              runtime.handler == 0x2000u &&
              runtime.destroyed == 0x2000u &&
              runtime.calls == (std::vector<std::string>{
                  "allocate", "write", "write", "manager", "insert",
                  "destroy-handler"}),
          "show handler layout and insertion");
    Runtime direct;
    Check(Registration::RegisterShow(direct) == 0x8eaca0u &&
              direct.words.at(0x2004u) == 0x4a77a0u,
          "direct show registration");
    Runtime null_handler;
    null_handler.allocation = 0u;
    Check(Registration::RegisterShow(null_handler) == 0x8eaca0u &&
              null_handler.handler == 0u &&
              null_handler.destroyed == 0u &&
              null_handler.calls == (std::vector<std::string>{
                  "allocate", "manager", "insert"}),
          "null handler insertion");
    Runtime failure;
    failure.fail_insert = true;
    bool caught = false;
    try { (void)Registration::RegisterShow(failure); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && failure.destroyed == 0x2000u &&
              failure.calls.back() == "destroy-handler",
          "handler insertion failure cleanup");
}

void CheckFactoryFailures() {
    using Registration = pal4::HelpWindowRegistration;
    Runtime empty;
    empty.allocation = 0u;
    Check(Registration::Create(0x9000u, empty) == 0u &&
              empty.constructor_name == 0u,
          "null object allocation");
    Runtime failure;
    failure.fail_construct = true;
    bool caught = false;
    try { (void)Registration::Create(0x9000u, failure); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && failure.freed == 0x2000u &&
              failure.calls.back() == "free",
          "constructor failure cleanup");
}

}  // namespace

int main() {
    try {
        CheckNameFactory();
        CheckShowHandler();
        CheckFactoryFailures();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
