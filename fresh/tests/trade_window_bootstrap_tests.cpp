#include "pal4/trade_window_bootstrap.h"

#include <cstdlib>
#include <iostream>
#include <map>
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

struct Runtime final : pal4::ITradeWindowBootstrapRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{0x8b0a54u, 0x9010u},
                                                 {0x8e79a4u, 0x9010u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x7000u;
    std::uint32_t allocated_bytes = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t released = 0u;
    std::uint32_t released_flags = 0u;
    std::uint32_t tree = 0u;
    std::uint32_t key = 0u;
    std::uint32_t inserted_callback = 0u;
    bool throw_constructor = false;
    bool throw_insert = false;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocated_bytes = bytes;
        return allocation;
    }
    void Free(const std::uint32_t storage) override {
        Check(storage == 0x7000u, "failed constructor storage");
        calls.emplace_back("free");
        ++freed;
    }
    std::uint32_t ConstructWindow(const std::uint32_t storage,
                                  const std::uint32_t name) override {
        Check(storage == 0x7000u && name == 0x9010u,
              "trade constructor receives allocated storage and name");
        calls.emplace_back("construct");
        if (throw_constructor) throw std::runtime_error("construct");
        return 0x7010u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t ProcessFrameEvent(const std::uint32_t manager,
                                    const std::uint32_t name,
                                    const std::uint32_t factory) override {
        Check(manager == 0x8000u && name == 0x9010u &&
                  factory == 0x48a840u,
              "trade factory registration arguments");
        calls.emplace_back("process");
        return 0xa000u;
    }
    void InsertOrReplace(const std::uint32_t tree_value,
                         const std::uint32_t key_value,
                         const std::uint32_t callback) override {
        calls.emplace_back("insert");
        tree = tree_value;
        key = key_value;
        inserted_callback = callback;
        if (throw_insert) throw std::runtime_error("insert");
    }
    void ReleaseCallback(const std::uint32_t callback,
                         const std::uint32_t flags) override {
        calls.emplace_back("release");
        released = callback;
        released_flags = flags;
    }
};

void TestNameAndFactory() {
    Runtime runtime;
    Check(pal4::TradeWindowBootstrap::GetNameThunk(runtime) == 0x9010u &&
              runtime.words.at(0x8e79a4u) == 0x9010u &&
              runtime.calls == std::vector<std::string>{"read", "write"},
          "name thunk saves current TradeWindow name");
    runtime.calls.clear();
    Check(pal4::TradeWindowBootstrap::GetName(runtime) == 0x9010u &&
              runtime.calls == std::vector<std::string>{"read", "write"},
          "direct name entry has identical writes");
    runtime.calls.clear();
    Check(pal4::TradeWindowBootstrap::RegisterFactoryThunk(runtime) ==
              0xa000u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "read", "process"},
          "factory thunk forwards manager, saved name and factory address");
    runtime.calls.clear();
    Check(pal4::TradeWindowBootstrap::RegisterFactory(runtime) == 0xa000u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "read", "process"},
          "direct factory registration preserves call order");
}

void TestFactoryAllocation() {
    Runtime runtime;
    Check(pal4::TradeWindowBootstrap::Create(0x9010u, runtime) == 0x7010u &&
              runtime.allocated_bytes == 0x1b4u && runtime.freed == 0u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "construct"},
          "factory allocates 436 bytes then constructs trade window");

    Runtime empty;
    empty.allocation = 0u;
    Check(pal4::TradeWindowBootstrap::Create(0x9010u, empty) == 0u &&
              empty.calls == std::vector<std::string>{"allocate"},
          "null allocation skips constructor");

    Runtime failure;
    failure.throw_constructor = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::TradeWindowBootstrap::Create(0x9010u,
                                                               failure));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failure.freed == 1u &&
              failure.calls == std::vector<std::string>{
                  "allocate", "construct", "free"},
          "constructor exception frees allocated trade window");
}

void TestShowEventRegistration() {
    Runtime runtime;
    Check(pal4::TradeWindowBootstrap::RegisterShowEventThunk(runtime) ==
              0x8e79a0u &&
              runtime.allocated_bytes == 8u &&
              runtime.words.at(0x7004u) == 0x9010u &&
              runtime.words.at(0x7000u) == 0x843494u &&
              runtime.tree == 0x8104u && runtime.key == 0x8b0a44u &&
              runtime.inserted_callback == 0x7000u &&
              runtime.released == 0x7000u && runtime.released_flags == 1u &&
              runtime.calls == std::vector<std::string>{
                  "read", "allocate", "write", "write", "manager",
                  "insert", "release"},
          "show event thunk constructs and releases callback after insertion");

    Runtime empty;
    empty.allocation = 0u;
    Check(pal4::TradeWindowBootstrap::RegisterShowEvent(empty) == 0x8e79a0u &&
              empty.inserted_callback == 0u && empty.released == 0u &&
              empty.calls == std::vector<std::string>{
                  "read", "allocate", "manager", "insert"},
          "null callback still inserts a null event value");

    Runtime failure;
    failure.throw_insert = true;
    bool caught = false;
    try {
        static_cast<void>(
            pal4::TradeWindowBootstrap::RegisterShowEvent(failure));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failure.released == 0x7000u &&
              failure.released_flags == 1u &&
              failure.calls.back() == "release",
          "insertion exception releases temporary callback");
}

}  // namespace

int main() {
    TestNameAndFactory();
    TestFactoryAllocation();
    TestShowEventRegistration();
    return EXIT_SUCCESS;
}
