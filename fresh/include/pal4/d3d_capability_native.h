#pragma once
#include "pal4/d3d_capability_checker.h"

namespace pal4 {
class NativeD3DCapabilityRuntime final : public ID3DCapabilityRuntime {
public:
    [[nodiscard]] std::uint32_t CurrentDevice() override;
    void QueryCapabilities(std::uint32_t device,
        std::array<std::uint8_t, 0x130>& caps) override;
};
}
