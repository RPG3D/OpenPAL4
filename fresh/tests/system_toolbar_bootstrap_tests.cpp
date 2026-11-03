#include "pal4/system_toolbar_bootstrap.h"
#include "pal4/system_toolbar_frame.h"

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

struct Runtime final : pal4::ISystemToolbarBootstrapRuntime,
                       pal4::ISystemToolbarFrameRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x8b07d8u, 0x7000u}, {0x8e7980u, 0x7000u},
        {0x8411b8u, 0xa000u}, {0x5000u + 36u, 0x6000u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x5000u;
    bool throw_construct = false;
    bool throw_layout = false;
    std::uint32_t callback_allocations = 0u;
    std::uint32_t found_windows = 0u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 0x5cu || bytes == 24u,
              "frame or callback allocation size");
        calls.emplace_back("allocate");
        if (bytes == 0x5cu) return allocation;
        return 0x9000u + ++callback_allocations * 0x20u;
    }
    std::uint32_t ConstructFrame(const std::uint32_t storage,
                                 const std::uint32_t name) override {
        Check(storage == allocation && name == 0x7000u,
              "frame constructor arguments");
        calls.emplace_back("construct");
        if (throw_construct) throw std::runtime_error("construct");
        return 0x5004u;
    }
    void Free(const std::uint32_t storage) override {
        Check(storage == allocation, "failed constructor storage");
        calls.emplace_back("free");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x6000u;
    }
    std::uint32_t ProcessFrameEvent(const std::uint32_t manager,
                                    const std::uint32_t name,
                                    const std::uint32_t factory) override {
        Check(manager == 0x6000u && name == 0x7000u &&
                  factory == 0x482550u,
              "frame-manager factory arguments");
        calls.emplace_back("process");
        return 0x8000u;
    }
    void LoadLayout(const std::uint32_t frame,
                    const std::uint32_t name) override {
        Check(frame == allocation && name == 0x7000u,
              "composed layout arguments");
        calls.emplace_back("layout");
        if (throw_layout) throw std::runtime_error("layout");
    }
    void ConstructMouseHelper(const std::uint32_t helper) override {
        Check(helper == allocation + 40u, "composed helper address");
        calls.emplace_back("helper");
    }
    void DestroyMouseHelper(std::uint32_t) override {
        calls.emplace_back("destroy-helper");
    }
    void DestroyBase(std::uint32_t) override {
        calls.emplace_back("destroy-base");
    }
    std::uint32_t FindGlobalWindow(std::string_view) override {
        return 0x8000u + ++found_windows;
    }
    std::uint32_t FindChildWindow(std::uint32_t,
                                  std::string_view) override {
        return 0x8000u + ++found_windows;
    }
    std::uint32_t Subscribe(std::uint32_t,
                            std::uint32_t,
                            std::uint32_t) override {
        return 0u;
    }
    void ReleaseConnection(std::uint32_t) override {}
    void SetZOrderingEnabled(std::uint32_t, bool) override {}
    void SetupMouseHandlers(std::uint32_t,
                            std::uint32_t) override {}
};

void TestNameAndThunks() {
    Runtime runtime;
    Check(pal4::SystemToolbarBootstrap::GetFrameName(runtime) == 0x7000u &&
              runtime.words.at(0x8e7980u) == 0x7000u &&
              runtime.calls == std::vector<std::string>{
                  "read:9111512", "write:9337216"},
          "get-frame thunk initializes name pointer");
    runtime.calls.clear();
    Check(pal4::SystemToolbarBootstrap::InitializeFrameName(runtime) ==
              0x7000u && runtime.calls.size() == 2u,
          "initializer is callable directly");
    runtime.calls.clear();
    Check(pal4::SystemToolbarBootstrap::CreateUI(runtime) == 0x8000u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "read:9337216", "process"},
          "create-UI thunk routes to frame-manager setup");
    runtime.calls.clear();
    Check(pal4::SystemToolbarBootstrap::SetupFrame(runtime) == 0x8000u &&
              runtime.calls.size() == 3u,
          "setup is callable directly");
}

void TestFactory() {
    Runtime runtime;
    Check(pal4::SystemToolbarBootstrap::CreateFrame(0x7000u, runtime) ==
              0x5004u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "construct"},
          "factory returns constructor result");
    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(pal4::SystemToolbarBootstrap::CreateFrame(0x7000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "null allocation skips constructor");
    runtime.calls.clear();
    runtime.allocation = 0x5000u;
    runtime.throw_construct = true;
    bool caught = false;
    try {
        (void)pal4::SystemToolbarBootstrap::CreateFrame(0x7000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "construct", "free"},
          "constructor exception releases factory allocation");
}

void TestComposedFactory() {
    Runtime runtime;
    Check(pal4::SystemToolbarBootstrap::CreateFrame(
              0x7000u, runtime, runtime) == 0x5000u &&
              runtime.calls[0] == "allocate" &&
              runtime.calls[1] == "layout" &&
              runtime.calls[2] == "helper" &&
              runtime.words.at(0x5000u) ==
                  pal4::SystemToolbarFrame::kFrameVtable &&
              runtime.callback_allocations == 7u &&
              runtime.found_windows == 7u,
          "factory composes reconstructed toolbar constructor");

    runtime = Runtime{};
    runtime.throw_layout = true;
    bool caught = false;
    try {
        (void)pal4::SystemToolbarBootstrap::CreateFrame(
            0x7000u, runtime, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "layout", "free"},
          "composed factory releases allocation on layout failure");
}

}  // namespace

int main() {
    TestNameAndThunks();
    TestFactory();
    TestComposedFactory();
    return EXIT_SUCCESS;
}
