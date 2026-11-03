#pragma once
#include "pal4/camera_projection_matrix.h"

namespace pal4 {
// Native x86 SDK objects only; engine must be started. Copies use typed local
// RwMatrix values so the host's word-array storage need not alias SDK structs.
class NativeCameraProjectionRuntime final : public ICameraProjectionRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    float ReadFloat(std::uint32_t address) override;
    std::uint32_t GetObjectTransform(std::uint32_t frame) override;
    void CopyMatrix(CameraProjectionMatrixImage& destination, std::uint32_t source) override;
    void InvertMatrix(CameraProjectionMatrixImage& destination, std::uint32_t source) override;
    void Scale(CameraProjectionMatrixImage& matrix, const CameraProjectionVector3& scale, std::uint32_t mode) override;
    void Translate(CameraProjectionMatrixImage& matrix, const CameraProjectionVector3& offset, std::uint32_t mode) override;
    void Multiply(CameraProjectionMatrixImage& destination, const CameraProjectionMatrixImage& left,
                  const CameraProjectionMatrixImage& right) override;
    std::uint32_t SetTransform(std::uint32_t transform_id, const CameraProjectionMatrixImage& matrix) override;
};
}
