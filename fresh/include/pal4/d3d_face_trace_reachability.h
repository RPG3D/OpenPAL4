#pragma once

#include "pal4/d3d_line2.h"

#include <cstdint>

namespace pal4 {

class ID3dFaceTraceReachabilityRuntime {
public:
    virtual ~ID3dFaceTraceReachabilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Trace(
        std::uint32_t face, const D3dLine2State& line,
        std::uint32_t& neighbor) = 0;
};

class D3dFaceTraceReachabilityRuntimeStub final
    : public ID3dFaceTraceReachabilityRuntime {
public:
    [[nodiscard]] std::uint32_t Trace(
        std::uint32_t, const D3dLine2State&,
        std::uint32_t&) override { return 0u; }
};

class D3dFaceTraceReachability final {
public:
    // IDA 0x603f30.
    [[nodiscard]] static bool CanReach(
        std::uint32_t first_face, const D3dTriangleVector3& start,
        const D3dTriangleVector3& end,
        ID3dFaceTraceReachabilityRuntime& runtime);
};

}  // namespace pal4
