#include "pal4/debug_function_loader.h"

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

class Runtime final : public pal4::IDebugFunctionRuntime {
public:
    std::uint32_t LoadLibrary(const char* name) override {
        Check(std::string(name) == "PSAPI.DLL", "loads exact PSAPI name");
        events.emplace_back("load");
        return module;
    }
    std::uint32_t GetProcAddress(std::uint32_t handle,
                                 const char* name) override {
        Check(handle == module, "all exports resolve against loaded module");
        events.emplace_back(name);
        const std::size_t index = events.size() - 2u;
        return addresses.at(index);
    }

    std::vector<std::string> events;
    std::vector<std::uint32_t> addresses{0x100u, 0x200u, 0x300u};
    std::uint32_t module = 0x77u;
};

void CheckSuccessAndCache() {
    pal4::DebugFunctionState state;
    pal4::DebugFunctionLoader loader(state);
    Runtime runtime;
    Check(loader.Load(runtime) == 1u && state.initialized == 1u &&
              state.enum_process_modules == 0x100u &&
              state.get_module_base_name == 0x200u &&
              state.get_module_file_name == 0x300u &&
              runtime.events == std::vector<std::string>{
                  "load", "EnumProcessModules", "GetModuleBaseNameA",
                  "GetModuleFileNameExA"},
          "all three exports load in order and set guard");
    runtime.events.clear();
    Check(loader.Load(runtime) == 1u && runtime.events.empty(),
          "guard equal to one bypasses loader");
}

void CheckFailuresAndRetry() {
    for (std::size_t failed = 0u; failed < 4u; ++failed) {
        pal4::DebugFunctionState state;
        Runtime runtime;
        if (failed == 0u) runtime.module = 0u;
        else runtime.addresses[failed - 1u] = 0u;
        pal4::DebugFunctionLoader loader(state);
        Check(loader.Load(runtime) == 0u && state.initialized == 0u &&
                  runtime.events.size() == failed + 1u,
              "failure stops at the first missing library or export");
        if (failed >= 2u) {
            Check(state.enum_process_modules == 0x100u,
                  "previously resolved export remains stored on failure");
        }
        runtime.module = 0x77u;
        runtime.addresses = {0x100u, 0x200u, 0x300u};
        runtime.events.clear();
        Check(loader.Load(runtime) == 1u && state.initialized == 1u &&
                  runtime.events.size() == 4u,
              "failed load retries the entire sequence");
    }
}

void CheckNonOneGuard() {
    pal4::DebugFunctionState state;
    state.initialized = 2u;
    Runtime runtime;
    pal4::DebugFunctionLoader loader(state);
    Check(loader.Load(runtime) == 1u && state.initialized == 1u &&
              runtime.events.size() == 4u,
          "non-one guard does not bypass the loader");
}

}  // namespace

int main() {
    CheckSuccessAndCache();
    CheckFailuresAndRetry();
    CheckNonOneGuard();
    std::cout << "3 debug function loader test cases passed\n";
}
