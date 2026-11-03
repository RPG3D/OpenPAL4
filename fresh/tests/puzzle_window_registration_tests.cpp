#include "pal4/puzzle_window_registration.h"

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

struct Runtime final : pal4::IPuzzleWindowRegistrationRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t manager = 0x2000u;
    static constexpr std::uint32_t name = 0x8b12e0u;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8b12dcu, name}};
    std::vector<std::string> calls;
    std::uint32_t allocation = object;
    std::uint32_t size = 0u;
    std::uint32_t layout = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t factory_name = 0u;
    std::uint32_t factory_address = 0u;
    std::uint32_t tree = 0u;
    std::uint32_t key = 0u;
    std::uint32_t inserted = 0xffffffffu;
    std::uint32_t destroyed = 0u;
    bool throw_construct = false;
    bool throw_insert = false;

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
        size = bytes;
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        freed = address;
        calls.emplace_back("free");
    }
    std::uint32_t Construct(std::uint32_t address,
                            std::uint32_t layout_name) override {
        Check(address == object, "puzzle frame pointer");
        layout = layout_name;
        calls.emplace_back("construct");
        if (throw_construct) throw std::runtime_error("construct failed");
        return object + 4u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    std::uint32_t RegisterFactory(std::uint32_t target,
                                   std::uint32_t window_name,
                                   std::uint32_t factory) override {
        Check(target == manager, "factory manager");
        factory_name = window_name;
        factory_address = factory;
        calls.emplace_back("factory");
        return 0xa5u;
    }
    void InsertOrReplace(std::uint32_t target,
                         std::uint32_t event_key,
                         std::uint32_t handler) override {
        tree = target;
        key = event_key;
        inserted = handler;
        calls.emplace_back("insert");
        if (throw_insert) throw std::runtime_error("insert failed");
    }
    void DestroyHandler(std::uint32_t handler,
                        std::uint32_t flags) override {
        Check(flags == 1u, "deleting handler flag");
        destroyed = handler;
        calls.emplace_back("destroy");
    }
};

void CheckNameAndFactory() {
    Runtime runtime;
    Check(pal4::PuzzleWindowRegistration::InitializeNameThunk(runtime) ==
              Runtime::name &&
              runtime.words.at(0x8eac88u) == Runtime::name &&
              runtime.calls == (std::vector<std::string>{"read", "write"}),
          "puzzleWindow name initialization and thunk");
    runtime.calls.clear();
    Check(pal4::PuzzleWindowRegistration::RegisterFactoryThunk(runtime) ==
              0xa5u && runtime.factory_name == Runtime::name &&
              runtime.factory_address == 0x4a0a40u &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "read", "factory"}),
          "factory registration and thunk");
}

void CheckCreate() {
    Runtime runtime;
    Check(pal4::PuzzleWindowRegistration::Create(
              0x8000u, runtime) == Runtime::object + 4u &&
              runtime.size == 0x1a4u && runtime.layout == 0x8000u &&
              runtime.calls == (std::vector<std::string>{
                  "allocate", "construct"}),
          "factory allocation and constructor forwarding");
    Runtime no_memory;
    no_memory.allocation = 0u;
    Check(pal4::PuzzleWindowRegistration::Create(
              0x8000u, no_memory) == 0u &&
              no_memory.calls == (std::vector<std::string>{"allocate"}),
          "null factory allocation");
    Runtime failing;
    failing.throw_construct = true;
    bool caught = false;
    try {
        (void)pal4::PuzzleWindowRegistration::Create(0x8000u, failing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failing.freed == Runtime::object &&
              failing.calls.back() == "free",
          "constructor exception releases frame");
}

void CheckHandler() {
    Runtime runtime;
    Check(pal4::PuzzleWindowRegistration::RegisterHandlerThunk(runtime) ==
              0x8eac84u && runtime.size == 8u &&
              runtime.words.at(Runtime::object + 4u) == 0x4a1180u &&
              runtime.words.at(Runtime::object) == 0x8435dcu &&
              runtime.tree == Runtime::manager + 0x104u &&
              runtime.key == 0x8b12d8u &&
              runtime.inserted == Runtime::object &&
              runtime.destroyed == Runtime::object &&
              runtime.calls == (std::vector<std::string>{
                  "allocate", "write", "write", "manager", "insert",
                  "destroy"}),
          "show_puzzle handler layout and ownership");
    Runtime no_memory;
    no_memory.allocation = 0u;
    Check(pal4::PuzzleWindowRegistration::RegisterHandler(no_memory) ==
              0x8eac84u && no_memory.inserted == 0u &&
              no_memory.destroyed == 0u,
          "null handler insertion");
    Runtime failing;
    failing.throw_insert = true;
    bool caught = false;
    try {
        (void)pal4::PuzzleWindowRegistration::RegisterHandler(failing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failing.destroyed == Runtime::object &&
              failing.calls.back() == "destroy",
          "handler insertion exception cleanup");
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
