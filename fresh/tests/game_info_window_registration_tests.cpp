#include "pal4/game_info_window_registration.h"

#include <array>
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

struct Runtime final : pal4::IGameInfoWindowRegistrationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8b139cu, 0x8b13f4u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x2000u;
    std::uint32_t allocation_size = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t layout_name = 0u;
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
        layout_name = name;
        if (throw_construct) throw std::runtime_error("construct");
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
        return 0x55u;
    }
    void InsertOrReplace(std::uint32_t target,
                         std::uint32_t event_key,
                         std::uint32_t value) override {
        calls.emplace_back("insert");
        tree = target;
        key = event_key;
        handler = value;
        if (throw_insert) throw std::runtime_error("insert");
    }
    void DestroyHandler(std::uint32_t value,
                        std::uint32_t flags) override {
        calls.emplace_back("destroy-handler");
        Check(flags == 1u, "deleting destructor flag");
        destroyed = value;
    }
};

using Handler = std::uint32_t (*)(Runtime&);

struct HandlerSpec {
    Handler thunk;
    Handler direct;
    std::uint32_t callback;
    std::uint32_t key;
    std::uint32_t result;
};

std::uint32_t ShowThunk(Runtime& runtime) {
    return pal4::GameInfoWindowRegistration::RegisterShowThunk(runtime);
}
std::uint32_t Show(Runtime& runtime) {
    return pal4::GameInfoWindowRegistration::RegisterShow(runtime);
}
std::uint32_t HelpThunk(Runtime& runtime) {
    return pal4::GameInfoWindowRegistration::RegisterHelpThunk(runtime);
}
std::uint32_t Help(Runtime& runtime) {
    return pal4::GameInfoWindowRegistration::RegisterHelp(runtime);
}
std::uint32_t UpdateThunk(Runtime& runtime) {
    return pal4::GameInfoWindowRegistration::RegisterUpdateThunk(runtime);
}
std::uint32_t Update(Runtime& runtime) {
    return pal4::GameInfoWindowRegistration::RegisterUpdate(runtime);
}

constexpr std::array<HandlerSpec, 3> kHandlers{{
    {ShowThunk, Show, 0x4a6ba0u, 0x8b13a0u, 0x8eacaau},
    {HelpThunk, Help, 0x4a6bc0u, 0x8b13a4u, 0x8eaca9u},
    {UpdateThunk, Update, 0x4a6bf0u, 0x8b13a8u, 0x8eaca8u},
}};

void CheckNameAndFactory() {
    using Registration = pal4::GameInfoWindowRegistration;
    Runtime runtime;
    Check(Registration::InitializeNameThunk(runtime) == 0x8b13f4u &&
              runtime.words.at(0x8eacacu) == 0x8b13f4u &&
              runtime.calls == (std::vector<std::string>{"read", "write"}),
          "gameInfo name pointer and forwarding");
    runtime.calls.clear();
    Check(Registration::RegisterFactoryThunk(runtime) == 0x55u &&
              runtime.factory_name == 0x8b13f4u &&
              runtime.factory_address == 0x4a6420u &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "read", "register-factory"}),
          "gameInfo factory registration");
    runtime.calls.clear();
    Check(Registration::Create(0x9000u, runtime) == 0x2004u &&
              runtime.allocation_size == 0x50u &&
              runtime.layout_name == 0x9000u &&
              runtime.calls == (std::vector<std::string>{
                  "allocate", "construct"}),
          "gameInfo factory object size and constructor forwarding");
}

void CheckHandlers() {
    for (const auto& spec : kHandlers) {
        Runtime runtime;
        Check(spec.thunk(runtime) == spec.result &&
                  runtime.allocation_size == 8u &&
                  runtime.words.at(0x2000u) == 0x8435dcu &&
                  runtime.words.at(0x2004u) == spec.callback &&
                  runtime.tree == 0x3104u &&
                  runtime.key == spec.key &&
                  runtime.handler == 0x2000u &&
                  runtime.destroyed == 0x2000u &&
                  runtime.calls == (std::vector<std::string>{
                      "allocate", "write", "write", "manager",
                      "insert", "destroy-handler"}),
              "callback handler thunk and native fields");
        Runtime direct;
        Check(spec.direct(direct) == spec.result &&
                  direct.words.at(0x2004u) == spec.callback &&
                  direct.key == spec.key,
              "direct handler entry matches thunk");
        Runtime empty;
        empty.allocation = 0u;
        Check(spec.direct(empty) == spec.result &&
                  empty.handler == 0u && empty.destroyed == 0u &&
                  empty.calls == (std::vector<std::string>{
                      "allocate", "manager", "insert"}),
              "null callback handler still inserts");
        Runtime failure;
        failure.throw_insert = true;
        bool caught = false;
        try { (void)spec.direct(failure); }
        catch (const std::runtime_error&) { caught = true; }
        Check(caught && failure.destroyed == 0x2000u &&
                  failure.calls.back() == "destroy-handler",
              "insertion failure releases temporary handler");
    }
}

void CheckFactoryFailures() {
    using Registration = pal4::GameInfoWindowRegistration;
    Runtime empty;
    empty.allocation = 0u;
    Check(Registration::Create(0x9000u, empty) == 0u &&
              empty.layout_name == 0u,
          "null factory allocation");
    Runtime failure;
    failure.throw_construct = true;
    bool caught = false;
    try { (void)Registration::Create(0x9000u, failure); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && failure.freed == 0x2000u &&
              failure.calls.back() == "free",
          "constructor failure releases object");
}

}  // namespace

int main() {
    try {
        CheckNameAndFactory();
        CheckHandlers();
        CheckFactoryFailures();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
