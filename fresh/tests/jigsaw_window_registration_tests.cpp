#include "pal4/jigsaw_window_registration.h"

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

struct Runtime final : pal4::IJigsawWindowRegistrationRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t manager = 0x2000u;
    static constexpr std::uint32_t name = 0x8b1240u;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8b123cu, name}};
    std::vector<std::string> calls;
    std::uint32_t allocation = object;
    std::uint32_t bytes = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t layout = 0u;
    std::uint32_t factory_name = 0u;
    std::uint32_t factory = 0u;
    std::uint32_t tree = 0u;
    std::uint32_t key = 0u;
    std::uint32_t inserted = 0u;
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
        Check(address == object, "construct object");
        layout = layout_name;
        if (throw_construct) throw std::runtime_error("construct failed");
        return object + 4u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    std::uint32_t RegisterFactory(std::uint32_t target,
                                   std::uint32_t window_name,
                                   std::uint32_t factory_address) override {
        Check(target == manager, "factory manager");
        calls.emplace_back("register");
        factory_name = window_name;
        factory = factory_address;
        return 0xa5u;
    }
    void InsertOrReplace(std::uint32_t target,
                         std::uint32_t event_key,
                         std::uint32_t handler) override {
        calls.emplace_back("insert");
        tree = target;
        key = event_key;
        inserted = handler;
        if (throw_insert) throw std::runtime_error("insert failed");
    }
    void DestroyHandler(std::uint32_t handler,
                        std::uint32_t flags) override {
        Check(flags == 1u, "deleting flag");
        calls.emplace_back("destroy");
        destroyed = handler;
    }
};

void CheckNameAndFactory() {
    Runtime runtime;
    Check(pal4::JigsawWindowRegistration::InitializeNameThunk(runtime) ==
              Runtime::name &&
              runtime.words.at(0x8eac80u) == Runtime::name &&
              runtime.calls == (std::vector<std::string>{"read", "write"}),
          "jigsaw name pointer initialization");
    runtime.calls.clear();
    Check(pal4::JigsawWindowRegistration::RegisterFactoryThunk(runtime) ==
              0xa5u &&
              runtime.factory_name == Runtime::name &&
              runtime.factory == 0x49f750u &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "read", "register"}),
          "jigsaw factory registration");
}

void CheckCreate() {
    Runtime runtime;
    Check(pal4::JigsawWindowRegistration::Create(0x8000u, runtime) ==
              Runtime::object + 4u &&
              runtime.bytes == 0x190u && runtime.layout == 0x8000u &&
              runtime.calls == (std::vector<std::string>{
                  "allocate", "construct"}),
          "jigsaw factory allocation and construction");
    Runtime failed;
    failed.allocation = 0u;
    Check(pal4::JigsawWindowRegistration::Create(0x8000u, failed) == 0u &&
              failed.calls == (std::vector<std::string>{"allocate"}),
          "allocation failure skips construction");
    Runtime throwing;
    throwing.throw_construct = true;
    bool caught = false;
    try {
        (void)pal4::JigsawWindowRegistration::Create(0x8000u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.freed == Runtime::object &&
              throwing.calls.back() == "free",
          "constructor failure frees frame");
}

void CheckHandler() {
    Runtime runtime;
    Check(pal4::JigsawWindowRegistration::RegisterHandlerThunk(runtime) ==
              0x8eac7cu &&
              runtime.bytes == 8u &&
              runtime.words.at(Runtime::object + 4u) == 0x49fed0u &&
              runtime.words.at(Runtime::object) == 0x8435dcu &&
              runtime.tree == Runtime::manager + 0x104u &&
              runtime.key == 0x8b1238u &&
              runtime.inserted == Runtime::object &&
              runtime.destroyed == Runtime::object &&
              runtime.calls == (std::vector<std::string>{
                  "allocate", "write", "write", "manager", "insert",
                  "destroy"}),
          "show_jigsaw handler layout and insertion");
    Runtime failed;
    failed.allocation = 0u;
    Check(pal4::JigsawWindowRegistration::RegisterHandler(failed) ==
              0x8eac7cu && failed.inserted == 0u && failed.destroyed == 0u,
          "null handler is inserted after allocation failure");
    Runtime throwing;
    throwing.throw_insert = true;
    bool caught = false;
    try {
        (void)pal4::JigsawWindowRegistration::RegisterHandler(throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.destroyed == Runtime::object &&
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
