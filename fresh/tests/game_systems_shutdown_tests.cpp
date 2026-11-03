#include "pal4/game_systems_shutdown.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
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

class Runtime final : public pal4::IGameSystemsShutdownRuntime {
public:
    void ShutdownRenderSystem() override {
        events.emplace_back("shutdown-render");
    }
    std::uint32_t InitializeLogger() override {
        events.emplace_back("logger-init");
        return 0x1234u;
    }
    std::int32_t AppendLog(const std::uint32_t logger,
                            const std::string_view format,
                            const std::string_view source_file,
                            const std::uint32_t line) override {
        events.emplace_back("logger-append");
        Check(logger == 0x1234u &&
                  format ==
                      "FILE:  [%s]\nLINE:  [%d]\nINFO:  Terminate3D success\n\n\n" &&
                  source_file ==
                      R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)" &&
                  line == 0x493u,
              "shutdown log arguments");
        return result;
    }
    std::int32_t result = 0;
    std::vector<std::string> events;
};

}  // namespace

int main() {
    pal4::GameSystemsShutdown shutdown;
    for (const std::int32_t value : {0, 42, -1}) {
        Runtime runtime;
        runtime.result = value;
        Check(shutdown.Shutdown(runtime) == value &&
                  runtime.events ==
                      std::vector<std::string>{
                          "shutdown-render", "logger-init",
                          "logger-append"},
              "shutdown order and logger result forwarding");
    }
    std::cout << "3 game shutdown result paths passed\n";
}
