#include "pal4/shop_ui_bootstrap.h"

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

struct Runtime final : pal4::IShopUiBootstrapRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x8b098cu, 0x9010u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x7000u;
    std::uint32_t freed = 0u;
    bool throw_constructor = false;

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
        Check(bytes == 0xc8u, "shop frame is 200 bytes");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t ConstructShop(const std::uint32_t storage,
                                const std::uint32_t name) override {
        Check(storage == 0x7000u && name == 0x9010u,
              "factory passes storage and name to constructor");
        calls.emplace_back("construct");
        if (throw_constructor) throw std::runtime_error("construct");
        return 0x7010u;
    }
    void Free(const std::uint32_t storage) override {
        Check(storage == 0x7000u, "factory frees failed storage");
        calls.emplace_back("free");
        ++freed;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t ProcessFrameEvent(
        const std::uint32_t manager, const std::uint32_t name,
        const std::uint32_t factory) override {
        Check(manager == 0x8000u && name == 0x9010u &&
                  factory == 0x4855c0u,
              "frame registration uses saved type name and factory");
        calls.emplace_back("process");
        return 0xa000u;
    }
};

void TestNormalChain() {
    Runtime runtime;
    Check(pal4::ShopUiBootstrap::GetFrameType(runtime) == 0x9010u &&
              runtime.words.at(0x8e7998u) == 0x9010u &&
              runtime.calls ==
                  std::vector<std::string>{"read", "write"},
          "type thunk stores source name and forwards result");
    Check(pal4::ShopUiBootstrap::Create(0x9010u, runtime) == 0x7010u &&
              runtime.freed == 0u &&
              runtime.calls[runtime.calls.size() - 2u] == "allocate" &&
              runtime.calls.back() == "construct",
          "shop factory allocates and constructs");
    Check(pal4::ShopUiBootstrap::GetFrame(runtime) == 0xa000u &&
              runtime.calls[runtime.calls.size() - 3u] == "manager" &&
              runtime.calls[runtime.calls.size() - 2u] == "read" &&
              runtime.calls.back() == "process",
          "frame thunk forwards manager registration result");
}

void TestAllocationAndConstructorFailure() {
    Runtime runtime;
    runtime.allocation = 0u;
    Check(pal4::ShopUiBootstrap::Create(0x9010u, runtime) == 0u &&
              runtime.calls ==
                  std::vector<std::string>{"allocate"} &&
              runtime.freed == 0u,
          "null allocation skips constructor");

    runtime = Runtime{};
    runtime.throw_constructor = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::ShopUiBootstrap::Create(0x9010u,
                                                        runtime));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.freed == 1u &&
              runtime.calls == (std::vector<std::string>{
                  "allocate", "construct", "free"}),
          "constructor exception frees factory allocation");
}

}  // namespace

int main() {
    TestNormalChain();
    TestAllocationAndConstructorFailure();
    return EXIT_SUCCESS;
}
