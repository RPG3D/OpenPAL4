#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

using CameraProjectionMatrixImage = std::array<std::uint32_t, 16u>;
using CameraProjectionVector3 = std::array<float, 3u>;

static_assert(sizeof(CameraProjectionMatrixImage) == 64);

struct CameraProjectionGlobals final {
    std::uint32_t texture = 0u;
    std::uint32_t strength_bits = 0u;
};

class ICameraProjectionRuntime {
public:
    virtual ~ICameraProjectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t frame) = 0;
    virtual void CopyMatrix(CameraProjectionMatrixImage& destination,
                            std::uint32_t source) = 0;
    virtual void InvertMatrix(
        CameraProjectionMatrixImage& destination,
        std::uint32_t source) = 0;
    virtual void Scale(CameraProjectionMatrixImage& matrix,
                       const CameraProjectionVector3& scale,
                       std::uint32_t mode) = 0;
    virtual void Translate(CameraProjectionMatrixImage& matrix,
                           const CameraProjectionVector3& offset,
                           std::uint32_t mode) = 0;
    virtual void Multiply(CameraProjectionMatrixImage& destination,
                          const CameraProjectionMatrixImage& left,
                          const CameraProjectionMatrixImage& right) = 0;
    [[nodiscard]] virtual std::uint32_t SetTransform(
        std::uint32_t transform_id,
        const CameraProjectionMatrixImage& matrix) = 0;
};

class CameraProjectionMatrix final {
public:
    // IDA 0x5fd130.
    [[nodiscard]] static std::uint32_t Set(
        std::uint32_t texture, std::uint32_t scene,
        std::uint32_t camera, float strength, float depth,
        CameraProjectionGlobals& globals,
        ICameraProjectionRuntime& runtime);
};

}  // namespace pal4
