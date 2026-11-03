#include "pal4/directory_path_processor.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IDirectoryPathRuntime {
public:
    bool SetCurrentDirectory(std::string_view path) override {
        calls.push_back("set:" + std::string(path));
        return set_succeeds;
    }
    void GetLastError() override { calls.push_back("error"); }
    bool CreateDirectory(std::string_view path) override {
        calls.push_back("create:" + std::string(path));
        return create_succeeds;
    }

    bool set_succeeds = true;
    bool create_succeeds = true;
    std::vector<std::string> calls;
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::DirectoryPathProcessor::Process("", runtime) &&
              runtime.calls.empty(), "empty path succeeds without calls");
    Check(pal4::DirectoryPathProcessor::Process("C:\\", runtime) &&
              runtime.calls.empty(), "drive root is skipped");

    Check(pal4::DirectoryPathProcessor::Process("C:\\save\\slot", runtime),
          "drive path succeeds");
    Check(runtime.calls == std::vector<std::string>{
              "set:C:\\save\\", "set:C:\\save\\slot"},
          "drive prefix and final component order");

    runtime.calls.clear();
    runtime.set_succeeds = false;
    Check(pal4::DirectoryPathProcessor::Process("save\\slot", runtime),
          "created directory succeeds");
    Check(runtime.calls == std::vector<std::string>{
              "set:save\\", "error", "create:save\\",
              "set:save\\slot", "error", "create:save\\slot"},
          "failed change-directory creates each prefix without retry");

    runtime.calls.clear();
    runtime.create_succeeds = false;
    Check(!pal4::DirectoryPathProcessor::Process("save\\slot", runtime),
          "creation failure propagates");
    Check(runtime.calls == std::vector<std::string>{
              "set:save\\", "error", "create:save\\"},
          "stops at first failed prefix");

    runtime.calls.clear();
    runtime.set_succeeds = true;
    runtime.create_succeeds = true;
    Check(pal4::DirectoryPathProcessor::Process(
              "\\\\server\\share\\folder", runtime),
          "UNC path succeeds");
    Check(runtime.calls == std::vector<std::string>{
              "set:\\\\server\\share\\",
              "set:\\\\server\\share\\folder"},
          "UNC server prefix is skipped");

    runtime.calls.clear();
    Check(pal4::DirectoryPathProcessor::Process("dir\\", runtime) &&
              runtime.calls == std::vector<std::string>{"set:dir\\"},
          "trailing separator is processed once");
}
