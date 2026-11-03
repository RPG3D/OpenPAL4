#pragma once
#include "pal4/camera_collision_render.h"
#include <rwcore.h>

namespace pal4 {
// One owner per projection render sequence; SDK callbacks borrow this instance.
class NativeCameraCollisionRenderRuntime final : public ICameraCollisionRenderRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    float ReadFloat(std::uint32_t address) override;
    std::int32_t Ftol(double value) override;
    std::uint32_t SetDeviceState(std::uint32_t state, std::uint32_t value) override;
    void SetupTransformMatrix(std::array<float,16>& destination,std::uint32_t source) override;
    void ScaleMatrix(std::array<float,16>& matrix,const std::array<float,3>& scale,std::uint32_t mode) override;
    void TranslateMatrix(std::array<float,16>& matrix,const std::array<float,3>& offset,std::uint32_t mode) override;
    void TraverseIntersections(std::uint32_t world,std::uint32_t intersection,
        std::uint32_t callback,CollisionRenderContext& context) override;
    std::uint32_t VertexCount() override;
    void SetVertexCount(std::uint32_t count) override;
    void SetStatistic(std::uint32_t count) override;
    std::uint32_t TransformVertices(std::uint32_t count,std::uint32_t matrix,std::uint32_t flags) override;
    void RenderPrimitive(std::uint32_t primitive) override;
    void EndPrimitive() override;
    void TransformPoints(std::array<std::array<float,3>,3>& destination,
        const std::array<std::array<float,3>,3>& source,const std::array<float,16>& matrix) override;
    std::uint32_t GetRenderState(std::uint32_t state) override;
    void SetRenderState(std::uint32_t state,std::uint32_t value) override;
    void WriteVertex(std::uint32_t index,const CollisionImmediateVertex& vertex) override;
    std::uint32_t Statistic() const noexcept { return statistic_; }
private:
    std::array<RwIm3DVertex,1500> vertices_{};
    std::uint32_t count_{};
    std::uint32_t statistic_{};
    void* device_{};
};
}
