#pragma once

#include "pal4/d3d_triangle_face.h"

#include <cstdint>

namespace pal4 {

struct D3dHeightResult final {
    float height = 0.0f;
    std::uint32_t eax = 0u;
};

class ID3dHeightRuntime {
public:
    virtual ~ID3dHeightRuntime() = default;
    [[nodiscard]] virtual D3dHeightResult Evaluate(
        float x, float z) = 0;
};

class D3dHeightRuntimeStub final : public ID3dHeightRuntime {
public:
    [[nodiscard]] D3dHeightResult Evaluate(float, float) override {
        return {};
    }
};

class D3dHeightProjection final {
public:
    // IDA 0x602220.
    [[nodiscard]] static std::uint32_t Project(
        D3dTriangleVector3& point, ID3dHeightRuntime& runtime);
};

}  // namespace pal4
