#include "pal4/click_game_window_registration.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IClickGameWindowRegistrationRuntime {
    static constexpr std::uint32_t name = 0x7000u;
    static constexpr std::uint32_t object = 0x2000u;
    static constexpr std::uint32_t manager = 0x3000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8b1208u, name}};
    std::uint32_t allocation = object;
    std::uint32_t bytes = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t constructed_layout = 0u;
    std::uint32_t registered_name = 0u;
    std::uint32_t registered_factory = 0u;
    std::uint32_t inserted_tree = 0u;
    std::uint32_t inserted_key = 0u;
    std::uint32_t inserted_handler = 0u;
    std::uint32_t destroyed = 0u;
    bool throw_construct = false;
    bool throw_insert = false;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.emplace_back("allocate");
        bytes = size;
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed = address;
    }
    std::uint32_t Construct(std::uint32_t address,
                            std::uint32_t layout_name) override {
        calls.emplace_back("construct");
        Check(address == object, "constructor object");
        constructed_layout = layout_name;
        if (throw_construct) throw std::runtime_error("construct failed");
        return address + 4u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    std::uint32_t RegisterFactory(std::uint32_t target,
                                   std::uint32_t window_name,
                                   std::uint32_t factory) override {
        calls.emplace_back("register");
        Check(target == manager, "factory manager");
        registered_name = window_name;
        registered_factory = factory;
        return 0xa5u;
    }
    void InsertOrReplace(std::uint32_t tree,
                         std::uint32_t key,
                         std::uint32_t handler) override {
        calls.emplace_back("insert");
        inserted_tree = tree;
        inserted_key = key;
        inserted_handler = handler;
        if (throw_insert) throw std::runtime_error("insert failed");
    }
    void DestroyHandler(std::uint32_t handler,
                        std::uint32_t flags) override {
        calls.emplace_back("destroy");
        Check(flags == 1u, "handler deleting flag");
        destroyed = handler;
    }
};

void CheckNameAndFactory() {
    Runtime runtime;
    Check(pal4::ClickGameWindowRegistration::InitializeNameThunk(runtime) ==
              Runtime::name &&
              runtime.words.at(0x8eac78u) == Runtime::name &&
              runtime.calls == (std::vector<std::string>{"read", "write"}),
          "name thunk stores static name");
    runtime.calls.clear();
    Check(pal4::ClickGameWindowRegistration::RegisterFactoryThunk(runtime) ==
              0xa5u &&
              runtime.registered_name == Runtime::name &&
              runtime.registered_factory == 0x49f270u &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "read", "register"}),
          "factory thunk registers constructor under stored name");
}

void CheckFactoryAllocation() {
    Runtime runtime;
    Check(pal4::ClickGameWindowRegistration::Create(0x1234u, runtime) ==
              Runtime::object + 4u &&
              runtime.bytes == 0x38u &&
              runtime.constructed_layout == 0x1234u &&
              runtime.calls == (std::vector<std::string>{
                  "allocate", "construct"}),
          "factory allocates and forwards constructor result");

    Runtime failed;
    failed.allocation = 0u;
    Check(pal4::ClickGameWindowRegistration::Create(0x1234u, failed) == 0u &&
              failed.calls == (std::vector<std::string>{"allocate"}),
          "failed allocation skips constructor");

    Runtime throwing;
    throwing.throw_construct = true;
    bool caught = false;
    try {
        (void)pal4::ClickGameWindowRegistration::Create(0x1234u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.freed == Runtime::object &&
              throwing.calls == (std::vector<std::string>{
                  "allocate", "construct", "free"}),
          "constructor exception frees allocation");
}

void CheckHandler() {
    Runtime runtime;
    Check(pal4::ClickGameWindowRegistration::RegisterHandlerThunk(runtime) ==
              0x8eac74u &&
              runtime.bytes == 8u &&
              runtime.words.at(Runtime::object + 4u) == Runtime::name &&
              runtime.words.at(Runtime::object) == 0x843494u &&
              runtime.inserted_tree == Runtime::manager + 0x104u &&
              runtime.inserted_key == 0x8b1204u &&
              runtime.inserted_handler == Runtime::object &&
              runtime.destroyed == Runtime::object &&
              runtime.calls == (std::vector<std::string>{
                  "read", "allocate", "write", "write", "manager",
                  "insert", "destroy"}),
          "handler thunk registers and releases temporary handler");

    Runtime failed;
    failed.allocation = 0u;
    Check(pal4::ClickGameWindowRegistration::RegisterHandler(failed) ==
              0x8eac74u &&
              failed.inserted_handler == 0u && failed.destroyed == 0u &&
              failed.calls == (std::vector<std::string>{
                  "read", "allocate", "manager", "insert"}),
          "handler allocation failure still inserts null");

    Runtime throwing;
    throwing.throw_insert = true;
    bool caught = false;
    try {
        (void)pal4::ClickGameWindowRegistration::RegisterHandler(throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.destroyed == Runtime::object &&
              throwing.calls.back() == "destroy",
          "insertion exception destroys temporary handler");
}

}  // namespace

int main() {
    try {
        CheckNameAndFactory();
        CheckFactoryAllocation();
        CheckHandler();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
