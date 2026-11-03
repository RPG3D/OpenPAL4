#include "pal4/exception_log_setup.h"

#include <cstdarg>
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

class Runtime final : public pal4::IExceptionLogRuntime,
                      public pal4::IExceptionLogSetupRuntime {
public:
    std::string Format(const char*, std::va_list) override { return {}; }
    std::uintptr_t Open(const char* path, const char* mode) override {
        events.emplace_back(std::string("open:") + path + ":" + mode);
        return open_result;
    }
    void Print(std::uintptr_t stream, const char*) override {
        Check(stream == open_result, "header uses opened stream");
        events.emplace_back("print");
    }
    int Close(std::uintptr_t stream) override {
        Check(stream == open_result, "header closes opened stream");
        events.emplace_back("close");
        return -1;
    }
    std::uint32_t GetModuleHandle(const char* name) override {
        const std::string module = name == nullptr ? "<main>" : name;
        events.emplace_back("module:" + module);
        if (module == "<main>") return 1u;
        if (module == "DBGHELP.DLL") return 2u;
        return 3u;
    }
    bool AddModule(std::uint32_t module) override {
        events.emplace_back("add:" + std::to_string(module));
        return add_results.at(static_cast<std::size_t>(module - 1u));
    }
    std::uint32_t SetSeTranslator(std::uint32_t handler) override {
        Check(handler == pal4::ExceptionLogSetup::kGlobalHandlerAddress,
              "sets the VC6 structured exception translator observed in IDA");
        events.emplace_back("install");
        return 0x5678u;
    }

    std::vector<std::string> events;
    std::vector<bool> add_results{true, true, true};
    std::uintptr_t open_result = 0x1234u;
};

void CheckAllModules() {
    Runtime runtime;
    pal4::ExceptionLog log;
    Check(pal4::ExceptionLogSetup::Install("seh.log", log, runtime, runtime) ==
              0x5678u && log.Path() == "seh.log" &&
              runtime.events == std::vector<std::string>{
                  "open:seh.log:wt", "print", "print", "print", "close",
                  "module:<main>", "add:1", "module:DBGHELP.DLL", "add:2",
                  "module:KERNEL32.DLL", "add:3", "install"},
          "all modules are added in order and previous handler is returned");
}

void CheckEarlyAddFailures() {
    for (std::size_t failed = 0u; failed < 2u; ++failed) {
        Runtime runtime;
        runtime.add_results[failed] = false;
        pal4::ExceptionLog log;
        (void)pal4::ExceptionLogSetup::Install("seh.log", log, runtime, runtime);
        const std::vector<std::string> expected_tail = failed == 0u
            ? std::vector<std::string>{"module:<main>", "add:1", "install"}
            : std::vector<std::string>{"module:<main>", "add:1",
                                       "module:DBGHELP.DLL", "add:2", "install"};
        Check(std::vector<std::string>(runtime.events.begin() + 5,
                                       runtime.events.end()) == expected_tail,
              "failed module add short circuits later modules, still installs handler");
    }
}

void CheckLogFailureStillInstalls() {
    Runtime runtime;
    runtime.open_result = 0u;
    pal4::ExceptionLog log;
    (void)pal4::ExceptionLogSetup::Install("bad.log", log, runtime, runtime);
    Check(runtime.events.front() == "open:bad.log:wt" &&
              runtime.events[1] == "module:<main>" &&
              runtime.events.back() == "install",
          "log open failure does not prevent module registration or installation");
}

}  // namespace

int main() {
    CheckAllModules();
    CheckEarlyAddFailures();
    CheckLogFailureStillInstalls();
    std::cout << "3 exception log setup test cases passed\n";
}
