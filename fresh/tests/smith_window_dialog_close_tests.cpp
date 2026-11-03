#include "pal4/smith_window.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowDialogCloseRuntime {
    std::vector<std::string> calls;
    std::uint32_t array = 0u;
    std::uint32_t record = 0u;

    std::uint32_t UIFrameManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    void InsertFrameRecord(std::uint32_t target,
                           std::uint32_t value) override {
        array = target;
        record = value;
        calls.emplace_back("insert");
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        Check(pal4::SmithWindow::OnDialogClose(
                  0x1000u, 0xabcdef01u, runtime) == 1u &&
                  runtime.array == 0x5134u &&
                  runtime.record == 0x10e8u &&
                  runtime.calls ==
                      std::vector<std::string>{"manager", "insert"},
              "dialog close inserts the Smith frame record");
        Runtime zero_argument;
        Check(pal4::SmithWindow::OnDialogClose(
                  0x1000u, 0u, zero_argument) == 1u &&
                  zero_argument.array == runtime.array &&
                  zero_argument.record == runtime.record,
              "callback argument does not affect insertion");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
