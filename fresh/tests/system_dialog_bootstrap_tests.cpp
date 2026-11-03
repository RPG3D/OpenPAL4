#include "pal4/system_dialog_bootstrap.h"
#include "pal4/system_dialog_frame.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemDialogBootstrapRuntime,
                       pal4::ISystemDialogFrameRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x8b0888u, 0x7000u}, {0x5024u, 0x6000u},
        {0x8411b8u, 0xa000u}, {0x84103cu, 0xa001u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x5000u;
    bool fail_construct = false;
    bool fail_layout = false;
    std::string current_name;
    std::uint32_t callback_allocations = 0u;

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
        Check(bytes == 0x30u || bytes == 12u,
              "dialog or callback allocation size");
        calls.emplace_back("allocate");
        if (bytes == 0x30u) return allocation;
        return 0xb000u + ++callback_allocations * 0x20u;
    }
    std::uint32_t ConstructDialog(const std::uint32_t storage,
                                  const std::uint32_t name) override {
        Check(storage == allocation && name == 0x7000u,
              "dialog constructor arguments");
        calls.emplace_back("construct");
        if (fail_construct) throw std::runtime_error("construct");
        return 0x5004u;
    }
    void Free(const std::uint32_t storage) override {
        Check(storage == allocation, "failed constructor allocation");
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
                  factory == 0x4835a0u,
              "dialog factory registration arguments");
        calls.emplace_back("process");
        return 0x8000u;
    }
    void LoadLayout(const std::uint32_t frame,
                    const std::uint32_t name) override {
        Check(frame == allocation && name == 0x7000u,
              "composed layout arguments");
        calls.emplace_back("layout");
        if (fail_layout) throw std::runtime_error("layout");
    }
    void DestroyBase(std::uint32_t) override {
        calls.emplace_back("destroy-base");
    }
    std::uint32_t BeginName(const std::uint32_t source) override {
        Check(source == allocation + 20u,
              "composed name source");
        current_name = "sysDialog/";
        return 0x9000u;
    }
    void ResetName(std::uint32_t,
                   std::uint32_t) override {
        current_name = "sysDialog/";
    }
    void AppendName(std::uint32_t,
                    const std::string_view suffix) override {
        current_name += suffix;
    }
    std::uint32_t FindWindow(std::uint32_t,
                             std::uint32_t) override {
        return current_name == "sysDialog/EditInfo" ? 0x8000u : 0x8004u;
    }
    void EndName(std::uint32_t) override {
        calls.emplace_back("end-name");
    }
    std::uint32_t Subscribe(std::uint32_t,
                            std::uint32_t,
                            std::uint32_t) override {
        return 0u;
    }
    void ReleaseConnection(std::uint32_t) override {}
};

void TestNameAndFactory() {
    Runtime runtime;
    Check(pal4::SystemDialogBootstrap::GetFrameName(runtime) == 0x7000u &&
              runtime.words.at(0x8e7988u) == 0x7000u &&
              runtime.calls == std::vector<std::string>{"read", "write"},
          "get-frame thunk initializes dialog name pointer");
    runtime.calls.clear();
    Check(pal4::SystemDialogBootstrap::InitializeFrameName(runtime) ==
              0x7000u && runtime.calls == std::vector<std::string>{
                  "read", "write"},
          "name initializer is independently callable");
    runtime.calls.clear();
    Check(pal4::SystemDialogBootstrap::GetFactory(runtime) == 0x8000u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "read", "process"},
          "factory thunk forwards manager, name and create callback");
    runtime.calls.clear();
    Check(pal4::SystemDialogBootstrap::SetupFrame(runtime) == 0x8000u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "read", "process"},
          "factory setup is independently callable");
}

void TestCreate() {
    Runtime runtime;
    Check(pal4::SystemDialogBootstrap::CreateDialog(
              0x7000u, runtime) == 0x5004u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "construct"},
          "create returns constructor result");
    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(pal4::SystemDialogBootstrap::CreateDialog(
              0x7000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "null allocation skips constructor");
    runtime = Runtime{};
    runtime.fail_construct = true;
    bool caught = false;
    try {
        (void)pal4::SystemDialogBootstrap::CreateDialog(0x7000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "construct", "free"},
          "constructor failure frees factory allocation");
}

void TestComposedCreate() {
    Runtime runtime;
    Check(pal4::SystemDialogBootstrap::CreateDialog(
              0x7000u, runtime, runtime) == 0x5000u &&
              runtime.words.at(0x5000u) ==
                  pal4::SystemDialogFrame::kFrameVtable &&
              runtime.words.at(0x5028u) == 0x8000u &&
              runtime.words.at(0x502cu) == 0x8004u &&
              runtime.callback_allocations == 2u &&
              runtime.calls.front() == "allocate" &&
              runtime.calls.back() == "end-name",
          "factory composes reconstructed system-dialog constructor");
    runtime = Runtime{};
    runtime.fail_layout = true;
    bool caught = false;
    try {
        (void)pal4::SystemDialogBootstrap::CreateDialog(
            0x7000u, runtime, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "layout", "free"},
          "composed factory releases allocation on constructor failure");
}

}  // namespace

int main() {
    TestNameAndFactory();
    TestCreate();
    TestComposedCreate();
    return EXIT_SUCCESS;
}
