#include "pal4/d3d_capability_checker.h"

#include <cstring>

namespace pal4 {

bool D3DCapabilityChecker::CheckVertexShaderVersion(
    const std::uint32_t required, ID3DCapabilityRuntime& runtime) {
    return CheckVersion(required, 0xc4u, runtime);
}

bool D3DCapabilityChecker::CheckPixelShaderVersion(
    const std::uint32_t required, ID3DCapabilityRuntime& runtime) {
    return CheckVersion(required, 0xccu, runtime);
}

bool D3DCapabilityChecker::CheckVersion(
    const std::uint32_t required, const std::uint32_t offset,
    ID3DCapabilityRuntime& runtime) {
    std::array<std::uint8_t, 0x130> caps{};
    const auto device = runtime.CurrentDevice();
    runtime.QueryCapabilities(device, caps);
    std::uint32_t observed{};
    std::memcpy(&observed, caps.data() + offset, sizeof(observed));
    return observed >= required;
}

}  // namespace pal4
