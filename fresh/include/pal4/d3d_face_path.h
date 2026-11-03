#pragma once

#include "pal4/d3d_triangle_face.h"
#include "pal4/d3d_face_trace_reachability.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct D3dFacePathPoint final {
    D3dTriangleVector3 position{};
    std::uint32_t face = 0u;
};

static_assert(sizeof(D3dFacePathPoint) == 16);

struct D3dFacePathState final {
    std::array<std::uint32_t, 12u> words{};
};

static_assert(sizeof(D3dFacePathState) == 48);

class ID3dFacePathRuntime {
public:
    virtual ~ID3dFacePathRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class D3dFacePathRuntimeStub final : public ID3dFacePathRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    void Free(std::uint32_t) override {}
};

class D3dFacePath final {
public:
    // IDA 0x6042f0.
    [[nodiscard]] static std::uint32_t Reset(
        D3dFacePathState& state, std::uint32_t storage_address,
        const D3dTriangleVector3& start, std::uint32_t start_face,
        const D3dTriangleVector3& end, std::uint32_t end_face,
        ID3dFacePathRuntime& runtime);
    // IDA 0x6043a0.
    [[nodiscard]] static std::uint32_t Append(
        D3dFacePathState& state, const D3dTriangleVector3& point,
        std::uint32_t face, ID3dFacePathRuntime& runtime);
    // IDA 0x604410.
    [[nodiscard]] static std::uint32_t AppendEnd(
        D3dFacePathState& state, ID3dFacePathRuntime& runtime);
    // IDA 0x604460.
    [[nodiscard]] static std::uint32_t ListFieldsAddress(
        std::uint32_t state_address);
    // IDA 0x604470.
    [[nodiscard]] static std::uint32_t SkipReachable(
        const D3dFacePathState& state, std::uint32_t cursor,
        ID3dFacePathRuntime& runtime,
        ID3dFaceTraceReachabilityRuntime& trace_runtime);
    // IDA 0x604500.
    static void CopyPoint(D3dFacePathPoint* destination,
                          const D3dFacePathPoint& source);
};

}  // namespace pal4
