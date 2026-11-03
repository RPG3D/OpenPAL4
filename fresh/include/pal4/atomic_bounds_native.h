#pragma once
#include "pal4/atomic_bounding_box_builder.h"

namespace pal4 {
// Borrow the two game mode words; their owners must outlive this adapter.
class NativeAtomicBoundsRuntime final : public IAtomicBoundingBoxRuntime,
                                       public IAtomicBoxTransformRuntime {
public:
    NativeAtomicBoundsRuntime(const std::uint32_t& direct_mode,
                              const std::uint32_t& secondary_mode)
        : direct_mode_(direct_mode), secondary_mode_(secondary_mode) {}
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t Allocate(std::uint32_t size,std::uint32_t flags) override;
    void Free(std::uint32_t address) override;
    bool UseDirectTransform() override;
    bool UseSecondaryDirectTransform() override;
    std::uint32_t GetObjectTransform(std::uint32_t frame) override;
    void SetupTransformMatrix(AtomicMatrix& destination,std::uint32_t source) override;
    void MultiplyMatrices(AtomicMatrix& destination,std::uint32_t object_transform,
                          const AtomicMatrix& parent_transform) override;
    void TransformPoints(std::uint32_t destination,std::uint32_t source,
                         std::int32_t count,const AtomicMatrix& matrix) override;
    void AddPoint(std::uint32_t box,std::uint32_t point) override;
    void TransformCorners(AtomicBoxCorners& corners,std::uint32_t matrix) override;
    std::uint32_t CalculateBox(std::uint32_t destination,const AtomicBoxCorners& corners) override;
private:
    const std::uint32_t& direct_mode_;
    const std::uint32_t& secondary_mode_;
};
}
