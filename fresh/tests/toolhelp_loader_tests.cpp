#include "pal4/toolhelp_loader.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IToolHelpRuntime {
public:
    std::uint32_t GetModuleHandle(const char* name) override {
        Check(std::string(name) == "KERNEL32.DLL", "loads exact module name");
        events.emplace_back("module");
        return module;
    }
    std::uint32_t GetProcAddress(std::uint32_t handle,
                                 const char* name) override {
        Check(handle == module, "all exports use KERNEL32 handle");
        events.emplace_back(name);
        return addresses.at(events.size() - 2u);
    }
    void SetLastErrorEx(std::uint32_t error, std::uint32_t type) override {
        Check(error == 0x45au && type == 1u,
              "missing module sets exact extended error");
        events.emplace_back("error");
    }

    std::vector<std::string> events;
    std::array<std::uint32_t, 7> addresses{
        1u, 2u, 3u, 4u, 5u, 6u, 7u};
    std::uint32_t module = 0x1111u;
};

void CheckSuccess() {
    Runtime runtime;
    pal4::ToolHelpState state;
    pal4::ToolHelpLoader loader(state);
    Check(loader.Initialize(runtime) == 1u && state.initialized == 1u &&
              state.functions == runtime.addresses &&
              runtime.events == std::vector<std::string>{
                  "module", "CreateToolhelp32Snapshot", "Module32First",
                  "Module32Next", "Process32First", "Process32Next",
                  "Thread32First", "Thread32Next"},
          "all seven exports are resolved and guard is set");
    runtime.events.clear();
    Check(loader.Initialize(runtime) == 1u && runtime.events.empty(),
          "guard equal to one returns without API calls");
}

void CheckMissingModuleAndExports() {
    Runtime missing_module;
    missing_module.module = 0u;
    pal4::ToolHelpState state;
    pal4::ToolHelpLoader loader(state);
    Check(loader.Initialize(missing_module) == 0u &&
              state.initialized == 0u &&
              missing_module.events == std::vector<std::string>{
                  "module", "error"},
          "missing module sets error and skips export resolution");

    for (std::size_t missing = 0u; missing < 7u; ++missing) {
        Runtime runtime;
        runtime.addresses[missing] = 0u;
        pal4::ToolHelpState partial;
        pal4::ToolHelpLoader partial_loader(partial);
        Check(partial_loader.Initialize(runtime) == 0u &&
                  partial.initialized == 0u &&
                  partial.functions == runtime.addresses &&
                  runtime.events.size() == 8u,
              "missing export still resolves and stores all seven slots");
    }
}

void CheckRetryAndNonOneGuard() {
    Runtime runtime;
    pal4::ToolHelpState state;
    state.initialized = 3u;
    runtime.addresses[2] = 0u;
    pal4::ToolHelpLoader loader(state);
    Check(loader.Initialize(runtime) == 0u && state.initialized == 3u,
          "non-one guard queries exports and remains unchanged on failure");
    runtime.events.clear();
    runtime.addresses[2] = 3u;
    Check(loader.Initialize(runtime) == 1u && state.initialized == 1u &&
              runtime.events.size() == 8u,
          "failed initialization retries all exports and then caches success");
}

}  // namespace

int main() {
    CheckSuccess();
    CheckMissingModuleAndExports();
    CheckRetryAndNonOneGuard();
    std::cout << "3 ToolHelp loader test cases passed\n";
}
