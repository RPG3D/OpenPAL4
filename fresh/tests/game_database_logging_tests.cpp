#include "pal4/game_database_logging.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDatabaseLoggingRuntime {
    std::vector<std::string> calls;
    std::uint32_t prepared = 0x9000u;
    std::uint32_t result = 0x1234u;

    std::uint32_t Prepare(const char* format, const char* file,
                          const std::uint32_t line,
                          const std::uint32_t message) override {
        calls.emplace_back("prepare");
        Check(std::string(file) ==
                  "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4"
                  "\\PAL4Game\\GameDB\\palGameDB.cpp",
              "GameDB logger source file");
        Check(message == 0x7000u, "GameDB logger forwards its message");
        if (line == 154u)
            Check(std::string(format) ==
                      "FILE:  [%s]\nLINE:  [%d]\nINFO:  dbLog:\t %s\n\n\n",
                  "database log format and source line");
        else
            Check(line == 161u && std::string(format) ==
                      "FILE:  [%s]\nLINE:  [%d]\nINFO:  AssertLog:\t %s\n\n\n",
                  "assertion log format and source line");
        return prepared;
    }
    std::uint32_t Append(const std::uint32_t value) override {
        calls.emplace_back("append");
        Check(value == prepared, "GameDB logger forwards prepared value");
        return result;
    }
};

void BothLogCallbacksForwardArgumentsAndReturn() {
    Runtime database;
    Check(pal4::GameDatabaseLogging::Database(0x7000u, database) ==
              0x1234u &&
              database.calls == std::vector<std::string>{"prepare", "append"},
          "database log calls prepare then append and forwards result");

    Runtime assertion;
    assertion.result = 0x4321u;
    Check(pal4::GameDatabaseLogging::Assertion(0x7000u, assertion) ==
              0x4321u &&
              assertion.calls == std::vector<std::string>{"prepare", "append"},
          "assertion log calls prepare then append and forwards result");
}

}  // namespace

int main() {
    try {
        BothLogCallbacksForwardArgumentsAndReturn();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
