#include "pal4/script_pal_test_result.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Runtime final : pal4::IScriptPalTestResultRuntime {
    std::vector<std::string> calls;
    std::uint32_t expected_address = 0u;

    std::uint32_t GlobalData() override {
        calls.emplace_back("global");
        return 0x1000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address != expected_address)
            throw std::runtime_error("PAL test result address");
        calls.emplace_back("read");
        return address ^ 0x12345678u;
    }
};

}  // namespace

int main() {
    try {
        for (const auto index : {-10, -1, 3, 100}) {
            Runtime runtime;
            if (pal4::ScriptPalTestResult::Get(index, runtime) != 0u ||
                !runtime.calls.empty())
                throw std::runtime_error("invalid index must not access global data");
        }
        for (std::int32_t index = 0; index < 3; ++index) {
            Runtime runtime;
            runtime.expected_address = 0x101cu +
                static_cast<std::uint32_t>(index) * 4u;
            if (pal4::ScriptPalTestResult::Get(index, runtime) !=
                    (runtime.expected_address ^ 0x12345678u) ||
                runtime.calls != std::vector<std::string>{"global", "read"})
                throw std::runtime_error("PAL test result selection or order");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
