#include "pal4/symbol_module_loader.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
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

class Runtime final : public pal4::ISymbolModuleRuntime {
public:
    std::uint32_t CheckOperatingSystemPlatform() override {
        events.emplace_back("platform");
        return platform;
    }
    std::uint32_t SymInitialize(std::uint32_t process,
                                std::uint32_t search_path,
                                std::uint32_t invade) override {
        Check(process == 0x22u && search_path == 0x33u &&
                  invade == expected_invade,
              "SymInitialize receives process, search path and invade flag");
        events.emplace_back("sym-init");
        return sym_init_result;
    }
    std::uint32_t EnumerateProcessModules(
        std::uint32_t process_id, std::uint32_t capacity,
        std::uint32_t buffer, std::uint32_t& count) override {
        Check(process_id == 0x11u, "dispatcher receives process ID");
        ++enumerate_calls;
        if (enumerate_calls == 1u) {
            Check(capacity == 0u && buffer == 0u && count == 0u,
                  "first enumeration requests only module count");
            events.emplace_back("count");
            count = static_cast<std::uint32_t>(modules.size());
            return first_enumerate_result;
        }
        Check(capacity == modules.size() && buffer == allocation,
              "second enumeration receives allocated module array");
        events.emplace_back("enumerate");
        if (second_count_override != 0xffffffffu) count = second_count_override;
        return second_enumerate_result;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == modules.size() * 4u,
              "allocation multiplies first enumeration count by four");
        events.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == allocation, "free receives original allocation");
        events.emplace_back("free");
    }
    std::uint32_t ReadModule(std::uint32_t buffer,
                             std::uint32_t index) override {
        Check(buffer == allocation && index < modules.size(),
              "reads each module from allocated array");
        events.emplace_back("read:" + std::to_string(index));
        return modules[index];
    }
    std::uint32_t GetModuleFileNameA(
        std::uint32_t module, char* filename,
        std::uint32_t capacity) override {
        Check(capacity == 0x104u && filename[0] == 0,
              "module filename call receives 260-byte local buffer");
        events.emplace_back("file-name:" + std::to_string(module));
        if (module == filename_failure_module) return 0u;
        const std::string path = "module" + std::to_string(module) + ".dll";
        std::memcpy(filename, path.c_str(), path.size() + 1u);
        return static_cast<std::uint32_t>(path.size());
    }
    std::uint32_t CreateFileA(
        const char* filename, std::uint32_t access,
        std::uint32_t share, std::uint32_t security,
        std::uint32_t creation, std::uint32_t flags,
        std::uint32_t template_file) override {
        Check(std::string(filename).find("module") == 0u &&
                  access == 0x80000000u && share == 1u && security == 0u &&
                  creation == 3u && flags == 0u && template_file == 0u,
              "CreateFileA receives filename and all seven original arguments");
        events.emplace_back("create-file");
        return 0x77u;
    }
    std::uint32_t SymLoadModule(
        std::uint32_t process, std::uint32_t file,
        const char* filename, std::uint32_t module_name,
        std::uint32_t base, std::uint32_t size) override {
        Check(process == 0x22u && file == 0x77u &&
                  std::string(filename).find("module") == 0u &&
                  module_name == 0u && size == 0u &&
                  (base == 0x1000u || base == 0x2000u),
              "SymLoadModule receives process, file, name, base and zero fields");
        events.emplace_back("sym-load:" + std::to_string(base));
        return base == load_failure_module ? 0u : 1u;
    }
    std::uint32_t GetLastError() override {
        events.emplace_back("get-error");
        return last_error;
    }
    void SymCleanup(std::uint32_t process) override {
        Check(process == 0x22u, "cleanup receives process handle");
        events.emplace_back("cleanup");
    }

    std::vector<std::string> events;
    std::vector<std::uint32_t> modules{0x1000u, 0x2000u};
    std::uint32_t platform = 0u;
    std::uint32_t expected_invade = 1u;
    std::uint32_t sym_init_result = 1u;
    std::uint32_t first_enumerate_result = 1u;
    std::uint32_t second_enumerate_result = 1u;
    std::uint32_t second_count_override = 0xffffffffu;
    std::uint32_t allocation = 0x5000u;
    std::uint32_t filename_failure_module = 0u;
    std::uint32_t load_failure_module = 0u;
    std::uint32_t last_error = 0u;
    std::uint32_t enumerate_calls = 0u;
};

std::uint32_t Run(Runtime& runtime) {
    return pal4::SymbolModuleLoader::Initialize(
        0x11u, 0x22u, 0x33u, runtime.expected_invade, runtime);
}

void CheckDirectAndEarlyFailures() {
    Runtime nt;
    nt.platform = 1u;
    nt.sym_init_result = 7u;
    Check(Run(nt) == 7u &&
              nt.events == std::vector<std::string>{"platform", "sym-init"},
          "NT path returns raw SymInitialize result");
    Runtime no_invade;
    no_invade.expected_invade = 0u;
    no_invade.sym_init_result = 9u;
    Check(Run(no_invade) == 9u &&
              no_invade.events == std::vector<std::string>{
                  "platform", "sym-init"},
          "zero invade flag uses direct path even on old platform");
    Runtime failed_init;
    failed_init.sym_init_result = 0u;
    Check(Run(failed_init) == 0u &&
              failed_init.events == std::vector<std::string>{
                  "platform", "sym-init"},
          "failed initial SymInitialize stops without cleanup");
    Runtime failed_count;
    failed_count.first_enumerate_result = 0u;
    Check(Run(failed_count) == 0u &&
              failed_count.events == std::vector<std::string>{
                  "platform", "sym-init", "count", "cleanup"},
          "first enumeration failure cleans symbols without allocation");
}

void CheckEnumerationAndFilenameFailures() {
    Runtime failed_enumeration;
    failed_enumeration.second_enumerate_result = 0u;
    Check(Run(failed_enumeration) == 0u &&
              failed_enumeration.events == std::vector<std::string>{
                  "platform", "sym-init", "count", "allocate", "enumerate",
                  "cleanup", "free"},
          "second enumeration failure cleans symbols before freeing array");

    Runtime missing_filename;
    missing_filename.filename_failure_module = 0x2000u;
    Check(Run(missing_filename) == 0u &&
              missing_filename.events[missing_filename.events.size() - 2u] ==
                  "cleanup" &&
              missing_filename.events.back() == "free" &&
              missing_filename.events[missing_filename.events.size() - 3u] ==
                  "file-name:8192",
          "filename failure stops on that module and cleans both resources");

    Runtime load_failure;
    load_failure.load_failure_module = 0x1000u;
    load_failure.last_error = 5u;
    Check(Run(load_failure) == 0u &&
              load_failure.events[load_failure.events.size() - 3u] ==
                  "get-error" &&
              load_failure.events[load_failure.events.size() - 2u] ==
                  "cleanup" &&
              load_failure.events.back() == "free",
          "failed SymLoadModule with nonzero last error aborts");
}

void CheckSuccessfulAndZeroErrorPaths() {
    Runtime success;
    Check(Run(success) == 1u && success.events.back() == "free" &&
              success.events[success.events.size() - 2u] == "sym-load:8192",
          "all modules load and allocated array is freed");
    Runtime ignored_failure;
    ignored_failure.load_failure_module = 0x1000u;
    ignored_failure.last_error = 0u;
    Check(Run(ignored_failure) == 1u &&
              ignored_failure.events[ignored_failure.events.size() - 2u] ==
                  "sym-load:8192" &&
              ignored_failure.events.back() == "free",
          "zero last error makes failed SymLoadModule nonfatal");
    Runtime reduced_count;
    reduced_count.second_count_override = 1u;
    Check(Run(reduced_count) == 1u &&
              reduced_count.events.back() == "free" &&
              reduced_count.events[reduced_count.events.size() - 2u] ==
                  "sym-load:4096",
          "second enumeration's updated count controls module loop");
    Runtime empty;
    empty.modules.clear();
    Check(Run(empty) == 1u && empty.events.back() == "free" &&
              empty.events.size() == 6u,
          "zero module count still allocates, enumerates and frees");
}

}  // namespace

int main() {
    CheckDirectAndEarlyFailures();
    CheckEnumerationAndFilenameFailures();
    CheckSuccessfulAndZeroErrorPaths();
    std::cout << "3 symbol module loader test cases passed\n";
}
