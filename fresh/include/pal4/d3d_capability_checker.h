#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class ID3DCapabilityRuntime {
public:
    virtual ~ID3DCapabilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CurrentDevice() = 0;
    virtual void QueryCapabilities(
        std::uint32_t device, std::array<std::uint8_t, 0x130>& caps) = 0;
};

class D3DCapabilityChecker final {
public:
    [[nodiscard]] static bool CheckVertexShaderVersion(
        std::uint32_t required, ID3DCapabilityRuntime& runtime);
    [[nodiscard]] static bool CheckPixelShaderVersion(
        std::uint32_t required, ID3DCapabilityRuntime& runtime);

private:
    [[nodiscard]] static bool CheckVersion(
        std::uint32_t required, std::uint32_t offset,
        ID3DCapabilityRuntime& runtime);
};

}  // namespace pal4
