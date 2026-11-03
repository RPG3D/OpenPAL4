#include "pal4/d3d_capability_checker.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const std::string_view message) {
    if (!condition) throw std::runtime_error(message.data());
}

class Runtime final : public pal4::ID3DCapabilityRuntime {
public:
    std::uint32_t c4{};
    std::uint32_t cc{};
    std::vector<std::uint32_t> devices;
    std::uint32_t current_calls{};

    std::uint32_t CurrentDevice() override {
        ++current_calls;
        return 0x1234u;
    }
    void QueryCapabilities(
        const std::uint32_t device,
        std::array<std::uint8_t, 0x130>& caps) override {
        devices.push_back(device);
        std::memcpy(caps.data() + 0xc4u, &c4, sizeof(c4));
        std::memcpy(caps.data() + 0xccu, &cc, sizeof(cc));
    }
};

void DistinctOffsetsAndUnsignedComparison() {
    Runtime runtime;
    runtime.c4 = 0xffff0200u;
    runtime.cc = 0xffff0101u;
    Check(pal4::D3DCapabilityChecker::CheckVertexShaderVersion(
              0xffff0200u, runtime) &&
              !pal4::D3DCapabilityChecker::CheckVertexShaderVersion(
                  0xffff0201u, runtime) &&
              pal4::D3DCapabilityChecker::CheckPixelShaderVersion(
                  0xffff0101u, runtime) &&
              !pal4::D3DCapabilityChecker::CheckPixelShaderVersion(
                  0xffff0200u, runtime),
          "version checks use distinct caps words and inclusive comparison");
    Check(runtime.current_calls == 4u &&
              runtime.devices == std::vector<std::uint32_t>(4u, 0x1234u),
          "each check fetches current device and invokes its caps query");
    runtime.c4 = 0x80000000u;
    Check(pal4::D3DCapabilityChecker::CheckVertexShaderVersion(
              0x7fffffffu, runtime),
          "caps comparison is unsigned");
}

}  // namespace

int main() {
    DistinctOffsetsAndUnsignedComparison();
    std::cout << "D3D capability contract tests passed\n";
}
