#include "pal4/repeat_game_window_registration.h"

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

struct Runtime final : pal4::IRepeatGameWindowRegistrationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8b1324u, 0x8b1328u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x2000u;
    std::uint32_t size = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t constructed_layout = 0u;
    std::uint32_t factory_name = 0u;
    std::uint32_t factory_address = 0u;
    std::uint32_t tree = 0u;
    std::uint32_t key = 0u;
    std::uint32_t handler = 0u;
    std::uint32_t destroyed = 0u;
    bool throw_construct = false;
    bool throw_insert = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        size = bytes;
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed = address;
    }
    std::uint32_t Construct(std::uint32_t object,
                            std::uint32_t layout_name) override {
        calls.emplace_back("construct");
        Check(object == allocation, "constructor receives allocation");
        constructed_layout = layout_name;
        if (throw_construct) throw std::runtime_error("construct");
        return object + 4u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x3000u;
    }
    std::uint32_t RegisterFactory(std::uint32_t manager,
                                   std::uint32_t name,
                                   std::uint32_t factory) override {
        calls.emplace_back("register");
        Check(manager == 0x3000u, "factory manager");
        factory_name = name;
        factory_address = factory;
        return 0x55u;
    }
    void InsertOrReplace(std::uint32_t target, std::uint32_t event_key,
                         std::uint32_t value) override {
        calls.emplace_back("insert");
        tree = target;
        key = event_key;
        handler = value;
        if (throw_insert) throw std::runtime_error("insert");
    }
    void DestroyHandler(std::uint32_t value,
                        std::uint32_t flags) override {
        calls.emplace_back("destroy");
        Check(flags == 1u, "deleting destructor flag");
        destroyed = value;
    }
};

void CheckNameAndFactory() {
    Runtime runtime;
    using Registration = pal4::RepeatGameWindowRegistration;
    Check(Registration::InitializeNameThunk(runtime) == 0x8b1328u &&
              runtime.words.at(0x8eac90u) == 0x8b1328u &&
              runtime.calls == (std::vector<std::string>{"read", "write"}),
          "name thunk stores RepeatGameWindow pointer");
    runtime.calls.clear();
    Check(Registration::RegisterFactoryThunk(runtime) == 0x55u &&
              runtime.factory_name == 0x8b1328u &&
              runtime.factory_address == Registration::kFactory &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "read", "register"}),
          "factory thunk registers native constructor address");
}

void CheckCreate() {
    using Registration = pal4::RepeatGameWindowRegistration;
    Runtime runtime;
    Check(Registration::Create(0x9000u, runtime) == 0x2004u &&
              runtime.size == 0x38u &&
              runtime.constructed_layout == 0x9000u &&
              runtime.calls == (std::vector<std::string>{
                  "allocate", "construct"}),
          "factory allocates and calls constructor");
    Runtime empty;
    empty.allocation = 0u;
    Check(Registration::Create(0x9000u, empty) == 0u &&
              empty.calls == (std::vector<std::string>{"allocate"}),
          "allocation failure skips constructor");
    Runtime throwing;
    throwing.throw_construct = true;
    bool caught = false;
    try { (void)Registration::Create(0x9000u, throwing); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && throwing.freed == 0x2000u &&
              throwing.calls.back() == "free",
          "constructor exception frees allocation");
}

void CheckHandler() {
    using Registration = pal4::RepeatGameWindowRegistration;
    Runtime runtime;
    Check(Registration::RegisterHandlerThunk(runtime) == 0x8eac8cu &&
              runtime.size == 8u &&
              runtime.words.at(0x2000u) == Registration::kHandlerVtable &&
              runtime.words.at(0x2004u) == 0x8b1328u &&
              runtime.tree == 0x3104u &&
              runtime.key == Registration::kEventKey &&
              runtime.handler == 0x2000u && runtime.destroyed == 0x2000u &&
              runtime.calls == (std::vector<std::string>{
                  "read", "allocate", "write", "write", "manager",
                  "insert", "destroy"}),
          "event handler registration and temporary ownership");
    Runtime empty;
    empty.allocation = 0u;
    Check(Registration::RegisterHandler(empty) == 0x8eac8cu &&
              empty.handler == 0u && empty.destroyed == 0u,
          "null handler still passes through insertion");
    Runtime throwing;
    throwing.throw_insert = true;
    bool caught = false;
    try { (void)Registration::RegisterHandler(throwing); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && throwing.destroyed == 0x2000u &&
              throwing.calls.back() == "destroy",
          "insertion exception releases handler");
}

}  // namespace

int main() {
    try {
        CheckNameAndFactory();
        CheckCreate();
        CheckHandler();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
