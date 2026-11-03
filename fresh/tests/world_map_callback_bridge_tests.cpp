#include "pal4/world_map_callback_bridge.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IWorldMapCallbackRuntime,
                       pal4::IUIFrameManagerAccessorRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1008u, 0x484db0u}, {0x100cu, 0xfffffff0u},
        {0x1010u, 0x30u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x2000u;
    std::uint32_t invoked_function = 0u;
    std::uint32_t invoked_receiver = 0u;

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 0x18u, "world-map callback allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t InvokeThiscall(const std::uint32_t function,
                                 const std::uint32_t receiver) override {
        calls.emplace_back("invoke");
        invoked_function = function;
        invoked_receiver = receiver;
        return 0xabcdu;
    }
    std::uint32_t GetInstance() override {
        calls.emplace_back("get manager");
        return 0x3000u;
    }
};

void TestCloneAndInvoke() {
    Runtime runtime;
    const auto callback = pal4::WorldMapCallbackBridge::Clone(0x1000u,
                                                                runtime);
    Check(callback == 0x2000u &&
              runtime.words.at(0x2000u) == 0x843cc8u &&
              runtime.words.at(0x2008u) == 0x484db0u &&
              runtime.words.at(0x200cu) == 0xfffffff0u &&
              runtime.words.at(0x2010u) == 0x30u &&
              runtime.words.count(0x2004u) == 0u &&
              runtime.words.count(0x2014u) == 0u,
          "clone copies only three payload words and installs final vtable");
    Check(runtime.calls == std::vector<std::string>{
              "allocate", "write", "read", "write", "read", "write",
              "read", "write", "write"},
          "clone writes base vtable before payload and derived vtable last");
    Check(pal4::WorldMapCallbackBridge::Invoke(callback, runtime) ==
              0xabcdu &&
              runtime.invoked_function == 0x484db0u &&
              runtime.invoked_receiver == 0x20u &&
              runtime.calls.back() == "invoke",
          "callback tail dispatch uses word 8 and wrapping receiver sum");
}

void TestNullAllocationAndManagerThunk() {
    Runtime runtime;
    runtime.allocation = 0u;
    Check(pal4::WorldMapCallbackBridge::Clone(0x1000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "null allocation has no source reads or writes");
    Check(pal4::UIFrameManagerAccessor::Get(runtime) == 0x3000u &&
              runtime.calls.back() == "get manager",
          "frame-manager thunk forwards singleton result");
}

}  // namespace

int main() {
    TestCloneAndInvoke();
    TestNullAllocationAndManagerThunk();
    return EXIT_SUCCESS;
}
