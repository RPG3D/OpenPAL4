#include "pal4/smith_window.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

struct Runtime final : pal4::ISmithWindowConfirmClickRuntime {
    std::vector<std::uint32_t> objects;
    std::uint8_t result = 0;

    std::uint8_t OnConfirmTrade(std::uint32_t object) override {
        objects.push_back(object);
        return result;
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        runtime.result = 0xa5u;
        if (pal4::SmithWindow::OnConfirmClickThunk(
                0x1000u, 0x12345678u, runtime) != 0xa5u ||
            runtime.objects != std::vector<std::uint32_t>{0x1000u}) {
            throw std::runtime_error("confirm thunk forwards object and result");
        }
        runtime.result = 0u;
        if (pal4::SmithWindow::OnConfirmClickThunk(
                0x2000u, 0u, runtime) != 0u ||
            runtime.objects.back() != 0x2000u) {
            throw std::runtime_error("confirm thunk ignores stack argument");
        }
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
