#include "pal4/script_round_times.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptRoundTimesRuntime {
    std::vector<std::string> calls;
    std::uint32_t current = 0u;
    std::uint32_t written = 0u;
    std::uint32_t saved = 0u;
    std::uint32_t accesses = 0u;

    std::uint32_t GlobalData() override {
        calls.emplace_back("global");
        return 0x1000u + 0x100u * accesses++;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1028u, "read from first global object");
        calls.emplace_back("read");
        return current;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x1128u, "write to second global object");
        calls.emplace_back("write");
        written = value;
    }
    void SaveSettings(std::uint32_t global_data) override {
        Check(global_data == 0x1200u, "save on third global object");
        calls.emplace_back("save");
        saved = global_data;
    }
};

void IncrementAndWrap() {
    for (const auto initial : {0u, 19u,
                               std::numeric_limits<std::uint32_t>::max()}) {
        Runtime runtime;
        runtime.current = initial;
        pal4::ScriptRoundTimes::Increment(runtime);
        Check(runtime.written == initial + 1u && runtime.saved == 0x1200u,
              "unsigned 32-bit increment and save");
        Check(runtime.accesses == 3u &&
                  runtime.calls == std::vector<std::string>{
                      "global", "read", "global", "write", "global", "save"},
              "three separate singleton acquisitions in original order");
    }
}

}  // namespace

int main() {
    try {
        IncrementAndWrap();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
