#include "pal4/smith_window.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowResetClickRuntime {
    std::vector<std::uint32_t> objects;
    std::uint8_t result = 0u;

    std::uint8_t OnResetClick(std::uint32_t object) override {
        objects.push_back(object);
        return result;
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        runtime.result = 0xa5u;
        Check(pal4::SmithWindow::OnResetClickThunk(
                  0x1000u, 0x12345678u, runtime) == 0xa5u &&
                  runtime.objects ==
                      std::vector<std::uint32_t>{0x1000u},
              "thunk forwards object and result");
        runtime.result = 0u;
        Check(pal4::SmithWindow::OnResetClickThunk(
                  0x2000u, 0u, runtime) == 0u &&
                  runtime.objects.back() == 0x2000u,
              "ignored argument does not alter forwarding");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
