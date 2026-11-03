#include "pal4/script_registration_history.h"

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

struct Runtime final : pal4::IScriptRegistrationHistoryRuntime {
    int result{17};
    std::vector<std::uint32_t> callbacks;
    int RegisterAtExit(std::uint32_t callback) override {
        callbacks.push_back(callback);
        return result;
    }
};

}  // namespace

int main() {
    pal4::ScriptRegistrationHistory history;
    Runtime runtime;
    Check(history.Initialize(0xA5u, runtime) == 17 &&
              history.Seed() == 0xA5u && history.Entries().empty() &&
              runtime.callbacks == std::vector<std::uint32_t>{0x7ECD90u},
          "startup initializes history and registers cleanup in order");

    std::string declaration = "int abs( int )";
    std::string description = "absolute value";
    history.Record(declaration.c_str(), description.c_str());
    declaration.assign("float sinf( float )");
    description.assign("sine");
    history.Record(declaration.c_str(), description.c_str());
    Check(history.Entries().size() == 2 &&
              history.Entries()[0] == pal4::ScriptRegistrationHistory::Entry{
                                           "int abs( int )", "absolute value"} &&
              history.Entries()[1] == pal4::ScriptRegistrationHistory::Entry{
                                           "float sinf( float )", "sine"},
          "record appends independent declaration and description copies");

    history.Cleanup();
    Check(history.Entries().empty() && history.Entries().capacity() == 0,
          "cleanup destroys every pair and releases vector storage");
    runtime.result = -1;
    Check(history.Initialize(0x5Au, runtime) == -1 &&
              history.Seed() == 0x5Au && runtime.callbacks.size() == 2,
          "startup forwards the CRT registration result");
}
